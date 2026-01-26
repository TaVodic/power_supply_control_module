arduino-cli compile -b arduino:avr:nano -v --output-dir ./build
C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
    -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
    -v -V -patmega328p -cusbasp -Pusb -Uflash:w:C:/Users/Martin/Documents/Arduino/BLink/build/Blink.ino.hex:i

#C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
#    -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
#    -v -V -patmega168p -cstk500v1 -PCOM9 -b19200 -U lfuse:w:0xDE:m -U hfuse:w:0xDF:m -U efuse:w:0xF9:m -Uflash:w:C:/Users/Martin/Documents/Arduino/LedStrip/build/LedStrip.ino.hex:i