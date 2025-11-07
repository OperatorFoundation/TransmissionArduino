# Guard to prevent re-running
if(RP2040_CONFIGURED)
  return()
endif()
set(RP2040_CONFIGURED TRUE CACHE INTERNAL "")

# Update the core index
message(STATUS "Updating arduino-cli core index...")
execute_process(
  COMMAND arduino-cli core update-index
  RESULT_VARIABLE UPDATE_RESULT
  OUTPUT_VARIABLE UPDATE_OUTPUT
  ERROR_VARIABLE UPDATE_ERROR
)

if(NOT UPDATE_RESULT EQUAL 0)
  message(WARNING "Failed to update core index: ${UPDATE_ERROR}")
endif()

# Check if RP2040 core is already installed
execute_process(
  COMMAND arduino-cli core list
  OUTPUT_VARIABLE CORE_LIST
  ERROR_QUIET
)

string(FIND "${CORE_LIST}" "rp2040:rp2040" RP2040_INSTALLED)

if(RP2040_INSTALLED EQUAL -1)
  message(STATUS "Installing RP2040 core...")

  # Add RP2040 board manager URL if not already added
  execute_process(
    COMMAND arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-rp2040/gh-pages/package_rp2040_index.json
    OUTPUT_QUIET
    ERROR_QUIET
  )

  # Update index again after adding RP2040 URL
  execute_process(
    COMMAND arduino-cli core update-index
    OUTPUT_QUIET
    ERROR_QUIET
  )

  # Install RP2040 core
  execute_process(
    COMMAND arduino-cli core install rp2040:rp2040
    RESULT_VARIABLE RP2040_INSTALL_RESULT
    OUTPUT_VARIABLE RP2040_INSTALL_OUTPUT
    ERROR_VARIABLE RP2040_INSTALL_ERROR
  )

  if(NOT RP2040_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install RP2040 core: ${RP2040_INSTALL_ERROR}")
  else()
    message(STATUS "RP2040 core installed successfully")
  endif()
else()
  message(STATUS "RP2040 core is already installed")
endif()

# Display installed cores
message(STATUS "Installed Arduino cores:")
execute_process(
  COMMAND arduino-cli core list
  OUTPUT_VARIABLE INSTALLED_CORES
)
message(STATUS "${INSTALLED_CORES}")

# Set useful variables for the rest of your CMake project
set(RP2040_CORE_INSTALLED TRUE)