include(ExternalProject)

function(
  add_rust_lib
  crate_name
  crate_root
  include_path
)

# TODO: replace - with _
set(LIB_NAME ${crate_name})

# TODO: replace - with _
set(LIB_FILENAME lib${crate_name}.a)

# Select a cargo build profile, if not already set
if (NOT DEFINED CARGO_PROFILE)
  set(CARGO_PROFILE release)
  if (DEFINED CONFIG_NO_OPTIMIZATIONS)
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