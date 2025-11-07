# Guard to prevent re-running
if(ESP32_CONFIGURED)
  return()
endif()
set(ESP32_CONFIGURED TRUE CACHE INTERNAL "")

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

# Check if ESP32 core is already installed
execute_process(
  COMMAND arduino-cli core list
  OUTPUT_VARIABLE CORE_LIST
  ERROR_QUIET
)

string(FIND "${CORE_LIST}" "esp32:esp32" ESP32_INSTALLED)

if(ESP32_INSTALLED EQUAL -1)
  message(STATUS "Installing ESP32 core...")

  # Add ESP32 board manager URL if not already added
  execute_process(
    COMMAND arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    OUTPUT_QUIET
    ERROR_QUIET
  )

  # Update index again after adding ESP32 URL
  execute_process(
    COMMAND arduino-cli core update-index
    OUTPUT_QUIET
    ERROR_QUIET
  )

  # Install ESP32 core
  execute_process(
    COMMAND arduino-cli core install esp32:esp32
    RESULT_VARIABLE ESP32_INSTALL_RESULT
    OUTPUT_VARIABLE ESP32_INSTALL_OUTPUT
    ERROR_VARIABLE ESP32_INSTALL_ERROR
  )

  if(NOT ESP32_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install ESP32 core: ${ESP32_INSTALL_ERROR}")
  else()
    message(STATUS "ESP32 core installed successfully")
  endif()
else()
  message(STATUS "ESP32 core is already installed")
endif()

# Display installed cores
message(STATUS "Installed Arduino cores:")
execute_process(
  COMMAND arduino-cli core list
  OUTPUT_VARIABLE INSTALLED_CORES
)
message(STATUS "${INSTALLED_CORES}")

# Set useful variables for the rest of your CMake project
set(ARDUINO_CLI_EXECUTABLE arduino-cli)
set(ESP32_CORE_INSTALLED TRUE)