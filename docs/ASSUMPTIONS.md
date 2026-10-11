# Assumptions

This document records all simplifying assumptions, approximations, and physical constants in OptiPlan.
Each entry documents **what** the assumption is, **why** it was made, and **what the impact** is.

---

## 1. Physical Layer (Phase 1)

### 1.1 Flat-Top Nyquist-WDM Spectrum
- **Assumption:** The WDM spectrum is assumed to consist of ideal Nyquist rectangular channels with flat Power Spectral Density $G_{\text{WDM}} = P_{\text{ch}} / B_{\text{ch}}$ across the occupied bandwidth $B_{\text{WDM}} = N_{\text{ch}} \cdot \Delta f$.
- **Why:** Enables closed-form integration of the 4-wave mixing / Cross-Phase Modulation kernel in the Gaussian Noise model (Poggiolini et al., 2012, Eq. 17).
- **Impact:** Slightly overestimates NLI for channels with roll-off (e.g. root-raised cosine with $\beta = 0.1 - 0.2$), yielding a conservative SNR estimate (typically within $0.2 - 0.4\text{ dB}$).

### 1.2 Incoherent Accumulation with Multi-Span Coherence Factor ($\epsilon = 0.05$)
- **Assumption:** Nonlinear interference power accumulates across $N_s$ spans as $N_s^{1+\epsilon} \cdot P_{\text{NLI,span}}$ with $\epsilon = 0.05$. In inhomogeneous routes (links with different channel occupancies), generalized $L_{1+\epsilon}$ norm accumulation is used:
  $$\eta_{\text{total}} = \left( \sum_{i=1}^{N_s} \eta_i^{\frac{1}{1+\epsilon}} \right)^{1+\epsilon}$$
- **Why:** Fully incoherent accumulation ($\epsilon = 0$) assumes NLI from different spans adds as uncorrelated noise powers, which slightly underestimates NLI in dispersion-unmanaged coherent links due to partial phase coherence in self-phase modulation (SPM). A coherence factor of $\epsilon \approx 0.05 - 0.06$ matches split-step Fourier simulations well across standard spans (Poggiolini et al., 2014; Carena et al., 2014).
- **Impact:** Captures realistic multi-span NLI penalty without the computational overhead of the full Enhanced GN (EGN) model.

### 1.3 Full EDFA Span Loss Compensation
- **Assumption:** Every in-line optical amplifier (EDFA) provides gain $G$ exactly equal to the total power attenuation of the preceding span ($G = 10^{\alpha_{\text{dB}} L_s / 10}$).
- **Why:** Standard operational regime in transparent optical core networks.
- **Impact:** Power launched into each fiber span remains constant along a link. Neglects amplifier gain saturation and dynamic ripple.

### 1.4 Dual-Polarization Manakov Equation Factor ($8/27$)
- **Assumption:** Fiber Kerr nonlinearity in standard single-mode fiber (SSMF) is governed by the Manakov equation for randomly birefringent fiber, giving the prefactor $8/27$ in the NLI power spectral density.
- **Why:** In standard telecommunication fiber, rapid random polarization mode dispersion averages nonlinear interactions over the Poincaré sphere.
- **Impact:** Standard in modern coherent transmission modeling; accurate for standard single-mode fibers with low PMD.

### 1.5 Modulation BER Approximations
- **Assumption:** BER for DP-BPSK is exact ($Q(\sqrt{2 \cdot \text{SNR}})$). For DP-QPSK, $BER \approx Q(\sqrt{\text{SNR}})$. For DP-8QAM and DP-16QAM, Gray-coded nearest-neighbor approximations are used:
  - 8-QAM: $BER \approx \frac{2}{3} Q\left(\sqrt{\frac{3}{7} \text{SNR}}\right)$
  - 16-QAM: $BER \approx \frac{3}{8} Q\left(\sqrt{\frac{\text{SNR}}{5}}\right)$
  A numerical bisection solver is also provided to solve for required SNR directly when exact inverted formulas are evaluated.
- **Why:** Standard textbook formulas (Proakis, Section 5.2) providing accurate required SNR thresholds for pre-FEC BER targets ($10^{-2}$ to $10^{-3}$).
- **Impact:** Error in required SNR is less than $0.05\text{ dB}$ in the typical pre-FEC operating range ($BER \le 10^{-2}$).
