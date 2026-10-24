#pragma once

namespace arcxpress {
constexpr const char* XML_OPENFPGA_ARCH_ROOT_NODE_NAME = "openfpga_architecture";
/* Techlib section */
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_NODE_NAME = "technology_library";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_NODE_NAME = "device_library";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NODE_NAME = "device_model";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_NODE_NAME = "lib";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_CORNER = "corner";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_REF = "ref";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_PATH = "path";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_NODE_NAME = "design";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_VDD = "vdd";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_PN_RATIO = "pn_ratio";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_PMOS_NODE_NAME = "pmos";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NMOS_NODE_NAME = "nmos";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_CHAN_LENGTH = "chan_length";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_MIN_WIDTH = "min_width";
constexpr const char* XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_VARIATION = "variation";
constexpr const char* XML_OPENFPGA_ARCH_VARIATION_LIB_NODE_NAME = "variation_library";
constexpr const char* XML_OPENFPGA_ARCH_VARIATION_NODE_NAME = "variation";
constexpr const char* XML_OPENFPGA_ARCH_VARIATION_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_VARIATION_ATTR_ABS_DEVIATION = "abs_deviation";
constexpr const char* XML_OPENFPGA_ARCH_VARIATION_ATTR_NUM_SIGMA = "num_sigma";
/* Circuit lib */
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_LIB_NODE_NAME = "circuit_library";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME = "circuit_model";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX = "prefix";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT = "is_default";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_DUMP_STRUCTURAL_VERILOG =
  "dump_structural_verilog";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST = "verilog_netlist";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME = "design_technology";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_STRUCTURE = "structure";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_ADD_CONST_INPUT =
  "add_const_input";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_CONST_INPUT_VAL =
  "const_input_val";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TOPOLOGY = "topology";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_SIZE = "size";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_FRAC_LUT = "fracturable_lut";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_NODE_NAME = "device_technology";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_ATTR_DEVICE_MODEL_NAME =
  "device_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_NODE_NAME = "pass_gate_logic";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LSPGL_NODE_NAME =
  "last_stage_pass_gate_logic";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LSPGL_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME = "port";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX = "prefix";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME = "lib_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE = "size";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_MODE_SELECT = "mode_select";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL = "default_val";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL = "is_global";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_IO = "is_io";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_DATA_IO = "is_data_io";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_CONFIG_ENABLE =
  "is_config_enable";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_PROG = "is_prog";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_RESET = "is_reset";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TRI_STATE_MAP = "tri_state_map";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LUT_FRAC_LEVEL = "lut_frac_level";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LUT_OUTPUT_MASK = "lut_output_mask";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_NUM_LEVEL = "num_level";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_F_PER_STAGE = "f_per_stage";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME = "delay_matrix";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT = "in_port";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT = "out_port";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME = "input_buffer";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME = "output_buffer";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INPUT_BUFFER_NODE_NAME =
  "lut_input_buffer";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INPUT_INVERTER_NODE_NAME =
  "lut_input_inverter";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INTER_BUFFER_NODE_NAME =
  "lut_intermediate_buffer";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST = "exist";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_LOCATION_MAP = "location_map";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_NODE_NAME = "wire_param";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_MODEL_TYPE = "model_type";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_R = "R";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_C = "C";
constexpr const char* XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_NUM_LEVEL = "num_level";
/* Configuration protocol */
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_NODE_NAME = "configuration_protocol";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_NODE_NAME = "organization";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_NUM_REGIONS = "num_regions";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_NODE_NAME =
  "programming_clock";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_ATTR_PORT = "port";
constexpr const char* XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_ATTR_CCFF_HEAD_INDICES =
  "ccff_head_indices";
/* Connection block and Switch block */
constexpr const char* XML_OPENFPGA_ARCH_CONNECTION_BLOCK_NODE_NAME = "connection_block";
constexpr const char* XML_OPENFPGA_ARCH_SWITCH_BLOCK_NODE_NAME = "switch_block";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_NODE_NAME = "switch";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
/* Routing segments */
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_SEGMENT_NODE_NAME = "routing_segment";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_NODE_NAME = "segment";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
/* Direct connection */
constexpr const char* XML_OPENFPGA_ARCH_DIRECT_CONNECTION_NODE_NAME = "direct_connection";
constexpr const char* XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_NODE_NAME = "direct";
constexpr const char* XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_TYPE = "type";
constexpr const char* XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
/* Tile annotation */
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_NODE_NAME = "tile_annotations";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_NODE_NAME =
  "physical_equivalent_site";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_TILE =
  "tile";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_SUBTILE =
  "subtile";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_SITE =
  "site";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_NODE_NAME = "global_port";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_IS_CLOCK = "is_clock";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_IS_RESET = "is_reset";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_DEFAULT_VAL =
  "default_val";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_CLOCK_ARCH_TREE_NAME =
  "clock_arch_tree_name";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_NODE_NAME = "tile";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_PORT = "port";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_X = "x";
constexpr const char* XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_Y = "y";
/* Pb type annotation */
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_NODE_NAME = "pb_type_annotations";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_NODE_NAME = "pb_type";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_MODE_NAME =
  "physical_mode_name";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_MODE_BITS = "mode_bits";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_NAME =
  "physical_pb_type_name";
constexpr const char*
  XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_INDEX_FACTOR =
    "physical_pb_type_index_factor";
constexpr const char*
  XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_INDEX_OFFSET =
    "physical_pb_type_index_offset";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_NODE_NAME = "port";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_PHYSICAL_MODE_PORT =
  "physical_mode_port";
constexpr const char*
  XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_PHYSICAL_MODE_PIN_ROTATE_OFFSET =
    "physical_mode_pin_rotate_offset";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_NODE_NAME = "interconnect";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_ATTR_NAME = "name";
constexpr const char* XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_ATTR_CIRCUIT_MODEL_NAME =
  "circuit_model_name";
}  // namespace arcxpress
