# Laboratory Power Supply Control Module (PSCM)   

Module is an upgarde for Nice-Power SPS305 0-30V/0-5A laboratory power supply. Module use encoders instead of potentiometers. Based on ATmega328PB.
Using arduino-cli framework.  
### Features:
* Encoders EC12E, fine and coarse set of volatge and current
* Digital potentiometers MCP4251
* ATmega328PB
* Sniffing the PS voltage, current and power - providing a feedback
* UART connection for PC connection and automated PS setting

### Libraries used:
* Encoder_mt, [original](https://github.com/PaulStoffregen/Encoder) modified to use PCINT and optimized for the use-case
* DispSniff, sniffing SDI & CLK pins of TM1640 (of the original PS) to capture voltage, current and power values of the PS
* MCP4251, [original](https://github.com/kulbhushanchand/MCP4251)

## [Schematic v1.0.0](docu/pscm_schematic_v1.0.0.pdf)    