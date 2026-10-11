#pragma once

/// @file planner.hpp
/// @brief Routing, Modulation, and Spectrum Assignment (RMSA) planner interfaces and implementations.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "optiplan/config.hpp"
#include "optiplan/lightpath.hpp"
#include "optiplan/qot_model.hpp"
#include "optiplan/routing.hpp"
#include "optiplan/spectrum.hpp"
#include "optiplan/topology.hpp"
#include "optiplan/traffic.hpp"

namespace optiplan {

/// Decision outcome for a connection request
struct PlanningDecision {
    bool accepted = false;
    Lightpath lightpath;
    std::string failure_reason;
    int candidates_evaluated = 0;
    int revalidations_performed = 0;
};

/// Abstract strategy interface for network planners
class IPlanner {
public:
    virtual ~IPlanner() = default;

    virtual std::string name() const = 0;

    /// Attempt to provision an incoming request
    virtual PlanningDecision planRequest(
        const Request& request,
        const Topology& topo,
        const KShortestPathCache& ksp_cache,
        SpectrumState& spectrum,
        const std::unordered_map<int, Lightpath>& active_lightpaths,
        const Config& config) = 0;
};

/// Baseline planner selecting modulation format based on static reach thresholds
class FixedMarginPlanner : public IPlanner {
public:
    explicit FixedMarginPlanner(std::shared_ptr<ISpectrumPolicy> policy = nullptr);

    std::string name() const override { return "Fixed-Margin Planner (Baseline)"; }

    PlanningDecision planRequest(
        const Request& request,
        const Topology& topo,
        const KShortestPathCache& ksp_cache,
        SpectrumState& spectrum,
        const std::unordered_map<int, Lightpath>& active_lightpaths,
        const Config& config) override;

    /// Select modulation format strictly based on physical path length [km]
    static std::shared_ptr<ModulationFormat> selectFormatByReach(double path_length_km);

private:
    std::shared_ptr<ISpectrumPolicy> policy_;
};

/// Ground-truth adaptive planner with physics-based QoT evaluation and existing-lightpath re-validation
class QoTAwarePlanner : public IPlanner {
public:
    explicit QoTAwarePlanner(std::shared_ptr<IQoTModel> qot_model = nullptr,
                             std::shared_ptr<ISpectrumPolicy> policy = nullptr,
                             double design_margin_dB = 1.0);

    std::string name() const override { return "QoT-Aware Adaptive Planner"; }

    PlanningDecision planRequest(
        const Request& request,
        const Topology& topo,
        const KShortestPathCache& ksp_cache,
        SpectrumState& spectrum,
        const std::unordered_map<int, Lightpath>& active_lightpaths,
        const Config& config) override;

    double designMarginDb() const { return design_margin_dB_; }
    void setDesignMarginDb(double margin) { design_margin_dB_ = margin; }

private:
    std::shared_ptr<IQoTModel> qot_model_;
    std::shared_ptr<ISpectrumPolicy> policy_;
    double design_margin_dB_;
    std::vector<std::shared_ptr<ModulationFormat>> formats_;
};

}  // namespace optiplan
