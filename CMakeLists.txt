cmake_minimum_required(VERSION 3.12)
project(VoltageControlledFun)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_STANDARD 11)


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
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/cores/teensy3
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/Audio
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/Bounce2
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SD
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SerialFlash
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/Wire
        ~/Desktop/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/EEPROM)