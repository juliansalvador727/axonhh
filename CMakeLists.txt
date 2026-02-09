cmake_minimum_required(VERSION 3.16)

project(axonhh
  VERSION 0.1.0
  DESCRIPTION "Dependency-free Hodgkin–Huxley neuron simulator"
  LANGUAGES CXX
)

# --------------------------------------------------
# Global build settings
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Reasonable warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --------------------------------------------------
# Source files
# --------------------------------------------------

set(CORE_SRC
  src/core/types.hpp
)

set(IO_SRC
  src/io/configparser.cpp
  src/io/csvwriter.cpp
)

set(MODEL_SRC
  src/model/hodgkinhuxley.cpp
  src/model/rates.cpp
)

set(NUMERICS_SRC
  src/numerics/euler.cpp
  src/numerics/rk4.cpp
)

set(STIM_SRC
  src/stim/stepstimulus.cpp
  src/stim/pulsestimulus.cpp
)

# --------------------------------------------------
# Main executable
# --------------------------------------------------

add_executable(axonhh
  src/main.cpp
  ${CORE_SRC}
  ${IO_SRC}
  ${MODEL_SRC}
  ${NUMERICS_SRC}
  ${STIM_SRC}
)

target_include_directories(axonhh PRIVATE
  src
)

# --------------------------------------------------
# Testing
# --------------------------------------------------

enable_testing()

add_executable(test_rates
  tests/test_rates.cpp
  ${CORE_SRC}
  ${MODEL_SRC}
)

target_include_directories(test_rates PRIVATE
  src
)

add_test(NAME RatesTest COMMAND test_rates)

add_executable(test_integrators
  tests/test_integrators.cpp
  ${CORE_SRC}
  ${MODEL_SRC}
  ${NUMERICS_SRC}
)

target_include_directories(test_integrators PRIVATE
  src
)

add_test(NAME IntegratorsTest COMMAND test_integrators)

# --------------------------------------------------
# Output locations (nice-to-have)
# --------------------------------------------------

set_target_properties(axonhh PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(test_rates test_integrators PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
