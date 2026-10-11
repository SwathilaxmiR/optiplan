# NSFNET 14-Node Topology — Source

## Description

The NSFNET (National Science Foundation Network) backbone topology is one of the most
widely used benchmark topologies in optical networking research. It consists of
**14 nodes** and **21 bidirectional links** spanning the continental United States.

## Data Source

The link distances used here are the **canonical academic version** commonly cited in
optical networking literature. They are sourced from:

1. **SNDlib** — Survivable Network Design Library  
   https://sndlib.put.poznan.pl/  
   Maintained by Poznan University of Technology.

2. **Cross-reference:** The same topology and distances appear in hundreds of published
   papers on RMSA and QoT-aware planning. Representative examples:
   - Christodoulopoulos et al., "Elastic Bandwidth Allocation in Flexible OFDM-based
     Optical Networks," J. Lightwave Technol., vol. 29, no. 9, 2011.
   - Chatterjee et al., "Routing and Spectrum Assignment Approaches in Elastic Optical
     Networks: A Comprehensive Survey," IEEE Comm. Surveys & Tutorials, vol. 17, no. 3, 2015.

## Notes

- Node numbering is 0-indexed (0–13).
- All links are bidirectional (each JSON entry represents both directions).
- City labels are approximate US city associations; the network is a logical topology.
- Some papers use slightly different distances (±100 km) due to different geographic
  mapping methods. The values here are the most commonly reproduced set.
