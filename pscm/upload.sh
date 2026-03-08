arduino-cli compile -b MiniCore:avr:328 --libraries ./lib -v --output-dir ./build
C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
    -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
    -v -V -patmega328pb -cusbasp -Pusb -U lfuse:w:0xFF:m -Uflash:w:build/pscm.ino.hex:i

#arduino-cli compile -b arduino:avr:nano -v --output-dir ./build
#C:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude              \
 #   -CC:/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf   \
 #   -v -V -patmega328p -cusbasp -Pusb -Uflash:w:build/pscm.ino.hex:i

#arduino-cli compile -b arduino:avr:uno --libraries ./lib -v --output-dir ./build
#arduino-cli upload -b MiniCore:avr:328 -p COM6 -v
#C:/Users/Martin/AppData/Local/Arduino15/packages/MiniCore/tools/avrdude/7.2-arduino.1/bin/avrdude \
 #   -CC:/Users/Martin/AppData/Local/Arduino15/packages/MiniCore/hardware/avr/3.0.2/avrdude.conf \
  #  -v -V -patmega328pb -curclock -PCOM6 -b115200 -D -xnometadata -Ueeprom:w:C:/Users/Martin/AppData/Local/Temp/arduino/sketches/FFC9C58D10CD3EB4F63B5E30AF03C9AC/pscm.ino.eep:i \
   # -Uflash:w:C:/Users/Martin/AppData/Local/Temp/arduino/sketches/FFC9C58D10CD3EB4F63B5E30AF03C9AC/pscm.ino.hex:i

"/c/Users/Martin/AppData/Local/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-objdump.exe" -d -S build/pscm.ino.elf > build/pscm.ino.elf.asm

#CKSEL0 1
#SUT1:0 11
#CKSEL[3:1] 111

#11111111