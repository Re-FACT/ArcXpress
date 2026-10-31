# MOSOFA

Modernized Skywater Open-source FPGA Architecture.

Based on the QLSOFA-HD, MOSOFA carries the upgrades in 

- Splitted I/O into PI and PO subtiles. User can customize the number of inputs and outputs separately.
- Fracturable Flip-Flops which support 
  - Active-high/low Asynchronous/Synchronous reset/set
  - Rising edge/Falling edge clocks
- Use little-endian netlists
