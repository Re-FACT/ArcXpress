.. _datasheet_mosofa_overview:

Overview
--------

*Modern Open-Source eFPGA Architecture*  (MOSOFA) is the modernized version of the *Skywater Open-source FPGA* (`SOFA <https://github.com/lnis-uofu/SOFA>`_)  

MOSOFA carries major upgrades in architecture and EDA driven by OpenFPGA, when compared to the SOFA FPGA architectures.

The technical highlights include:

- Programmable I/O blocks support fractuable operating modes
  - Registerable inputs 
  - Registerable outputs

- Each *Configurable Logic Block* (CLB) contains 8 *Fracturable Logic Elements* (FLEs), each of which can operate in different modes:

  - Single 4-input *Look-Up Table* (LUT) and programmable Flip-Flops (FFs)
  - Dual 3-input *Look-Up Table* (LUT) and programmable FFs
  - Soft adder and programmable FFs
  - Shift registers

- Each CLB can be driven by multiple clocks and resets

- Each flip-flop can be configured in more than 16 operating modes. To be specific,
 
  - Active-high/low and Asynchronous/synchronous reset/set
  - Rising/Falling edge triggered
  - With/without write enable

- Configuration chains contain multiple heads and tails, each of which is controlled by independent clocks
