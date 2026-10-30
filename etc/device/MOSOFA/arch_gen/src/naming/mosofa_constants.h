#pragma once

#include <array>

/** Common names for SOFA series. Each device may have difference on models 
 *  To be self-contained and conflict-free, each device should have a dedicated namespace
 */
namespace MOSOFA { // begin namespace MOSOFA

/* Vpr models */
/* Primary inputs */
constexpr const char* PI_BLIF_MODEL = "input";
constexpr const char* PI_CIRCUIT_MODEL = "pinput";
/* Primary outputs */
constexpr const char* PO_BLIF_MODEL = "output";
constexpr const char* PO_CIRCUIT_MODEL = "poutput";
/* Look-up Tables */
constexpr const char* FRAC_LUT4_BLIF_MODEL = "frac_lut4";
constexpr const char* FRAC_LUT4_CIRCUIT_MODEL = "frac_lut4";
/* Adders */
constexpr const char* ADDER_BLIF_MODEL = "soft_adder";
constexpr const char* PHYSICAL_ADDER_BLIF_MODEL = "adder";
constexpr const char* PHYSICAL_ADDER_CIRCUIT_MODEL = "adder";

/* Tile and subtile names */
constexpr const char* TILE_IO_PREFIX = "io";
constexpr const char* SUBTILE_PI_PREFIX = "pi";
constexpr const char* SUBTILE_PO_PREFIX = "po";

/* Operating modes: I/O pb_type names */
constexpr const char* PI_NAME = "inpad";
constexpr const char* PO_NAME = "outpad";
/* CLB pb_type names */
/* Physical modes */
constexpr const char* PHYSICAL_FRAC_LOGIC_NAME = "frac_logic";
constexpr const char* PHYSICAL_REGULAR_SCFF_NAME = "scff";
/* Operating modes */
constexpr const char* OPERATING_LUT3INTER_NAME = "lut3inter";
constexpr const char* OPERATING_BLE3_NAME = "ble3";
constexpr const char* OPERATING_BLE4_NAME = "ble4";
constexpr const char* OPERATING_ALE_NAME = "ale";
constexpr const char* OPERATING_SREG_NAME = "shift_register";
constexpr const char* OPERATING_FLE_NAME = "fle";
constexpr const char* OPERATING_FABRIC_NAME = "fabric";
constexpr const char* OPERATING_BUILTIN_LATCH_NAME = "latch";
constexpr const char* OPERATING_BLE3_FF_NAME = "v_ble3_ff";
constexpr const char* OPERATING_BLE4_FF_NAME = "v_ble4_ff";
constexpr const char* OPERATING_ALE_FF_NAME = "v_ale_ff";
constexpr const char* OPERATING_SREG_FF_NAME = "v_sreg_ff";

constexpr std::array<const char*, 3> OPERATING_REGULAR_SCFF_NAMES = {
  OPERATING_BLE3_FF_NAME, OPERATING_BLE4_FF_NAME, OPERATING_SREG_FF_NAME};
constexpr std::array<const char*, 4> ALL_REGULAR_SCFF_NAMES = {
  PHYSICAL_REGULAR_SCFF_NAME, OPERATING_BLE3_FF_NAME, OPERATING_BLE4_FF_NAME,
  OPERATING_SREG_FF_NAME};
constexpr std::array<const char*, 8> ALL_OPERATING_SCFF_NAMES = {
  OPERATING_BLE3_FF_NAME, OPERATING_BLE4_FF_NAME, OPERATING_SREG_FF_NAME};

/* Circuit model */
constexpr const char* PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME = "pb_mux_highload";
constexpr const char* PB_MUX_NO_CONST_INPUT_CIRCUIT_MODEL_NAME = "pb_mux_no_const_input";

}  // namespace alkaid
