arduino-cli compile -b MiniCore:avr:328 --libraries ./lib -v --output-dir ./build
C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
    -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
    -v -V -patmega328pb -cusbasp -Pusb -U lfuse:w:0xEF:m -Uflash:w:build/pscm.ino.hex:i

#arduino-cli compile -b arduino:avr:nano -v --output-dir ./build
#C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
#    -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
#    -v -V -patmega328p -cusbasp -Pusb -Uflash:w:build/pscm.ino.hex:i

#arduino-cli compile -b arduino:avr:uno --libraries ./lib -v --output-dir ./build
#arduino-cli upload -p COM10

"/c/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-objdump.exe" -d -S build/pscm.ino.elf > build/pscm.ino.elf.asm

#CKSEL0 1
#SUT1:0 10
#CKSEL[3:1] 111

#11101111