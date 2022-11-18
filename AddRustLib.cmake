function(
  add_rust_lib
  crate_name
  crate_root
  include_path
)

# TODO:
# * arch selection
# * debug/release

# TODO: replace - with _
set(LIB_NAME ${crate_name})

# TODO: replace - with _
set(LIB_FILENAME lib${crate_name}.a)

# TODO: Select based on zephyr build settings
set(CARGO_PROFILE debug)
set(CARGO_TARGET thumbv7em-none-eabi)

set(LIB_PATH ${crate_root}/target/${CARGO_TARGET}/${CARGO_PROFILE}/${LIB_FILENAME})

include(CMakePrintHelpers)

add_library(${LIB_NAME} INTERFACE ${LIB_FILENAME})
target_include_directories(${LIB_NAME} INTERFACE ${include_path})
target_link_libraries(${LIB_NAME} INTERFACE ${LIB_PATH})
target_link_libraries(app PRIVATE ${LIB_NAME})

add_custom_command(
  OUTPUT ${LIB_FILENAME}
  COMMAND cargo build --target ${CARGO_TARGET}
  WORKING_DIRECTORY ${crate_root}
  COMMENT "Building rust library '${crate_name}' target='${CARGO_TARGET}' args='${CARGO_ARGS}'"
  VERBATIM
)

endfunction()