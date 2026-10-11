/// @file main.cpp
/// @brief OptiPlan interactive CLI / demo executable showcasing Phase 0, 1, and 2 engines.

#include <iomanip>
#include <iostream>
#include <string>

#include "optiplan/config.hpp"
#include "optiplan/gn_model.hpp"
#include "optiplan/modulation.hpp"
#include "optiplan/physical_constants.hpp"
#include "optiplan/routing.hpp"
#include "optiplan/spectrum.hpp"
#include "optiplan/topology.hpp"

using namespace optiplan;

void printBanner() {
    std::cout << "\n"
              << "======================================================================\n"
              << "   OptiPlan: QoT-Aware Optical Network Planner (C++17 Engine)        \n"
              << "======================================================================\n\n";
}

int main(int argc, char* argv[]) {
    printBanner();

    std::string config_file = "configs/default.json";
    std::string topo_file = "data/topologies/nsfnet/topology.json";

    if (argc > 1)
        topo_file = argv[1];
    if (argc > 2)
        config_file = argv[2];

    try {
        // 1. Load Configuration
        std::cout << "[1] Loading Configuration from '" << config_file << "'...\n";
        Config cfg = Config::loadFromFile(config_file);
        std::cout << "    - Fiber Attenuation: " << cfg.fiber.attenuation_dB_per_km << " dB/km\n"
                  << "    - Fiber Dispersion:  " << cfg.fiber.dispersion_ps_nm_km << " ps/(nm*km)\n"
                  << "    - Fiber Gamma:       " << cfg.fiber.gamma_per_W_km << " /(W*km)\n"
                  << "    - EDFA Noise Figure: " << cfg.amplifier.noise_figure_dB << " dB\n"
                  << "    - Channels:          " << cfg.channel.num_channels << " x "
                  << cfg.channel.channel_spacing_GHz << " GHz grid (32 GBaud)\n\n";

        // 2. Load Network Topology
        std::cout << "[2] Loading Network Topology from '" << topo_file << "'...\n";
        Topology topo = Topology::loadFromFile(topo_file);
        std::cout << "    - Topology Name:     " << topo.name() << "\n"
                  << "    - Total Nodes:       " << topo.numNodes() << "\n"
                  << "    - Total Links:       " << topo.numLinks() << "\n"
                  << "    - Graph Connected:   " << (topo.isConnected() ? "YES" : "NO") << "\n\n";

        // 3. Routing Engine (Yen's KSP)
        std::cout << "[3] Routing Engine: Precomputing k-Shortest Paths (k=3)...\n";
        KShortestPathCache ksp_cache(topo, 3);
        std::cout << "    - Precomputed all-pairs shortest paths successfully.\n\n";

        // 4. Physical-Layer QoT Engine (GN Model) & Modulation Formats
        std::cout << "[4] Physical-Layer QoT Engine (Gaussian Noise Model):\n";
        GNModel gn_model(0.05);  // Coherence factor epsilon = 0.05
        auto formats = getStandardModulationFormats();

        std::cout << "    Supported Modulation Formats (at pre-FEC BER = 1e-2):\n";
        std::cout << "    " << std::setw(12) << "Format" << std::setw(14) << "Bits/Symbol"
                  << std::setw(16) << "Net Rate (Gb/s)" << std::setw(18) << "Required SNR (dB)\n";
        std::cout << "    " << std::string(60, '-') << "\n";
        for (const auto& fmt : formats) {
            std::cout << "    " << std::setw(12) << fmt->name() << std::setw(14)
                      << fmt->bitsPerSymbol() << std::setw(16) << fmt->netBitRateGbps()
                      << std::setw(18) << std::fixed << std::setprecision(2)
                      << fmt->requiredSnrDb(0.01) << " dB\n";
        }
        std::cout << "\n";

        // 5. Spectrum State & Demonstration
        std::cout << "[5] Spectrum Management & Sample Lightpath Allocations:\n";
        SpectrumState spectrum(topo.numLinks(), cfg.channel.num_channels);
        FirstFitPolicy first_fit;

        // Demonstrate a few sample demands:
        struct SampleDemand {
            int src;
            int dst;
            std::string label;
        };

        std::vector<SampleDemand> demands;
        if (topo.numNodes() >= 14) {
            demands = {{0, 2, "Seattle -> San Diego"},
                       {0, 13, "Seattle -> Washington DC"},
                       {1, 9, "Palo Alto -> Atlanta"},
                       {4, 11, "Boulder -> Ithaca"}};
        } else {
            demands = {{0, 3, "Node A -> Node D"}};
        }

        for (const auto& demand : demands) {
            std::cout << "  --------------------------------------------------------------------\n";
            std::cout << "  Demand: " << demand.label << " (Node " << demand.src << " -> Node "
                      << demand.dst << ")\n";

            const auto& candidate_paths = ksp_cache.getKShortestPaths(demand.src, demand.dst);
            std::cout << "  Found " << candidate_paths.size() << " candidate paths:\n";

            bool provisioned = false;
            for (size_t p_idx = 0; p_idx < candidate_paths.size(); ++p_idx) {
                const auto& path = candidate_paths[p_idx];

                // Prepare LinkSegments for QoT
                std::vector<LinkSegment> segments;
                for (int lid : path.link_ids) {
                    segments.push_back({topo.getLink(lid).length_km,
                                        std::max(1, spectrum.getOccupiedChannelCount(lid))});
                }

                auto qot = gn_model.evaluatePath(segments, cfg.channel.launch_power_dBm, cfg);
                SlotMask free_slots = spectrum.findAvailableSlots(path);

                // Best feasible modulation format
                std::shared_ptr<ModulationFormat> chosen_fmt = nullptr;
                for (auto it = formats.rbegin(); it != formats.rend(); ++it) {
                    if (qot.snr_db >= (*it)->requiredSnrDb(0.01, 1.0)) {  // 1 dB design margin
                        chosen_fmt = *it;
                        break;
                    }
                }

                int chosen_slot = first_fit.selectSlot(path, free_slots, spectrum);

                std::cout << "    Path #" << (p_idx + 1) << ": " << path.length_km << " km, "
                          << path.hops() << " hops | "
                          << "SNR: " << std::fixed << std::setprecision(2) << qot.snr_db << " dB, "
                          << "OSNR: " << qot.osnr_db << " dB | ";

                if (chosen_fmt && chosen_slot != -1 && !provisioned) {
                    std::cout << "SELECTED: " << chosen_fmt->name() << " ("
                              << chosen_fmt->netBitRateGbps() << " Gb/s) on Slot #" << chosen_slot
                              << "\n";
                    spectrum.allocateSlot(path, chosen_slot);
                    provisioned = true;
                } else if (!chosen_fmt) {
                    std::cout << "FEASIBILITY FAILED (SNR below minimum format threshold)\n";
                } else {
                    std::cout << "ALTERNATIVE: " << chosen_fmt->name()
                              << " (Available Slots: " << free_slots.count() << ")\n";
                }
            }
        }

        std::cout << "  --------------------------------------------------------------------\n\n";
        std::cout << "[6] Network Spectrum Summary:\n";
        std::cout << "    - Average Link Spectrum Utilization: " << std::fixed
                  << std::setprecision(2) << (spectrum.getAverageUtilization() * 100.0) << " %\n";
        std::cout << "    - Live Links with Allocated Channels: \n";
        for (int l = 0; l < topo.numLinks(); ++l) {
            int occ = spectrum.getOccupiedChannelCount(l);
            if (occ > 0) {
                const auto& link = topo.getLink(l);
                std::cout << "      * Link " << std::setw(2) << l << " ("
                          << topo.getNode(link.source).name << " <-> "
                          << topo.getNode(link.target).name << "): " << occ << "/"
                          << cfg.channel.num_channels << " slots occupied (" << std::fixed
                          << std::setprecision(1) << (spectrum.getLinkUtilization(l) * 100.0)
                          << "%)\n";
            }
        }

        std::cout << "\n[OK] OptiPlan execution completed successfully!\n\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] " << ex.what() << "\n\n";
        return 1;
    }
}
