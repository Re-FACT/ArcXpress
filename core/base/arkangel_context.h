#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#undef min
#undef max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef IN
#undef ABSOLUTE
#ifdef isdigit
#undef isdigit
#endif
#endif

#include "benchmark_manager.h"
#include "boot_config.h"
#ifdef ARKANGEL_LICENSED_BUILD
#include "license_manager.h"
#endif
#include "openfpga_context.h"
#include "pdk_manager.h"
#include "tapeout_manager.h"
/********************************************************************
 * This file includes the declaration of the date structure
 * ArkangelContext, which is used for data exchange between
 * different modules in Arkangel shell environment
 *
 * If a command of Arkangel needs to exchange data with other commands,
 * it must use this data structure to access/mutate.
 * In such case, you must add data structures to this context
 *
 * Note:
 * Please respect to the following rules when using the context
 * 1. This data structure will be created only once in the main() function
 *    The data structure is design to be large and contain all the
 *    data structure required by each module of OpenFPGA core engine.
 *    Do NOT create or duplicate in your own module!
 * 2. Be clear in your mind if you want to access/mutate the data inside
 *    Read-only data should be accessed by
        const ArkangelContext&
 *    Mutate should use reference
 *      ArkangelContext&
 * 3. Please keep the definition of ArkangelContext short
 *    Do put ONLY well-modularized data structure under this root.
 * 4. We build this data structure based on the Context from VPR
 *    which does NOT allow users to copy the internal members
 *    This is due to that the data structures in the Arkangel context
 *    are typically big in terms of memory
 *******************************************************************/
class ArkangelContext : public OpenfpgaContext {
 public: /* Public accessors */
  const arkangel::BootConfig& boot_config() const { return boot_config_; }
  const arkangel::TapeoutManager& tapeout_manager() const { return tapeout_manager_; }
  const arkangel::BenchmarkManager& benchmark_manager() const { return benchmark_manager_; }
  const arkangel::PdkManager& pdk_manager() const { return pdk_manager_; }
#ifdef ARKANGEL_LICENSED_BUILD
  arkangel::LicenseManager& license_manager() { return license_manager_; }
#endif
  bool is_tapeout_created() const { return tapeout_created_; }

 public: /* Public mutators */
  arkangel::BootConfig& mutable_boot_config() { return boot_config_; }
  arkangel::TapeoutManager& mutable_tapeout_manager() { return tapeout_manager_; }
  arkangel::BenchmarkManager& mutable_benchmark_manager() { return benchmark_manager_; }
  arkangel::PdkManager& mutable_pdk_manager() { return pdk_manager_; }
#ifdef ARKANGEL_LICENSED_BUILD
  arkangel::LicenseManager& mutable_license_manager() { return license_manager_; }
#endif
  void set_tapeout_created() { tapeout_created_ = true; }

 private: /* Internal data */
  /* Project status */
  bool tapeout_created_ = false;
  arkangel::BootConfig boot_config_;
  arkangel::TapeoutManager tapeout_manager_;
  arkangel::BenchmarkManager benchmark_manager_;
  arkangel::PdkManager pdk_manager_;
#ifdef ARKANGEL_LICENSED_BUILD
  arkangel::LicenseManager license_manager_;
#endif
};
