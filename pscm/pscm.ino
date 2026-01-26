#include <Arduino.h>

#define pin_MCP_SC1
#define pin_MCP_SC2
#define pin_MOSI
#define pin_MISO
#define pin_SCK

#define pin_LED_V
#define pin_LED_C

#define pin_SW_V
#define pin_SW_C
#define pin_ENC_V_A
#define pin_ENC_V_B
#define pin_ENC_C_A
#define pin_ENC_C_B

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}