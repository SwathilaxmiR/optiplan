# Nobel-EU 28-Node Topology — Source

## Description

The **Nobel-EU** network topology represents a pan-European core optical transport backbone
comprising **28 nodes** (major European cities) and **41 bidirectional links**. It was
originally designed as part of the European Union Information Society Technologies (IST)
project **NOBEL** (*Next generation Optical networks for Broadband European Leadership*).

## Data Source

- **Repository:** SNDlib (Survivable Network Design Library)
- **Official URL:** https://sndlib.put.poznan.pl/nobel-eu.overview.action
- **Download Link:** https://sndlib.put.poznan.pl/download/sndlib-networks-xml/nobel-eu.xml
- **Citation:** S. Orlowski, M. Pióro, A. Tomaszewski, and R. Wessäly, "SNDlib 1.0–Survivable
  Network Design Library," *Networks*, vol. 55, no. 3, pp. 276–286, 2010.

## Distance Calculation

The link lengths ($L_e$) in `topology.json` are computed using the great-circle Haversine
formula from the official node GPS coordinates provided in SNDlib ($R_{\text{Earth}} = 6371.0\text{ km}$):
$$d = 2 R \arcsin\left(\sqrt{\sin^2\left(\frac{\Delta\phi}{2}\right) + \cos\phi_1 \cos\phi_2 \sin^2\left(\frac{\Delta\lambda}{2}\right)}\right)$$
where $\phi$ is latitude and $\lambda$ is longitude.

## Network Properties
- Nodes: 28
- Bidirectional Links: 41 (82 unidirectional links)
- Average Node Degree: $2 \times 41 / 28 \approx 2.93$
- Diameter / Spanning: Pan-European (Glasgow to Athens, Lisbon/Madrid to Warsaw/Stockholm).
