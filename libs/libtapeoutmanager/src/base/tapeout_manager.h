#ifndef TAPEOUT_MANAGER_H
#define TAPEOUT_MANAGER_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "arch_io_constraints.h"
#include "boot_config.h"
#include "command_exit_codes.h"
#include "physical_types.h"
#include "tapeout_manager_fwd.h"
#include "vtr_geometry.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class TapeoutManager {
 public: /* -- Types -- */
  /* The type of step names */
  enum class e_step_name {
    PRE_ARCH = 0,
    ARCH,
    BITSTREAM,
    NETLIST,
    PD,
    SO,
    DV,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  enum class e_substep_name {
    ARCH_INIT = 0,
    ARCH_DATA,
    ARCH_INIT_MODULE_NAMES,
    ARCH_INIT_FABRIC_KEY,
    ARCH_IO_COORD,
    ARCH_PIN_TABLE,
    GOLDEN_BITSTREAM,
    NETLIST_GEN,
    NETLIST_OPT,
    NETLIST_COARSE_ANALYZE,
    NETLIST_COARSE_FLOORPLAN,
    NETLIST_QOR,
    PRE_ARCH_LUT_ANALYZE,
    DV_SETUP,
    DV_RTL_PRECONFIG,
    DV_GL_PRECONFIG,
    DV_PL_PRECONFIG,
    DV_GL_POWER,
    DV_GL_STA,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  enum class e_arch_file_type {
    VPR_ARCH = 0,
    OPENFPGA_ARCH,
    OPENFPGA_GNTWK_ARCH,
    OPENFPGA_CKBUF_LOC_MAP, /* Clock buffer locatioin map file */
    DESIGN_VAR,
    OPENFPGA_SIM_SETTING,       /* Openfpga default simulation setting file */
    OPENFPGA_CKBUF_MAP,         /* Openfpga clock buffer cell map file */
    OPENFPGA_BITSTREAM_SETTING, /* Openfpga default bitstream setting file */
    OPENFPGA_TILE_CONFIG,       /* Openfpga tile configuration file */
    OPENFPGA_PCF_XML, /* Openfpga XML-based PCF, this is a dummy version which is applied when
                         benchmark does not have one */
    VPR_SDC, /* Vpr sdc file, this is a dummy version which is applied when benchmark does not have
                one */
    OPENFPGA_PCF_CONFIG,      /* Openfpga PCF custom command configuration */
    OPENFPGA_BOUNDARY_TIMING, /* Openfpga PCF custom command configuration */
    OPENFPGA_PCF,             /* Openfpga  PCF, this is a dummy version which is applied when
                                     benchmark does not have one */
    VPR_CONSTR,               /* VPR constraint files */
    NUM_TYPES                 /* Count the number of types, also this is an invalid type */
  };
  /* The type of status codes */
  enum class e_run_status {
    SUCCESS = 0,
    FAILURE,
    REQ_REFRESH, /* Require a new run. Applicable to be back-annotation step */
    FRESH,       /* The run is never executed before */
    NUM_TYPES    /* Count the number of types, also this is an invalid type */
  };
  /* Type of third party IP files */
  enum class e_ip_file_type {
    RTL = 0,
    GL,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  /* Format of third party IP files */
  enum class e_ip_file_format {
    AUTO = 0, /* Auto detect from file name */
    VERILOG,
    SYSTEM_VERILOG,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  /* Type of corner tags */
  enum class e_corner_tag_type {
    SCLIB = 0,
    SPEF,
    ARCH_TIMING,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  /* Type of qorso tasks */
  enum class e_qorso_task_type {
    LEAKAGE = 0,
    CONFIG_POWER,
    TIMING,
    DYNAMIC_POWER,
    STA,
    TIMING_GCLK,
    TIMING_GRST,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  /* Type of scripts */
  enum class e_script_type {
    OPENFPGA_SCRIPT_BITGEN = 0, /* Openfpga shell script for bitstream generation */
    OPENFPGA_SCRIPT_GCLKBITGEN, /* Openfpga shell script for gclk sweeping bitstream generation */
    OPENFPGA_SCRIPT_GRSTBITGEN, /* Openfpga shell script for grst sweeping bitstream generation */
    OPENFPGA_SCRIPT_BASEBITGEN, /* Openfpga shell script for bitstream generation */
    OPENFPGA_SCRIPT_DEVICEDATA, /* Openfpga shell script for device data cache */
    OPENFPGA_SCRIPT_REFMODULENAMES, /* Openfpga shell script for reference module names generation
                                     */
    OPENFPGA_SCRIPT_REFFKEY,        /* Openfpga shell script for reference fabric key generation */
    OPENFPGA_SCRIPT_IOCOORD,        /* Openfpga shell script for io coordinate generation */
    OPENFPGA_SCRIPT_FABRIC,         /* Openfpga shell script for netlist generation */
    OPENFPGA_SCRIPT_PRECONFIGTB,    /* Openfpga shell script for netlist generation */
    OPENFPGA_SCRIPT_FULLTB,         /* Openfpga shell script for netlist generation */
    YOSYS_SCRIPT_SYNTH,             /* Yosys script for synthesis */
    YOSYS_SCRIPT_REWRITE,           /* Yosys script for synthesis rewriting */
    YOSYS_SCRIPT_SWEEPK,            /* Yosys script to sweep LUT size */
    NUM_TYPES                       /* Count the number of types, also this is an invalid type */
  };
  /* Type of simulators */
  enum class e_simulator_type {
    VCS = 0,
    ICARUS,
    MODELSIM,
    QUESTA,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };

  /* A structure to contain constraint info for a benchmark-run-corner combination */
  struct BenchmarkStatus {
    std::vector<std::string> constraint_files;
  };

  // Structure to hold custom pin utilization data
  struct CustomPinUtilization {
    double input_pin_util;
    double output_pin_util;
  };

  /* Structure for constraint keys */
  struct BenchmarkStatusKey {
    std::string benchmark;
    std::string run;
    std::string corner;

    // Comparison operator for use as map key
    bool operator<(const BenchmarkStatusKey& other) const {
      if (benchmark != other.benchmark) {
        return benchmark < other.benchmark;
      }
      if (run != other.run) {
        return run < other.run;
      }
      return corner < other.corner;
    }
  };

  // Structure to hold tile-specific pin utilization
  struct TilePinUtilization {
    std::map<std::string, CustomPinUtilization>
      tile_utilizations;  // tile_name -> CustomPinUtilization
  };

  // Nested map to store custom pin utilization: benchmark -> run -> corner -> TilePinUtilization
  std::map<std::string, std::map<std::string, std::map<std::string, TilePinUtilization>>>
    custom_pin_utilization_nested_;

 private: /* -- Types for internal use only -- */
  /* A structure for sub-step status */
  struct SubstepStatus {
    std::map<e_substep_name, e_run_status> status;
  };
  /* A structure to contain all the information for a step run */
  struct RunInfo {
    e_run_status status = e_run_status::FRESH;
    bool frozen = false;
    void clear() {
      status = e_run_status::FRESH;
      frozen = false;
    }
  };

 public: /* -- Constructors -- */
  TapeoutManager();
  /* Not allow to copy and copy assignment operator. Keep the copy unique */
  TapeoutManager(const TapeoutManager&) = delete;
  void operator=(const TapeoutManager&) = delete;

 public: /* -- Aggregators -- */
  std::vector<e_step_name> dev_steps() const;
  std::array<e_step_name, size_t(e_step_name::NUM_TYPES)> steps() const;
  std::vector<std::string> step_runs(const e_step_name& step) const;
  std::vector<e_arch_file_type> arch_file_types() const;
  std::vector<e_arch_file_type> openfpga_setting_file_types() const;
  std::vector<e_script_type> openfpga_script_file_types() const;
  std::vector<e_script_type> yosys_script_file_types() const;
  /* Constraint-related aggregators (from ProjectManager) */
  std::vector<std::string> constraint_benchmarks() const;
  std::vector<std::string> constraint_runs(const std::string& benchmark) const;
  std::vector<std::string> constraint_corners(const std::string& benchmark,
                                              const std::string& run) const;

 public: /* -- Accessors -- */
  std::string file() const;
  std::string name() const;
  std::string family() const;
  std::string path() const;
  std::string booting_root() const;
  std::string arch_gen_bin_path(const bool& keep_reserved_word = false) const;

  std::string author() const;
  std::string date_created() const;
  std::string date_last_modified() const;
  std::string tool_version() const;

  std::string pdk_config(const bool& keep_reserved_word = false) const;
  std::vector<std::string> boundary_timing_corners() const;
  std::string boundary_timing_file(const std::string& corner,
                                   const bool& keep_reserved_word = false) const;
  std::vector<std::string> benchmark_configs(const bool& keep_reserved_word = false) const;
  std::string default_benchmark_name() const;
  std::string default4clk_sweep_benchmark_name() const;
  std::string default4rst_sweep_benchmark_name() const;
  std::vector<std::string> disabled_benchmarks() const;
  std::vector<std::string> disabled_corners() const;
  std::string arch_template_file(const bool& keep_reserved_word = false,
                                 const bool& check_valid = true) const;
  std::string dp_arch_file(const e_arch_file_type& f_type,
                           const bool& keep_reserved_word = false) const;
  std::vector<std::string> dp_arch_files(const bool& keep_reserved_word = false) const;
  std::string ultimate_arch_file(const e_arch_file_type& f_type,
                                 const bool& keep_reserved_word = false) const;
  std::vector<std::string> ultimate_arch_files(const bool& keep_reserved_word = false) const;
  std::string openfpga_setting_file(const e_arch_file_type& f_type,
                                    const bool& keep_reserved_word = false) const;
  std::string openfpga_script_file(const e_script_type& f_type,
                                   const bool& keep_reserved_word = false) const;
  std::string yosys_script_file(const e_script_type& f_type,
                                const bool& keep_reserved_word = false) const;
  size_t route_chan_width() const;
  size_t ccff_max_fanout(const std::string& pdk_name) const;
  std::string netlist_makeup_key_root_path(const bool& keep_reserved_word = false) const;
  std::vector<std::string> synth_pdk_names() const;
  std::string synth_sdc_root_path(const std::string& pdk_name,
                                  const bool& keep_reserved_word = false) const;
  std::string synth_strategy_file(const std::string& pdk_name,
                                  const bool& keep_reserved_word = false) const;
  std::vector<std::string> floorplan_pdk_names() const;
  std::string floorplan_strategy_file(const std::string& pdk_name,
                                      const bool& keep_reserved_word = false) const;
  std::vector<std::string> netlist_flags_pdk_names() const;
  std::vector<std::string> netlist_flags(const std::string& pdk_name) const;
  std::string flag_value(const std::string& pdk_name, const std::string& name) const;
  std::vector<IpFileId> ip_files() const;
  e_ip_file_type ip_file_type(const IpFileId& f_id) const;
  std::string ip_file_type_str(const IpFileId& f_id) const;
  e_ip_file_format ip_file_format(const IpFileId& f_id) const;
  std::string ip_file_format_str(const IpFileId& f_id) const;
  std::string ip_file(const IpFileId& f_id, const bool& keep_reserved_word = false) const;
  std::vector<IpFileId> ip_files_by_type(const e_ip_file_type& f_type) const;
  std::string openfpga_stats_conf_file(const bool& keep_reserved_word = false) const;
  std::string openfpga_error_conf_file(const bool& keep_reserved_word = false) const;
  std::string yosys_error_conf_file(const bool& keep_reserved_word = false) const;
  std::string default_corner() const;
  std::vector<std::string> corner_tags() const;
  std::string corner_tag(const std::string& name, const e_corner_tag_type& type,
                         const bool& keep_reserved_word = false) const;
  std::vector<std::string> user_defined_corner_tags() const;
  std::string user_defined_corner_tag(const std::string& name, const e_corner_tag_type& type,
                                      const bool& keep_reserved_word = false) const;
  bool is_user_defined_corner(const std::string& name) const;
  bool is_builtin_corner(const std::string& name) const;
  bool is_disabled_corner(const std::string& corner_name, const bool& verbose = false) const;
  std::vector<std::string> builtin_corner_tags() const;
  std::string builtin_corner_tag(const std::string& name, const e_corner_tag_type& type,
                                 const bool& keep_reserved_word = false) const;
  /* Get the arch timing file path with a given name of corner. Most commonly used */
  std::string arch_timing_file(const std::string& name,
                               const bool& keep_reserved_word = false) const;
  /* Get the arch timing file path with a given type. Only used by I/O functions */
  std::string arch_timing_template_file() const;
  std::string arch_timing_template_pdk() const;
  std::vector<std::string> arch_timing_file_pdks() const;
  std::vector<std::string> arch_timing_file_types(const std::string& pdk_name) const;
  std::string arch_timing_file_by_type(const std::string& pdk_name, const std::string& type,
                                       const bool& keep_reserved_word = false) const;
  /* Print out all the valid corners */
  void show_corners() const;

  std::vector<std::string> qorso_tasks() const;
  std::vector<std::string> qorso_task_pdk_names(const std::string& t_name) const;
  e_qorso_task_type qorso_task_type(const std::string& t_name) const;
  std::string qorso_task_type_str(const std::string& t_name) const;
  std::string qorso_task_config(const std::string& t_name, const std::string& pdk_name,
                                const bool& keep_reserved_word = false) const;

  std::string step_latest_run(const e_step_name& step) const;
  e_run_status step_run_status(const e_step_name& step, const std::string& run) const;
  std::string step_run_status_str(const e_step_name& step, const std::string& run) const;
  bool step_run_frozen(const e_step_name& step, const std::string& run) const;
  std::vector<e_step_name> step_run_deps(const e_step_name& step, const std::string& run) const;
  /* The previous run id which the current step run is depending on */
  std::string step_run_dep_run(const e_step_name& step, const std::string& run,
                               const e_step_name& dep) const;
  std::vector<e_substep_name> substeps(const e_step_name& step, const std::string& run) const;
  e_run_status substep_status(const e_step_name& step, const std::string& run,
                              const e_substep_name& substep) const;
  std::string substep_status_str(const e_step_name& step, const std::string& run,
                                 const e_substep_name& substep) const;
  /* Sub-tool executable paths */
  std::string openfpga_bin_path() const;
  std::string python_bin_path() const;
  std::string python_site_package_path() const;
  std::string python_cocotb_config_path() const;
  std::string libpython_loc_path() const;
  std::string fabric_key_assistant_bin_path() const;
  std::string pin_table_gen_bin_path() const;
  std::string netlist_makeup_bin_path() const;
  std::string ccff_makeup_bin_path() const;
  std::string synth_bin_path() const;
  std::string frontend_bin_path() const;
  std::string frontend_cell_lib_path() const;
  std::string qorso_bin_path() const;
  std::string bus_group_gen_bin_path() const;
  std::string cocotb_util_bin_path() const;
  std::string cocotb_convert_makefile_application_simulation() const;
  std::string cocotb_force_bitstream_bin_path() const;
  std::string cocotb_makefile_include_file_path() const;
  std::string benchmark_simulation_cocotb_makefile_include_file_path() const;

  const ArchIoConstraints& arch_io_constraints() const;

  bool is_pdk_natively_supported_in_synth(const std::string& pdk_name) const;
  bool is_pdk_natively_supported_in_floorplan(const std::string& pdk_name) const;
  std::vector<std::string> ccff_makeup_pdk_names() const;
  std::string ccff_makeup_buf_config_file(const std::string& pdk_name,
                                          const bool& full_path = true) const;
  bool pdk_require_netlist_makeup_config(const std::string& pdk_name) const;
  std::string netlist_makeup_config_file(const std::string& pdk_name,
                                         const bool& full_path = true) const;
  int io_num(const std::string& run, const e_side& curr_side,
             const ArchIoConstraints::e_io_direction& io_type) const;
  size_t core_height(const std::string& run) const;
  size_t core_width(const std::string& run) const;
  bool sync_dp_dimension(const std::string& run) const;
  size_t num_clocks(const std::string& run) const;
  size_t num_prog_clocks(const std::string& run) const;
  size_t num_config_regions(const std::string& run) const;
  size_t num_resets(const std::string& run) const;
  std::vector<std::string> arch_gen_file_types() const;
  std::string raw_arch_gen_bin_path(const std::string& type_str) const;
  std::string default_simulator() const;
  /* Constraint-related accessors (from ProjectManager) */
  std::vector<std::string> get_vpr_constraint_files(const std::string& benchmark_name,
                                                    const std::string& run,
                                                    const std::string& corner) const;

  std::vector<std::string> get_aggregated_constraint_files(
    const std::string& benchmark_name, const std::string& run, const std::string& corner,
    const std::string& internal_clock_constraint_file) const;

 public: /* -- Mutators -- */
  void set_file(const std::string& val);
  void set_name(const std::string& val);
  void set_family(const std::string& val);
  bool set_path(const std::string& val);
  bool set_arch_gen_bin_path(const std::string& boot_type, const std::string& val);
  void set_author(const std::string& val);
  void set_date_created(const std::string& val);
  void set_date_last_modified(const std::string& val);
  void set_tool_version(const std::string& val);
  /* Automatically modify the time stamp as needed */
  void update_date_last_modified();

  void set_kw_root_path(const std::string& val);
  void add_boundary_timing_file(const std::string& corner, const std::string& val);
  void set_pdk_config(const std::string& val);
  bool add_benchmark_config(const std::string& val);
  void set_default_benchmark_name(const std::string& val);
  void set_default4clk_sweep_benchmark_name(const std::string& val);
  void set_default4rst_sweep_benchmark_name(const std::string& val);
  int add_disabled_benchmark(const std::string& val);
  int add_disabled_corner(const std::string& val, const bool& verbose = false);
  void set_arch_template_file(const std::string& val);
  bool set_arch_timing_template_pdk(const std::string& val);
  bool set_arch_timing_template_file(const std::string& val);
  bool add_arch_timing_file(const std::string& pdk_name, const std::string& type,
                            const std::string& val);
  bool add_dp_arch_file(const std::string& ftype_str, const std::string& fname);
  bool add_ultimate_arch_file(const std::string& ftype_str, const std::string& fname);
  bool add_openfpga_setting_file(const std::string& ftype_str, const std::string& fname);
  bool add_openfpga_script_file(const std::string& ftype_str, const std::string& fname);
  bool add_yosys_script_file(const std::string& ftype_str, const std::string& fname);
  void set_route_chan_width(const size_t& val);
  bool set_ccff_max_fanout(const std::string& pdk_name, const size_t& val);
  void set_netlist_makeup_key_root_path(const std::string& val);
  bool add_netlist_makeup_config_file(const std::string& pdk_name, const std::string& val);
  bool add_ccff_makeup_buf_config_file(const std::string& pdk_name, const std::string& val);
  bool set_synth_sdc_root_path(const std::string& pdk_name, const std::string& val);
  bool set_synth_strategy_file(const std::string& pdk_name, const std::string& val);
  bool set_floorplan_strategy_file(const std::string& pdk_name, const std::string& val);
  void set_pdk_name(const std::string& val);
  void set_default_corner(const std::string& val);
  bool add_corner_tag(const std::string& name, const std::string& sclib_tag,
                      const std::string& spef_tag);
  void set_default_builtin_corner(const std::string& pdk_name, const std::string& val);
  bool add_builtin_corner_tag(const std::string& pdk_name, const std::string& name,
                              const std::string& sclib_tag, const std::string& spef_tag);
  bool add_ip_file(const std::string& ftype_str, const std::string& fname,
                   const std::string& ffmt_str);
  bool add_qorso_task(const std::string& name, const std::string& type);
  bool add_qorso_task_config_by_pdk(const std::string& name, const std::string& pdk_name,
                                    const std::string& config_file);
  bool add_netlist_flag(const std::string& pdk_name, const std::string& name,
                        const std::string& val);

  bool add_step_run(const e_step_name& step, const std::string& run);
  bool add_step_run(const std::string& step_str, const std::string& run);
  bool set_step_latest_run(const e_step_name& step, const std::string& run);
  bool set_step_latest_run(const std::string& step_str, const std::string& run);
  bool set_step_run_status(const e_step_name& step, const std::string& run,
                           const e_run_status& status);
  bool set_step_run_status(const std::string& step_str, const std::string& run,
                           const std::string& status);
  bool set_step_run_frozen(const std::string& step_str, const std::string& run, const bool& val);
  bool add_step_run_dep(const std::string& step_str, const std::string& run,
                        const std::string& dep_str, const std::string& dep_run);
  bool add_step_run_dep(const e_step_name& step, const std::string& run, const e_step_name& dep,
                        const std::string& dep_run);
  bool set_substep_status(const e_step_name& step, const std::string& run,
                          const e_substep_name& substep, const e_run_status& status);
  bool set_substep_status(const std::string& step, const std::string& run,
                          const std::string& substep, const std::string& status);
  /* Check the status of all the substeps for a given run of a step. And update the status of the
   * step. For example, if all the substeps are success, the step should be a success
   */
  bool check_and_update_step_status(const e_step_name& step, const std::string& run,
                                    const bool& verbose);
  /* Add I/O settings */
  bool add_num_io_per_tile_setting(const std::string& side_str, const std::string& func,
                                   const std::string& var_name, const std::string& msb_var_name,
                                   const size_t& max);
  bool add_io_type_setting(const std::string& side_str, const std::string& var_name);
  /* Load constants from boot configuration */
  int load_boot_config(const BootConfig& boot_cfg);
  void clear();
  void clear_status();
  void set_openfpga_stats_conf_file(const std::string& val);
  void set_openfpga_error_conf_file(const std::string& val);
  void set_yosys_error_conf_file(const std::string& val);

  bool set_step_run_status_including_substeps(const e_step_name& step, const std::string& run,
                                              const e_run_status& status);
  /* Modify architecture parameters */
  int remove_io(const std::string& run, const std::string& side_str, const bool& verbose);
  int set_io_num(const std::string& run, const std::string& side_str, const std::string& type_str,
                 const size_t& io_num, const bool& no_validation, const bool& verbose);
  /* The no validation flag is used by parser only. Validation will be done once parsing is finished
   */
  int set_sync_dp_dimension(const std::string& run, const bool& val, const bool& no_validation,
                            const bool& verbose);
  int set_core_dimension(const std::string& run, const size_t& height, const size_t& width,
                         const bool& no_validation, const bool& verbose);
  int set_num_clocks(const std::string& run, const size_t& num_clks, const bool& no_validation,
                     const bool& verbose);
  int set_num_prog_clocks(const std::string& run, const size_t& num_pclks,
                          const bool& no_validation, const bool& verbose);
  int set_num_config_regions(const std::string& run, const size_t& num_regions,
                             const bool& no_validation, const bool& verbose);
  int set_num_resets(const std::string& run, const size_t& num_rsts, const bool& no_validation,
                     const bool& verbose);
  std::vector<BenchmarkStatusKey> get_all_constraint_keys() const;

  bool check_clock_buffer_constraints(const std::string& benchmark_name, const std::string& run,
                                      const std::string& corner) const;

  bool empty() const;
  /* Constraint-related mutators (from ProjectManager) */
  int add_vpr_constraints_to_benchmark(const std::string& benchmark_name, const std::string& run,
                                       const std::string& corner, const std::string& file);
  /* Pin utilization-related mutators */
  int add_custom_pin_utilization_to_benchmark(const std::string& benchmark_name,
                                              const std::string& run, const std::string& corner,
                                              const std::string& tile_name, double input_pin_util,
                                              double output_pin_util);

  // Method to get custom pin utilization data for a specific tile
  std::pair<bool, CustomPinUtilization> get_custom_pin_utilization_for_tile(
    const std::string& benchmark_name, const std::string& run, const std::string& corner,
    const std::string& tile_name) const;

  // Method to get all custom pin utilization data for a benchmark/run/corner
  std::vector<std::pair<std::string, CustomPinUtilization>>
  get_custom_pin_utilization_for_benchmark_run_corner(const std::string& benchmark_name,
                                                      const std::string& run,
                                                      const std::string& corner) const;

  // Method to get target external pin utilization string for VPR
  std::string get_target_ext_pin_utilization_str(const std::string& benchmark_name,
                                                 const std::string& run,
                                                 const std::string& corner) const;

  // Method to get default pin utilization for a tile from the architecture template
  std::pair<float, float> get_default_pin_utilization(const std::string& tile_name) const;
  // Method to get all default pin utilization values from the architecture template
  std::string get_all_default_pin_utilization_str() const;
  float get_default_input_pin_utilization(const std::string& tile_name) const;
  float get_default_output_pin_utilization(const std::string& tile_name) const;

  // Method to get the complete custom pin utilization nested map
  const auto& get_custom_pin_utilization_nested() const { return custom_pin_utilization_nested_; }

  void clear_constraint_data();

  /* Public API for accessing benchmark status */
  BenchmarkStatus& find_benchmark_status(const std::string& benchmark_name, const std::string& run,
                                         const std::string& corner);
  const BenchmarkStatus& find_benchmark_status(const std::string& benchmark_name,
                                               const std::string& run,
                                               const std::string& corner) const;

 public: /* -- Validator -- */
  bool valid_run(const e_step_name& step, const std::string& run) const;
  /* No self-dependency! */
  bool valid_dep(const e_step_name& step, const e_step_name& dep) const;
  bool valid_step_name(const e_step_name& step) const;
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate() const;
  bool validate_pdk_config_file() const;
  bool validate_boundary_timing_file() const;
  std::string invalid_run() const;
  /* Check if a given corner name is valid (defined or not) */
  bool valid_corner_name(const std::string& val) const;
  /* Check if the corner name is to sweep all the corners */
  bool sweep_corner(const std::string& val) const;

  void check_valid_run(const e_step_name& step, const std::string& run) const;
  bool valid_qorso_task_type(const e_qorso_task_type& type) const;
  bool valid_simulator_type(const e_simulator_type& type) const;

 public: /* Public utility */
  e_step_name str2step_name(const std::string& str, const bool& verbose = false) const;
  std::string step_name2str(const e_step_name& type, const bool& verbose = false) const;
  size_t step_name_str_max_len() const;
  std::string step_name_all2str() const;

  e_step_name next_step(const e_step_name& curr_step) const;
  std::string find_downstream_run_on_step_run(const e_step_name& step, const std::string& run,
                                              const e_step_name& n_step) const;

  e_substep_name str2substep_name(const std::string& str, const bool& verbose = false) const;
  std::string substep_name2str(const e_substep_name& type, const bool& verbose = false) const;
  size_t substep_name_str_max_len() const;
  std::string substep_name_all2str() const;

  e_run_status str2status(const std::string& str, const bool& verbose = false) const;
  std::string status2str(const e_run_status& type, const bool& verbose = false) const;
  size_t status_str_max_len() const;
  std::string status_all2str() const;

  e_arch_file_type str2arch_file_type(const std::string& str, const bool& verbose = false) const;
  std::string arch_file_type2str(const e_arch_file_type& type, const bool& verbose = false) const;
  std::string openfpga_setting_file_type2str(const e_arch_file_type& type,
                                             const bool& verbose = false) const;
  std::string arch_file_type_all2str() const;

  e_script_type str2script_type(const std::string& str, const bool& verbose = false) const;
  std::string script_type2str(const e_script_type& type, const bool& verbose = false) const;
  std::string script_type_all2str() const;
  std::string openfpga_script_type_all2str() const;
  std::string yosys_script_type_all2str() const;

  e_ip_file_type str2ip_file_type(const std::string& str, const bool& verbose = false) const;
  std::string ip_file_type2str(const e_ip_file_type& type, const bool& verbose = false) const;
  std::string ip_file_type_all2str() const;

  e_ip_file_format str2ip_file_format(const std::string& str, const bool& verbose = false) const;
  std::string ip_file_format2str(const e_ip_file_format& type, const bool& verbose = false) const;
  std::string ip_file_format_all2str() const;

  e_corner_tag_type str2corner_tag_type(const std::string& str, const bool& verbose = false) const;
  std::string corner_tag_type2str(const e_corner_tag_type& type, const bool& verbose = false) const;
  std::string corner_tag_type_all2str() const;

  e_qorso_task_type str2qorso_task_type(const std::string& str, const bool& verbose = false) const;
  std::string qorso_task_type2str(const e_qorso_task_type& type, const bool& verbose = false) const;
  std::string qorso_task_type_all2str() const;

  e_simulator_type str2simulator_type(const std::string& str, const bool& verbose = false) const;
  std::string simulator_type2str(const e_simulator_type& type, const bool& verbose = false) const;
  std::string simulator_type_all2str() const;

 private: /* -- Private Validator -- */
  void check_valid() const;
  void check_valid_pdk_config_file() const;
  void check_valid_boundary_timing_file() const;
  void check_valid_arch_file_type(const e_arch_file_type& type) const;
  void check_valid_openfpga_setting_file_type(const e_arch_file_type& type) const;
  void check_valid_script_type(const e_script_type& type) const;
  void check_valid_openfpga_script_type(const e_script_type& type) const;
  void check_valid_yosys_script_type(const e_script_type& type) const;
  void check_valid_ip_file_format(const e_ip_file_format& fmt) const;
  void check_valid_ip_file_type(const e_ip_file_type& type) const;
  void check_valid_corner_tag_type(const e_corner_tag_type& type) const;
  void check_valid_qorso_task_type(const e_qorso_task_type& type) const;
  void check_valid_simulator_type(const e_simulator_type& type) const;
  void check_valid_step(const e_step_name& step) const;
  void check_valid_dep(const e_step_name& step, const e_step_name& dep) const;
  bool valid_arch_file_type(const e_arch_file_type& type) const;
  bool valid_openfpga_setting_file_type(const e_arch_file_type& type) const;
  bool valid_script_type(const e_script_type& type) const;
  bool valid_openfpga_script_type(const e_script_type& type) const;
  bool valid_yosys_script_type(const e_script_type& type) const;
  bool valid_ip_file_type(const e_ip_file_type& type) const;
  bool valid_ip_file_format(const e_ip_file_format& fmt) const;
  bool valid_corner_tag_type(const e_corner_tag_type& type) const;
  bool valid_qorso_task(const std::string& t_name) const;
  bool valid_run_status(const e_run_status& status) const;
  bool valid_substep_name(const e_substep_name& substep) const;
  std::string replace_reserved_keywords(const std::string& val) const;
  bool valid_ip_file_id(const IpFileId& f_id) const;
  std::map<std::string, std::pair<float, float>> get_all_default_pin_utilization_map() const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::string file_;
  std::string name_;
  std::string family_;
  std::string path_;

  /* Reserved keyword which will be applied when generating the path to benchmark and pdk config
   * files . For example, PWD -> getcwd() */
  std::map<std::string, std::string> reserved_keywords_;

  /* Time stamps */
  std::string author_;             /* Author of pdk file */
  std::string date_created_;       /* the date that created for the first time */
  std::string date_last_modified_; /* the date that is lastly modified */
  std::string tool_version_;       /* the version of ArcXpress that creates the project */

  /* Boot type */
  BootConfig::e_boot_type boot_type_;

  /* PDK data */
  std::string pdk_config_;

  /* Boundary timing [corner]->file path */
  std::map<std::string, std::string> boundary_timing_files_;

  /* Benchmark data */
  std::map<std::string, size_t> benchmark_config_;
  std::string default_benchmark_name_;
  std::string default4clk_sweep_benchmark_name_;
  std::string default4rst_sweep_benchmark_name_;
  std::vector<std::string> disabled_benchmarks_;

  /* Arch data */
  std::string arch_template_file_;
  /* Arch generator */
  std::map<BootConfig::e_boot_type, std::string> arch_gen_bin_path_;
  /* Built-in arch timing template. [pdk_name]->[tag]. Apply when the corner is not related to any
   * existing built-in corner */
  std::string arch_timing_template_pdk_;
  std::string arch_timing_template_file_;
  /* Built-in arch timing file per pdk. [pdk_name]->[tag]->[file path] */
  std::map<std::string, std::map<std::string, std::string>> arch_timing_files_;
  std::array<size_t, size_t(e_arch_file_type::NUM_TYPES)> dp_arch_file_cnt_;
  std::array<std::string, size_t(e_arch_file_type::NUM_TYPES)> dp_arch_files_;
  std::array<size_t, size_t(e_arch_file_type::NUM_TYPES)> ultimate_arch_file_cnt_;
  std::array<std::string, size_t(e_arch_file_type::NUM_TYPES)> ultimate_arch_files_;
  std::array<size_t, size_t(e_script_type::NUM_TYPES)> script_file_cnt_;
  std::array<std::string, size_t(e_script_type::NUM_TYPES)> script_files_;
  size_t route_chan_width_;
  std::vector<e_ip_file_type> ip_file_types_;
  std::vector<e_ip_file_format> ip_file_formats_;
  std::vector<std::string> ip_files_;
  /* [pdk][flag_name] = value */
  std::map<std::string, std::map<std::string, std::string>> netlist_flags_;
  std::string openfpga_stats_conf_file_;
  std::string yosys_error_conf_file_;
  std::string openfpga_error_conf_file_;

  /* I/O settings */
  ArchIoConstraints arch_io_constr_;
  std::map<std::string, vtr::Point<size_t>> core_dimension_;
  std::map<std::string, bool> sync_dp_dimension_;
  std::map<std::string, size_t> num_clocks_;
  std::map<std::string, size_t> num_pclocks_;
  std::map<std::string, size_t> num_config_regions_;
  std::map<std::string, size_t> num_resets_;
  std::map<std::string, std::array<std::map<e_side, size_t>,
                                   size_t(ArchIoConstraints::e_io_direction::NUM_TYPES)>>
    num_ios_;

  /* Netlist makeup: TODO: consider to be [pdk]->value */
  std::string netlist_makeup_key_root_path_;
  std::map<std::string, std::string> netlist_makeup_config_file_;
  std::map<std::string, std::string> ccff_makeup_buf_config_file_;
  /* Synthesis [pdk]->value */
  std::map<std::string, std::string> synth_sdc_root_;
  std::map<std::string, std::string> synth_strategy_file_;
  std::map<std::string, size_t> ccff_max_fanout_;

  /* Synthesis [pdk]->value */
  std::map<std::string, std::string> floorplan_strategy_file_;

  /* Qorso */
  std::map<std::string, size_t> qorso_task_name2ids_;
  std::vector<e_qorso_task_type> qorso_task_types_;
  std::vector<std::map<std::string, std::string>> qorso_task_config_;

  /* User-defined Corner map: tag -> [sclib, spef, arch_timing] */
  mutable std::string default_corner_;
  std::vector<std::string> disabled_corners_;
  std::map<std::string, std::array<std::string, size_t(e_corner_tag_type::NUM_TYPES)>> corner_map_;

  /* Built-in corner information: [pdk name] -> [corners] */
  std::string pdk_name_;
  std::map<std::string, size_t> pdk2builtin_corner_ids_;
  std::vector<std::string> builtin_default_corners_;
  std::vector<std::map<std::string, std::array<std::string, size_t(e_corner_tag_type::NUM_TYPES)>>>
    builtin_corner_map_;

  /* Each step has a required name which should be one of the enumertion of e_step_name
   * Each e_step_name has a fixed index according to its type index
   */
  std::array<size_t, size_t(e_step_name::NUM_TYPES)> step_cnt_;
  std::array<std::string, size_t(e_step_name::NUM_TYPES)> step_latest_runs_;
  std::array<std::map<std::string, RunInfo>, size_t(e_step_name::NUM_TYPES)> step_status_;
  std::array<std::map<std::string, SubstepStatus>, size_t(e_step_name::NUM_TYPES)> substep_status_;
  /* [step][run][dep][dep_run]*/
  std::array<std::map<std::string, std::array<std::string, size_t(e_step_name::NUM_TYPES)>>,
             size_t(e_step_name::NUM_TYPES)>
    step_deps_;
  std::map<BenchmarkStatusKey, BenchmarkStatus> benchmark_status_info_;

  /* Constants */

  std::array<std::string, size_t(e_step_name::NUM_TYPES)> STEP_NAME_STRING_;
  std::array<std::string, size_t(e_run_status::NUM_TYPES)> STATUS_STRING_;
  std::array<std::string, size_t(e_arch_file_type::NUM_TYPES)> ARCH_FILE_TYPE_STRING_;
  std::array<std::string, size_t(e_substep_name::NUM_TYPES)> SUBSTEP_NAME_STRING_;
  std::array<std::string, size_t(e_script_type::NUM_TYPES)> SCRIPT_TYPE_STRING_;
  std::array<std::string, size_t(e_ip_file_type::NUM_TYPES)> IP_FILE_TYPE_STRING_;
  std::array<std::string, size_t(e_ip_file_format::NUM_TYPES)> IP_FILE_FORMAT_STRING_;
  std::array<std::string, size_t(e_corner_tag_type::NUM_TYPES)> CORNER_TAG_TYPE_STRING_;
  std::array<std::string, size_t(e_qorso_task_type::NUM_TYPES)> QORSO_TASK_TYPE_STRING_;
  std::array<std::string, size_t(e_simulator_type::NUM_TYPES)> SIMULATOR_TYPE_STRING_;
  std::string KW_ROOT_;
  std::string KW_PWD_;
  std::string KW_PROJHOME_;
  std::string OPENFPGA_BIN_RELPATH_;
  std::string PYTHON_BIN_RELPATH_;
  std::string PYTHON_COCOTB_CONFIG_RELPATH_;
  std::string PYTHON_SITE_PACKAGES_RELPATH_;
  std::string LIBPYTHON_LOC_RELPATH_;
  std::string FKEYASSIST_BIN_RELPATH_;
  std::string PINTABLEGEN_BIN_RELPATH_;
  std::string NETLIST_MAKEUP_BIN_RELPATH_;
  std::string CCFF_MAKEUP_BIN_RELPATH_;
  std::string SYNTH_BIN_RELPATH_;
  std::string FRONTEND_BIN_RELPATH_;
  std::string FRONTEND_CELL_LIB_RELPATH_;
  std::string QORSO_BIN_RELPATH_;
  std::string BGG_BIN_RELPATH_;
  std::string COCOTB_UTIL_BIN_RELPATH_;
  std::string COCOTB_CONVERT_MAKEFILE_APPLICATION_SIMULATION_;
  std::string COCOTB_FORCE_BITSTREAM_BIN_RELPATH_;
  std::string COCOTB_MAKEFILE_INC_FILE_RELPATH_;
  std::string BM_SIM_COCOTB_MAKEFILE_INC_FILE_RELPATH_;
  std::string KW_CORNER_ALL_;

  /* Flag */
  mutable bool is_valid_;
  mutable bool is_valid_pdk_config_file_;
  mutable bool is_valid_boundary_timing_file_;
};

}  // namespace arcxpress

#endif
