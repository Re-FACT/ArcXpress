#ifndef ARKANGEL_CONSTANTS_H
#define ARKANGEL_CONSTANTS_H
/********************************************************************
 * This file includes constants for arkangel
 *******************************************************************/

#include <string>

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Constants
 *******************************************************************/

/* Device families */
constexpr const char* DEVICE_FAMILY_NAME_1000 = "MX1000";
constexpr const char* DEVICE_FAMILY_NAME_1500 = "MX1500";
constexpr const char* DEVICE_FAMILY_NAME_200 = "MX200";
constexpr const char* DEVICE_FAMILY_NAME_2000 = "MX2000";
constexpr const char* DEVICE_FAMILY_NAME_2500 = "MX2500";

/* Device serials */
/* Naming rule: Product Class + Lut Input Number + Arch Code + Product Status
 *     where,
 *         Product Class:
 *             P - PSOC
 *             E - Embedded FPGA IP
 *             F - FPGA IC
 *         Lut Number (K):
 *             1 - 1K-Look Up Table logic BLE
 *             4 - 4K-Look Up Table logic BLE
 *             6 - 6K-Look Up Table logic BLE
 *             20 - 20K-Look Up Table logic BLE
 *         Arch Code:
 *             A# - Alkaid + Iteration number
 *             AS# - Alkaid Small + Iteration number
 *             ASL# - Alkaid Small Lite + Iteration number
 *             N# - Neo + Iteration number
 *             K# - Kira + Iteration number
 *         Product Status:
 *             MU# - Mock Up + Iteration number
 *             TC# - Test Chip + Iteration number
 *             ES# - Engineering Sample + Iteration number
 *             RP# - Risk Production + Iteration number
 *             MP or none - Mass Production
 */
constexpr const char* DEVICE_SEARIAL_IP_4LUT_ARCH_1_TC_1 = "E4A1TC1";
constexpr const char* DEVICE_SEARIAL_FC_4LUT_ARCH_1_TC_1 = "F4A1TC1";
constexpr const char* DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1 = "E6A2TC1";
constexpr const char* DEVICE_SEARIAL_PC_6LUT_ARCH_2_TC_1 = "P6A2TC1";
constexpr const char* DEVICE_SEARIAL_IP_6LUT_ARCH_3_TC_1 = "E6A3MT1";
constexpr const char* DEVICE_SEARIAL_FC_6LUT_ARCH_3_TC_1 = "F6A3TC1";
constexpr const char* DEVICE_SEARIAL_IP_6LUT_ARCH_4_MU_1 = "E6A4MU1";
constexpr const char* DEVICE_SEARIAL_IP_1LUT_ARCH_L1_MU_1 = "E15AL1MU1";
constexpr const char* DEVICE_SEARIAL_IP_20LUT_ARCH_T1_MU_1 = "E200AT1MU1";

/* Package */
/* Naming rule: Pacakge Code + IC Pin Num + Temparature Qualification
 *     where,
 *         Package Code:
 *             FG - FCGBA, Flip Chip Ball Grid Array
 *             TQ - TQFP, Thin Quad Flat Pack
 *             LQ - LQFP, Low-profile Quad Flat Pack
 *             QN - QFN, Quad Flatpack No Lead
 *             DN - DFN, Dual Flatpack No Lead
 *         IC Pin Num:
 *             Number of IC pins
 *         Temparture Qualification:
 *             C - Customerial
 *             I - Industrial
 *             V - Vehicle
 *
 */
constexpr const char* DEVICE_PACKAGE_SG256A_C = "SG256AC";
constexpr const char* DEVICE_PACKAGE_SG121A_C = "SG121AC";
constexpr const char* DEVICE_PACKAGE_NULL = "FABRIC";

/* Delimiter */
constexpr const char* DEVICE_SEARIAL_DELIMITER = "-";

/* Devices */
/* Naming rule: Device Family + Serial Code + Package Code*/
constexpr int DEVICE_SEARIAL_SECTION_NUM = 3;
const std::string DEVICE_MX200_E4A1TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_4LUT_ARCH_1_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // 20220430 MPW1 EFPGA IP
const std::string DEVICE_MX200_F4A1TC1_SG121AC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_FC_4LUT_ARCH_1_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_SG121A_C);  // 20220430 MPW1 FPGA IC
const std::string DEVICE_MX200_E6A2TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // 20221109 MPW2 IP
const std::string DEVICE_MX200_P6A2TC1_SG256A_C =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_PC_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_SG256A_C);  // 20221109 MPW2 PSOC IC
const std::string DEVICE_MX200_E6A3MT1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_3_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // MPW3 20230701 MPW3 EFPGA IP
const std::string DEVICE_MX200_F6A3TC1_SG256A_C =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_FC_6LUT_ARCH_3_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_SG256A_C);  // MPW3 202310?? MPW3 FPGA IC
const std::string DEVICE_MX200_E6A4MU1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_4_MU_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // MPW4 20231201 MPW4 EFPGA IP MOCK
const std::string DEVICE_MX200_E15AL1MU1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_1LUT_ARCH_L1_MU_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // 1.5K AlkAidS Lite Pe-PNR mock
const std::string DEVICE_MX200_E200AT1MU1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_200) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_20LUT_ARCH_T1_MU_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // 20K AlkAidT Pe-Tapeout mock
const std::string DEVICE_MX1000_E6A2TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_1000) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // tool evaluation
const std::string DEVICE_MX1500_E6A2TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_1500) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // tool evaluation
const std::string DEVICE_MX2000_E6A2TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_2000) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // tool evaluation
const std::string DEVICE_MX2500_E6A2TC1_FABRIC =
  std::string(DEVICE_FAMILY_NAME_2500) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_SEARIAL_IP_6LUT_ARCH_2_TC_1) + std::string(DEVICE_SEARIAL_DELIMITER) +
  std::string(DEVICE_PACKAGE_NULL);  // tool evaluation

constexpr const char* DEVICE_DIR = "etc/device/";
constexpr const char* UTILS_DIR = "etc/utils/";
constexpr const char* VPR_ARCH_FILE_NAME = "arch/vpr_arch.xml";
constexpr const char* PLACEMENT_DELAY_LOOKUP_FILE_NAME = "arch/placement_delay_lookup.bin";
constexpr const char* ROUTER_LOOKAHEAD_FILE_NAME = "arch/router_lookahead.bin";
constexpr const char* RR_GRAPH_FILE_NAME = "arch/rr_graph.bin";
constexpr const char* OPENFPGA_ARCH_FILE_NAME = "arch/openfpga_arch.xml";
constexpr const char* OPENFPGA_GLOBAL_NETWORK_FILE_NAME = "arch/openfpga_global_network.xml";
constexpr const char* OPENFPGA_SIM_SETTING_FILE_NAME = "util/openfpga_sim_setting.xml";
constexpr const char* FABRIC_KEY_FILE_NAME = "arch/fabric_key.xml";
constexpr const char* MEMORY_ADDRESS_MAP_FILE_NAME = "arch/memory_address_map.xml";
constexpr const char* REPACK_DESIGN_CONSTRAINT_FILE_NAME =
  "util/repack_design_constraints_dummy.xml";
constexpr const char* PIN_TABLE_FILE_NAME = "util/efpga_io_pin_table.csv";
constexpr const char* IO_MAP_FILE_NAME = "util/io_coordinate.xml";
constexpr const char* PCF_FILE_NAME = "util/pcf_dummy.pcf";
constexpr const char* IVERILOG_DEFAULT_STRATEGY_FILE_NAME = "strategy/iverilog_strategy.xml";
constexpr const char* VPR_DEFAULT_STRATEGY_FILE_NAME = "strategy/vpr_strategy.xml";
constexpr const char* YOSYS_DEFAULT_STRATEGY_FILE_NAME = "strategy/yosys_strategy.xml";
constexpr const char* YOSYS_DEFAULT_TECH_FILE_PATH = "synth";
constexpr const char* LUT_SIZE = "5";
constexpr const char* MULT_TO_DSP_NAME = "mul2dsp";
constexpr const char* DSP_NAME = "mult_12x10_map";
constexpr const char* BITGEN_DEFAULT_STRATEGY_FILE_NAME = "strategy/bitgen_strategy.xml";
constexpr const char* BITGEN_DEFAULT_GROUP_TILE_FILE_NAME = "util/tile_config.xml";

/* bit stream reorder */
constexpr const char* BITSTREAM_REORDER_SCRIPT_DIR = "bitstream_reorder/scripts/";
constexpr const char* BITSTREAM_REORDER_SCRIPT_FILE = "bitstream_reorder.py";
constexpr const char* BITSTREAM_REORDER_DISTRIBUTION_OPTION = "--bitstream_distribution";
constexpr const char* BITSTREAM_REORDER_INPUT_OPTION = "--input_bitstream";
constexpr const char* BITSTREAM_REORDER_OUTPUT_OPTION = "--output_bitstream";
constexpr const char* BITSTREAM_REORDER_TILE_BITSEQ_OPTION = "--tile_bitseq";
constexpr const char* BITSTREAM_REORDER_TOP_CHAIN_OPTION = "--top_chain";
constexpr const char* BITSTREAM_REORDER_TILE_CHAINS_OPTION = "--tile_chains";
constexpr const char* BITSTREAM_REORDER_DISTRIBUTION_FILE = "bitstream_distribution.xml";
constexpr const char* BITSTREAM_REORDER_TOP_CHAIN_FILE = "fpga_top_chain.yaml";
constexpr const char* BITSTREAM_REORDER_SCAN_MAPPING_WILDCARD_FILE_NAME = "*.txt";
constexpr const char* BITSTREAM_REORDER_TILE_CHAIN_WILDCARD_FILE_NAME = "*.yaml";
constexpr const char* BITSTREAM_REORDER_VERBOSITY_OPTION = "--verbosity";
constexpr const char* BITSTREAM_REORDER_VERBOSITY_OPTION_VALUE = "0";

/* placement constrain */
constexpr const char* PLACEMENT_CONSTRAIN_SCRIPT_DIR = "placement_constraint/scripts/";
constexpr const char* PLACEMENT_CONSTRAIN_SCRIPT_FILE = "place_constraint.py";
constexpr const char* PLACEMENT_CONSTRAIN_INPUT_FILE_OPTION = "--input_file";
constexpr const char* PLACEMENT_CONSTRAIN_OUTPUT_FILE_OPTION = "--output_file";
constexpr const char* PLACEMENT_CONSTRAIN_CONSTRAINT_SETTING_OPTION = "--constraint_setting";
constexpr const char* PLACEMENT_CONSTRAIN_RELAX_UNCONSTRAINED_BLOCKS_OPTION =
  "--relax_unconstrained_blocks";
constexpr const char* PLACEMENT_CONSTRAIN_REG_EXP_NAME_MATCH_OPTION = "--reg_exp_name_match";

/* sdc device delay */
constexpr const char* SDC_DEVICE_DELAY_SCRIPT_DIR = "sdc_device_delay/scripts/";
constexpr const char* SDC_DEVICE_DELAY_SCRIPT_FILE = "sdc_device_delay.py";
constexpr const char* SDC_DEVICE_DELAY_INPUT_SDC_FILE_OPTION = "--input_sdc_file";
constexpr const char* SDC_DEVICE_DELAY_INPUT_PCF_FILE_OPTION = "--input_pcf_file";
constexpr const char* SDC_DEVICE_DELAY_INPUT_DEVICE_DELAY_FILE_OPTION = "--input_device_delay_file";
constexpr const char* SDC_DEVICE_DELAY_OUTPUT_SDC_FILE_OPTION = "--output_sdc_file";

/* route sanity check */
constexpr const char* ROUTE_SANITY_CHECK_SCRIPT_DIR = "route_sanity_check/scripts/";
constexpr const char* ROUTE_SANITY_CHECK_SCRIPT_FILE = "route_sanity_check.py";
constexpr const char* ROUTE_SANITY_CHECK_INPUT_PROJECT_FILE_OPTION = "--input_project_file";
constexpr const char* ROUTE_SANITY_CHECK_INPUT_ROUTE_FILE_OPTION = "--input_route_file";

/* convert subckt to latch */
constexpr const char* CONVERT_SUBCKT_SCRIPT_DIR = "power_estimation/scripts/";
constexpr const char* CONVERT_SUBCKT_SCRIPT_FILE = "convert_ntlist.py";
constexpr const char* BRAM_DSP_INFO_YAML_FILE = "bram_dsp_info_map.yaml";
constexpr const char* CONVERT_SUBCKT_INPUT_FILE_OPTION = "--blif_file";
constexpr const char* CONVERT_SUBCKT_OUTPUT_FILE_OPTION = "--output_file";
constexpr const char* BRAM_DSP_INFO_YAML_OPTION = "--info_yaml";

/* convert subckt to latch */
constexpr const char* FIX_FORMAT_SCRIPT_DIR = "power_estimation/scripts/";
constexpr const char* FIX_FORMAT_SCRIPT_FILE = "fix_latch_formate.py";
constexpr const char* FIX_FORMAT_INPUT_FILE_OPTION = "--input_file";
constexpr const char* FIX_FORMAT_OUTPUT_FILE_OPTION = "--output_file";
constexpr const char* FIX_FORMAT_CLOCK_OPTION = "--clock";

/* restore net name in ace.act */
constexpr const char* RESTORE_NET_NAME_SCRIPT_DIR = "power_estimation/scripts/";
constexpr const char* RESTORE_NET_NAME_FILE = "restore_name.py";
constexpr const char* RESTORE_NET_NAME_INPUT_YOSYS_FILE_OPTION = "--input_yosys_file";
constexpr const char* RESTORE_NET_NAME_INPUT_ACE_FILE_OPTION = "--input_ace_file";
constexpr const char* RESTORE_NET_NAME_ACE_ACT_FILE_OPTION = "--ace_activity_file";
constexpr const char* RESTORE_NET_NAME_OUTPUT_FILE_OPTION = "--output_activity";

/*parse verilog file */
constexpr const char* PARSE_VERILOG_DIR = "memory_initialization/";
constexpr const char* PARSE_VERILOG_SCRIPT_FILE = "parse_verilog.py";
constexpr const char* PARSE_VERILOG_PSOUCE_FILE_OPTION = "--verilog_file";

/* bit generation for a device */
constexpr const char* BITGEN_SETTING_DIR = "bitgen/";
constexpr const char* SCAN_MAPPING_DIR = "scan_mapping/";
constexpr const char* TILE_CHAIN_DIR = "tile_chain/";
constexpr const char* TOP_CHAIN_DIR = "top_chain/";
constexpr const char* MIF_DIR = "memory_initialization/";

/* column name in pin table */
constexpr const char* COLUMN_NAME_EXTERNAL_PIN = "Ball ID";
constexpr const char* COLUMN_NAME_EFPGA_PORT = "eFPGA Port Name";
constexpr const char* COLUMN_NAME_DIRECTION = "Direction";
constexpr const char* COLUMN_NAME_EFPGA_PIN = "eFPGA Pin Name";
constexpr const char* COLUMN_NAME_COORD_X = "eFPGA Pin Coordinate X";
constexpr const char* COLUMN_NAME_COORD_Y = "eFPGA Pin Coordinate Y";
constexpr const char* COLUMN_NAME_COORD_Z = "eFPGA Pin Coordinate Z";

/* transform bus port */
constexpr const char* TRANSFORM_BUS_PORT_SCRIPT_DIR = "transform_bus_port/scripts/";
constexpr const char* TRANSFORM_BUS_PORT_SCRIPT_FILE = "transform_bus_port.py";

/* power estimation */
constexpr const char* POWER_ANALYSIS_SCRIPT_DIR = "power_estimation/scripts/";
constexpr const char* POWER_ANALYSIS_SCRIPT_FILE = "power_report.py";

/* This log file is for yosys to write log messages.
   The caller of yosys can analysis the log file to check the result of yosys. */
constexpr const char* YOSYS_LOG_FILE_NAME = "yosys.log";

/* support path corresponding to $ARKANGEL_HOME in file path */
constexpr const char* PATH_VARIABLE_ARKANGEL_HOME = "$ARKANGEL_HOME";

/* file format used by MIF feature*/
constexpr const char* FILE_FORMAT_MIF = "mif";
constexpr const char* FILE_FORMAT_RFMIF = "rfmif";

/* radix */
constexpr const char* RADIX_BIN = "BIN";
constexpr const char* RADIX_HEX = "HEX";

/* default RAM ID width if not specified */
const unsigned int DEFAULT_RAM_ID_WIDTH = 8;

/*routing coefficient to calibrate power data*/
const unsigned int ROUTING_COEFFICIENT = 55;

/* license control */
constexpr const char* LICENSED_BUILD_VERSION = "2025R1";
// this is for some special purpose like we deliver a special build to GUI/contractor
// make sure this signature is same as the one in the .md5 file
// leave it empty if we don't want to build a "fixed" signature release
constexpr const char* LICENSE_FILE_SIGNATURE = "";

/* Python path */
constexpr const char* PYTHON_PATH_ENV_VAR_NAME = "PYTHONPATH";
constexpr const char* SYSTEM_PATH_ENV_VAR_NAME = "PATH";
constexpr const char* LIBPYTHON_PATH_ENV_VAR_NAME = "LIBPYTHON_LOC";

}  // namespace arcxpress

#endif
