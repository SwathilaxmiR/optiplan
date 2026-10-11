# Toy 4-Node Topology — Source

This topology is hand-crafted for OptiPlan's unit test suite.

## Structure

```
A ---100 km--- B
|               |
150 km        200 km
|               |
C ---250 km--- D
```

- **4 nodes:** A, B, C, D
- **4 bidirectional links** with distances chosen to make hand computation easy:
  - A↔B: 100 km (slightly over 1 span at 80 km default)
  - A↔C: 150 km (about 2 spans)
  - B↔D: 200 km (about 3 spans)
  - C↔D: 250 km (about 4 spans)

## Purpose

- ASE/OSNR hand calculations
- Simple k-shortest-path verification
- Spectrum assignment basic tests
- QoT model smoke tests
