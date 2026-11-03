# Programmable Flip-Flop Documentation

## Overview
The `pff` module is a highly configurable Verilog register designed to adapt dynamically based on a 4-bit control vector (`mode_i`). It supports configurable clock edge sensitivity (rising/falling), reset/set synchronization mode (asynchronous/synchronous), control signal polarity (active-high/active-low), and control functionality (reset to `0` or set to `1`).

---

## Interface Signal Description

| Signal Name | Direction | Data Type | Description |
| :--- | :--- | :--- | :--- |
| `clk_i` | Input | `wire` | Master clock input signal. |
| `d_i` | Input | `wire` | Data input signal. |
| `clr_i` | Input | `wire` | Control signal (Reset or Set depending on mode). |
| `en_i` | Input | `wire` | Clock enable signal (`1`: update output, `0`: hold state). |
| `mode_i[3:0]` | Input | `wire [3:0]` | 4-bit configuration mode selection vector. |
| `q_o` | Output | `reg` | Data output register. |

---

## Configuration Mode Mapping (`mode_i[3:0]`)

| Bit Index | Parameter | Value `0` | Value `1` |
| :---: | :--- | :--- | :--- |
| **`mode_i[3]`** | Clock Edge (`clk_edge`) | **Rising Edge** (`posedge`) | **Falling Edge** (`negedge`) |
| **`mode_i[2]`** | Synchronization (`sync_mode`) | **Asynchronous** | **Synchronous** |
| **`mode_i[1]`** | Control Polarity (`polarity`) | **Active-Low** (`clr_i == 0`) | **Active-High** (`clr_i == 1`) |
| **`mode_i[0]`** | Control Function (`func_type`) | **Reset** (`q_o <= 0`) | **Set** (`q_o <= 1`) |

---

## Operating Modes Matrix

| Mode `mode_i[3:0]` | Trigger Edge | Mode Type | Control Polarity | Control Function | Output (`q_o`) on `clr_i` Asserted |
| :---: | :---: | :---: | :---: | :---: | :---: |
| `4'b0000` | Rising | Asynchronous | Active-Low | Reset | `0` |
| `4'b0001` | Rising | Asynchronous | Active-Low | Set | `1` |
| `4'b0010` | Rising | Asynchronous | Active-High | Reset | `0` |
| `4'b0011` | Rising | Asynchronous | Active-High | Set | `1` |
| `4'b0100` | Rising | Synchronous | Active-Low | Reset | `0` |
| `4'b0101` | Rising | Synchronous | Active-Low | Set | `1` |
| `4'b0110` | Rising | Synchronous | Active-High | Reset | `0` |
| `4'b0111` | Rising | Synchronous | Active-High | Set | `1` |
| `4'b1000` | Falling | Asynchronous | Active-Low | Reset | `0` |
| `4'b1001` | Falling | Asynchronous | Active-Low | Set | `1` |
| `4'b1010` | Falling | Asynchronous | Active-High | Reset | `0` |
| `4'b1011` | Falling | Asynchronous | Active-High | Set | `1` |
| `4'b1100` | Falling | Synchronous | Active-Low | Reset | `0` |
| `4'b1101` | Falling | Synchronous | Active-Low | Set | `1` |
| `4'b1110` | Falling | Synchronous | Active-High | Reset | `0` |
| `4'b1111` | Falling | Synchronous | Active-High | Set | `1` |

---

## Operational Priority Logic

1. **Reset / Set Override**: `clr_i` takes priority over data capture (`d_i`).
2. **Clock Enable (`en_i`)**: Sampled only when `clr_i` is inactive.
3. **Hold State**: When `en_i` is low (`0`) and `clr_i` is inactive, `q_o` retains its previous value.

```
                  +-----------------------------------+
                  |            clr_i Active?          |
                  +-----------------------------------+
                                /       \
                               /         \
                             Yes          No
                             /             \
             +-----------------+         +-------------------+
             | q_o <= target_val|        |    en_i Active?   |
             +-----------------+         +-------------------+
                                               /       \
                                              /         \
                                            Yes          No
                                            /             \
                             +-----------------+       +---------------+
                             |  q_o <= d_i     |       | q_o <= q_o    |
                             +-----------------+       +---------------+
```
