cmake_minimum_required(VERSION 3.18)
project(ESP32Mock LANGUAGES CXX)

# A ponte e um programa do computador, independente do build Arduino/AVR.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
find_path(PAHO_INCLUDE_DIR mqtt/async_client.h REQUIRED)
find_library(PAHO_CPP_LIBRARY NAMES paho-mqttpp3 REQUIRED)
find_library(PAHO_C_LIBRARY NAMES paho-mqtt3a paho-mqtt3as REQUIRED)

# Entrada, configuracao, coordenacao, protocolo e transportes compilam separados.
add_executable(ESP32Mock
    src/main.cpp
    src/config.cpp
    src/esp32.cpp
    src/protocol.cpp
    src/simulator.cpp
    src/ultralight.cpp
    src/serial/serial.cpp
    src/serial/serialMock.cpp
    src/utils/utils.cpp
    src/mqtt/mqtt.cpp
)
target_include_directories(ESP32Mock SYSTEM PRIVATE ${PAHO_INCLUDE_DIR})
target_link_libraries(ESP32Mock PRIVATE
    ${PAHO_CPP_LIBRARY}
    ${PAHO_C_LIBRARY}
    Threads::Threads
)
target_compile_options(ESP32Mock PRIVATE -Wall -Wextra -Wpedantic)
