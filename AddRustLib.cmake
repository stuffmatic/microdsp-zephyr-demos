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

add_custom_command(
  OUTPUT ${LIB_FILENAME}
  BYPRODUCTS ${LIB_FILENAME}
  COMMAND cargo build --target ${CARGO_TARGET} ${CARGO_ARGS}
  WORKING_DIRECTORY ${crate_root}
  COMMENT "Building rust library '${crate_name}' target='${CARGO_TARGET}' profile='${CARGO_PROFILE}' args='${CARGO_ARGS}'"
  VERBATIM
)

add_library(${LIB_NAME} INTERFACE ${LIB_FILENAME})
target_include_directories(${LIB_NAME} INTERFACE ${include_path})
target_link_libraries(${LIB_NAME} INTERFACE ${LIB_PATH})
target_link_libraries(app PRIVATE ${LIB_NAME})


endfunction()