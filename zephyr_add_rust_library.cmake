include(ExternalProject)

# A helper function for adding a dependency on a
# Rust crate to be compiled to a static library and
# linked into the app. By deafult, the crate is compiled
# in release mode unless CONFIG_DEBUG_OPTIMIZATIONS is set.
# To override this behavior, use the CARGO_PROFILE argument.
#
# Arguments:
#
# CRATE_NAME        - Required. The name of the crate as specified
#                     in Cargo.toml
# CRATE_DIR         - Required. Absolute path to the crate's root folder
# CRATE_HEADER_DIR  - Required. Absolute path to a folder containing the
#                     C wrapper header(s). This path gets added to the header
#                     search paths. If for example header.h is placed under
#                     include_path, it can be included from the app code
#                     like this: #include <header.h>.
# EXTRA_CARGO_ARGS  - Optional. Extra arguments to pass to cargo. Can for
#                     example be used to select features or change the log level.
# CARGO_PROFILE     - Optional. Name of cargo build profile to use. One of
#                     "release" or "debug". Overrides the automatically
#                     selected profile.
function(zephyr_add_rust_library)
cmake_parse_arguments(
    PARSED_ARGS
    ""
    "CRATE_NAME;CRATE_DIR;CRATE_HEADER_DIR;CARGO_PROFILE"
    "EXTRA_CARGO_ARGS"
    ${ARGN}
)

# Check required arguments
if(NOT DEFINED PARSED_ARGS_CRATE_NAME)
  message(FATAL_ERROR "Missing CRATE_NAME")
endif()
if(NOT DEFINED PARSED_ARGS_CRATE_DIR)
  message(FATAL_ERROR "Missing CRATE_DIR")
endif()
if(NOT DEFINED PARSED_ARGS_CRATE_HEADER_DIR)
  message(FATAL_ERROR "Missing CRATE_HEADER_DIR")
endif()

# Store parsed arguments
set(CRATE_NAME ${PARSED_ARGS_CRATE_NAME})
set(CRATE_DIR ${PARSED_ARGS_CRATE_DIR})
set(CRATE_HEADER_DIR ${PARSED_ARGS_CRATE_HEADER_DIR})
set(CARGO_PROFILE ${PARSED_ARGS_CARGO_PROFILE})
set(EXTRA_CARGO_ARGS ${PARSED_ARGS_EXTRA_CARGO_ARGS})

# - gets replaced with _ in build artefacts created by cargo
string(REPLACE "-" "_" SANITIZED_LIB_NAME ${CRATE_NAME})

# cargo adds the prefix lib to static libraries
set(LIB_FILENAME lib${SANITIZED_LIB_NAME}.a)

# Select a cargo build profile, if not already set
if (NOT DEFINED CARGO_PROFILE)
  set(CARGO_PROFILE release)
  if (DEFINED CONFIG_DEBUG_OPTIMIZATIONS)
    set(CARGO_PROFILE debug)
  endif()
endif()

# Set args to pass to cargo build depending on build profile
if (${CARGO_PROFILE} STREQUAL "debug")
  set(CARGO_PROFILE_ARGS "")
elseif(${CARGO_PROFILE} STREQUAL "release")
  set(CARGO_PROFILE_ARGS --release)
else()
  message(FATAL_ERROR "Invalid cargo profile ${CARGO_PROFILE}. Must be debug or release.")
endif()

# Select a cargo build target suitable for the current CPU
if(DEFINED CONFIG_CPU_CORTEX_M0)
  set(CARGO_TARGET thumbv6m-none-eabi)
elseif(DEFINED CONFIG_CPU_CORTEX_M3)
  set(CARGO_TARGET thumbv7em-none-eabi)
elseif(DEFINED CONFIG_CPU_CORTEX_M33)
  if(DEFINED CONFIG_FPU)
  set(CARGO_TARGET thumbv8m.main-none-eabihf)
  else()
  set(CARGO_TARGET thumbv8m.main-none-eabih)
  endif()
elseif(DEFINED CONFIG_CPU_CORTEX_M4 OR DEFINED CONFIG_CPU_CORTEX_M7)
  if(DEFINED CONFIG_FPU)
  set(CARGO_TARGET thumbv7em-none-eabihf)
  else()
  set(CARGO_TARGET thumbv7em-none-eabi)
  endif()
else()
  # TODO: Support more CPU types
  message(FATAL_ERROR "Failed to set cargo build target for CPU type")
endif()

# Output cargo build artefacts to the cmake build folder
set(CARGO_TARGET_DIR ${CMAKE_BINARY_DIR}/rust_crates/${CRATE_NAME})

# Add an external project for building the rust library.
# Inspired by the external_lib zephyr example.
set(LIB_PATH ${CARGO_TARGET_DIR}/${CARGO_TARGET}/${CARGO_PROFILE}/${LIB_FILENAME})
set(EXT_PROJ_NAME rust_ext_proj_${SANITIZED_LIB_NAME})

ExternalProject_Add(
  ${EXT_PROJ_NAME}
  BINARY_DIR ${CRATE_DIR}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND CARGO_TARGET_DIR=${CARGO_TARGET_DIR} cargo rustc --crate-type staticlib --target ${CARGO_TARGET} ${CARGO_PROFILE_ARGS} ${EXTRA_CARGO_ARGS}
  INSTALL_COMMAND ""
  SOURCE_DIR ${CRATE_DIR}
  BUILD_BYPRODUCTS ${LIB_PATH}
  BUILD_ALWAYS true # Always run cargo build. Reduces to a no-op if the built library is up to date.
  COMMENT "Building rust library '${CRATE_NAME}' target='${CARGO_TARGET}' profile='${CARGO_PROFILE}' args='${CARGO_PROFILE_ARGS}'"
)
add_library(${LIB_FILENAME} STATIC IMPORTED GLOBAL)
add_dependencies(${LIB_FILENAME} ${EXT_PROJ_NAME})
set_target_properties(${LIB_FILENAME} PROPERTIES IMPORTED_LOCATION ${LIB_PATH})

# Add C headers dir to include directories
set_target_properties(${LIB_FILENAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CRATE_HEADER_DIR})

# Link static library. --allow-multiple-definition
# is needed when linking to multiple rust libs that may
# contain the same rust compiler builtins.
target_link_libraries(app PUBLIC ${LIB_FILENAME} -Wl,--allow-multiple-definition)

endfunction()