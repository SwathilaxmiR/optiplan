# References

All physical models, algorithms, datasets, and formulas used in OptiPlan are cited here.
Every entry has been verified to exist as of the date listed.

---

## Physical-Layer Models

### GN Model (Gaussian Noise Model)

1. **P. Poggiolini**, "The GN Model of Non-Linear Propagation in Uncompensated Coherent
   Optical Systems," *Journal of Lightwave Technology*, vol. 30, no. 24, pp. 3857–3879,
   Dec. 2012.  
   DOI: [10.1109/JLT.2012.2217729](https://doi.org/10.1109/JLT.2012.2217729)  
   - **Equation 17 / 26**: Closed-form approximation for flat-top Nyquist-WDM spectrum:
     $$G_{\text{NLI}} = \frac{8}{27} \gamma^2 G_{\text{WDM}}^3 L_{\text{eff}}^2 \frac{\text{asinh}\left(\frac{\pi^2}{2} |\beta_2| L_{\text{eff,a}} B_{\text{WDM}}^2\right)}{\pi |\beta_2| L_{\text{eff,a}}}$$
   - **Equation 33**: Incoherent span accumulation and optimum launch power scaling.

2. **A. Carena, G. Bosco, V. Curri, Y. Jiang, P. Poggiolini, and F. Forghieri**, "EGN
   model of non-linear fiber propagation," *Optics Express*, vol. 22, no. 13,
   pp. 16335–16362, Jun. 2014.  
   DOI: [10.1364/OE.22.016335](https://doi.org/10.1364/OE.22.016335)  
   - Detailed derivation of multi-span coherence accumulation factor $\epsilon \approx 0.05$
     accounting for residual phase coherence in self-phase modulation.

3. **P. Poggiolini and Y. Jiang**, "Recent Advances in the Modeling of the Impact of
   Nonlinear Fiber Propagation Effects on Uncompensated Coherent Transmission Systems,"
   *Journal of Lightwave Technology*, vol. 35, no. 3, pp. 458–480, Feb. 2017.  
   DOI: [10.1109/JLT.2016.2613893](https://doi.org/10.1109/JLT.2016.2613893)  
   - Review of closed-form asymptotic expressions and optimum launch power formula:
     $$P_{\text{opt}} = \left(\frac{P_{\text{ASE}}}{2\eta}\right)^{1/3}$$

---

## Modulation Formats & BER

4. **J. G. Proakis and M. Salehi**, *Digital Communications*, 5th ed. McGraw-Hill, 2008.  
   - Section 5.2.1: Binary Phase-Shift Keying (BPSK) error probability ($Q(\sqrt{2 E_b/N_0})$).
   - Section 5.2.4: Quadrature Phase-Shift Keying (QPSK) bit error probability ($Q(\sqrt{2 E_b/N_0}) = Q(\sqrt{SNR})$).
   - Section 5.2.8: M-ary Quadrature Amplitude Modulation (M-QAM) error bounds and Gray coding.

---

## Network Topologies

5. **SNDlib** — Survivable Network Design Library.  
   URL: https://sndlib.put.poznan.pl/  
   Maintained by Poznan University of Technology.  
   - Source for NSFNET 14-node and Nobel-EU topologies.

---

## Algorithms

6. **J. Y. Yen**, "Finding the K Shortest Loopless Paths in a Network," *Management
   Science*, vol. 17, no. 11, pp. 712–716, Jul. 1971.  
   DOI: [10.1287/mnsc.17.11.712](https://doi.org/10.1287/mnsc.17.11.712)  
   - The k-shortest paths algorithm used in routing.

---

## RMSA (Routing, Modulation, and Spectrum Assignment)

7. **B. C. Chatterjee, N. Sarma, and E. Oki**, "Routing and Spectrum Assignment
   Approaches in Elastic Optical Networks: A Comprehensive Survey," *IEEE Communications
   Surveys & Tutorials*, vol. 17, no. 3, pp. 1776–1800, 2015.  
   DOI: [10.1109/COMST.2015.2431731](https://doi.org/10.1109/COMST.2015.2431731)  
   - Survey of RSA/RMSA algorithms and spectrum policies.

---

*Last updated: Phase 1 Physical Layer implementation.*
