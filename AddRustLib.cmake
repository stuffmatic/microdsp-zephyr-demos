include(ExternalProject)

# A helper function for adding a dependency on a
# rust crate that is compiled to a static library and
# linked into the app. By deafult, the create is compiled
# in release mode unless CONFIG_DEBUG_OPTIMIZATIONS is set.
# To override this behavior, set CARGO_PROFILE to 'release'
# or 'debug' before calling the function.
#
# Parameters:
#
# crate_name   - The name of the crate as specified in Cargo.toml
# crate_root   - Absolute path to the crate's root folder
# include_path - Absolute path to a folder containing the C wrapper header(s).
#                This path gets added to the header search paths. If for example
#                header.h is placed under include_path, it can be included
#                from the app code like this: #include <header.h>.
function(
  add_rust_lib
  crate_name
  crate_root
  include_path
)

# - gets replaced with _ in build artefacts created by cargo
string(REPLACE "-" "_" SANITIZED_LIB_NAME ${crate_name})

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
  set(CARGO_ARGS "")
else()
  set(CARGO_ARGS --release)
endif()

# Select a cargo build target suitable for the current CPU
if(DEFINED CONFIG_CPU_CORTEX_M0)
  set(CARGO_TARGET thumbv6m-none-eabi)
elseif(DEFINED CONFIG_CPU_CORTEX_M3)
  set(CARGO_TARGET thumbv7em-none-eabi)
elseif(DEFINED CONFIG_CPU_CORTEX_M33)
  if(DEFINED CPU_HAS_FPU)
  set(CARGO_TARGET thumbv8m.main-none-eabihf)
  else()
  set(CARGO_TARGET thumbv8m.main-none-eabih)
  endif()
elseif(DEFINED CONFIG_CPU_CORTEX_M4 OR DEFINED CONFIG_CPU_CORTEX_M7)
  if(DEFINED CPU_HAS_FPU)
  set(CARGO_TARGET thumbv7em-none-eabihf)
  else()
  set(CARGO_TARGET thumbv7em-none-eabi)
  endif()
else()
  # TODO: Add more CPU types
  message(FATAL_ERROR "Failed to set a cargo build target for CPU type")
endif()

set(LIB_PATH ${crate_root}/target/${CARGO_TARGET}/${CARGO_PROFILE}/${LIB_FILENAME})

# The following is adapted from the external_lib zephyr example
ExternalProject_Add(
  ext_proj
  BINARY_DIR ${crate_root}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND cargo build --target ${CARGO_TARGET} ${CARGO_ARGS}
  INSTALL_COMMAND ""
  SOURCE_DIR ${crate_root}
  BUILD_BYPRODUCTS ${LIB_PATH}
  COMMENT "Building rust library '${crate_name}' target='${CARGO_TARGET}' profile='${CARGO_PROFILE}' args='${CARGO_ARGS}'"
)
add_library(${LIB_FILENAME} STATIC IMPORTED GLOBAL)
add_dependencies(${LIB_FILENAME} ext_proj)
set_target_properties(${LIB_FILENAME} PROPERTIES IMPORTED_LOCATION ${LIB_PATH})
set_target_properties(${LIB_FILENAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${include_path})

target_link_libraries(app PUBLIC ${LIB_FILENAME})

endfunction()