# Programmable Flip-Flop (pff)

A highly configurable, synthesizable Verilog HDL flip-flop supporting dynamic run-time operation modes (rising/falling edge, synchronous/asynchronous control, active-high/low polarities, and enable configuration) accompanied by a comprehensive **cocotb** verification suite.

---

## Features

- **Edge Selectable (`mode_i[4]`)**: Configurable for rising edge (`posedge`) or falling edge (`negedge`) triggering.
- **Operation Type (`mode_i[3]`)**: Supports both Asynchronous and Synchronous reset/set operations.
- **Control Polarity (`mode_i[2]`)**: Configurable active-high or active-low reset/set control signal (`clr_i`).
- **Control Function (`mode_i[1]`)**: Supports either a **Reset** function (clears output to `0`) or a **Set** function (presets output to `1`).
- **Clock Enable Polarity (`mode_i[0]`)**: Configurable active-high or active-low clock enable (`en_i`).

---

## Port Interface

| Port Name | Direction | Width | Description |
| :--- | :--- | :--- | :--- |
| `clk_i` | Input | 1-bit | Clock input signal |
| `d_i` | Input | 1-bit | Data input |
| `clr_i` | Input | 1-bit | Asynchronous / Synchronous Reset or Set control signal |
| `en_i` | Input | 1-bit | Clock enable signal |
| `mode_i` | Input | 5-bit | Configuration mode vector |
| `q_o` | Output | 1-bit | Flip-flop data output |

---

## Mode Configuration Map (`mode_i[4:0]`)

| Bit | Function | `0` | `1` |
| :--- | :--- | :--- | :--- |
| **`mode_i[4]`** | Clock Edge | Rising Edge (`posedge`) | Falling Edge (`negedge`) |
| **`mode_i[3]`** | Operation Type | Asynchronous | Synchronous |
| **`mode_i[2]`** | Control Polarity | Active-Low (`clr_i` active when `0`) | Active-High (`clr_i` active when `1`) |
| **`mode_i[1]`** | Control Function | Reset (`q_o` $\rightarrow$ `0`) | Set (`q_o` $\rightarrow$ `1`) |
| **`mode_i[0]`** | Enable Polarity | Active-Low (`en_i` active when `0`) | Active-High (`en_i` active when `1`) |

---

## Verification with cocotb

This repository includes a Python-based testbench using **cocotb** to verify multiple modes of operation (such as asynchronous active-high reset and synchronous falling-edge set).

### Prerequisites
Make sure you have Python, `cocotb`, and a Verilog simulator (such as Icarus Verilog) installed:
```bash
pip install cocotb
sudo apt-get install iverilog  # Or equivalent for your OS
