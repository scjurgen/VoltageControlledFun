cmake_minimum_required(VERSION 3.10)
project(VoltageControlledFun)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_STANDARD 11)
if (APPLE)
    set(ARDUINO_AVR ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr)
endif(APPLE)
if (UNIX)
    set(ARDUINO_AVR ~/embedded)
endif (UNIX)

set(SOURCE_FILES
        AudioNoiseGate.cpp
        AudioNoiseGate.h
        PotReader.h
        TapDaBeat.cpp
        TapDaBeat.h
        VoltageControlledFun.cpp
        )

add_executable(VoltageControlledFun ${SOURCE_FILES})

target_compile_definitions(VoltageControlledFun PRIVATE
        __MK20DX256__=1
        F_CPU=72000000
        USB_DISABLED=1)

target_include_directories(VoltageControlledFun PUBLIC
        ${ARDUINO_AVR}/cores/teensy3
        ${ARDUINO_AVR}/libraries/Audio
        ${ARDUINO_AVR}/libraries/Bounce2
        ${ARDUINO_AVR}/libraries/SD
        ${ARDUINO_AVR}/libraries/SPI
        ${ARDUINO_AVR}/libraries/SerialFlash
        ${ARDUINO_AVR}/libraries/Wire
        ${ARDUINO_AVR}/libraries/EEPROM)