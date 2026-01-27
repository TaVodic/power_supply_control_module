#include <Arduino.h>

extern "C" {
  #include "tm1640snf.h"
}

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

static void prHex8(uint8_t b) {
  const char hx[] = "0123456789ABCDEF";
  Serial.write(hx[(b >> 4) & 0x0F]);
  Serial.write(hx[b & 0x0F]);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("TM1640 sniff start");

  tm1640snf_begin();
}

void loop() {
  /* digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500); */


  uint8_t v;
  if (tm1640snf_read(&v)) {
    if (v == TM1640SNF_MARK_S) {
      Serial.print("\r\n[S] ");
    } else if (v == TM1640SNF_MARK_E) {
      Serial.print(" [E]\r\n");
    } else {
      prHex8(v);
      Serial.write(' ');
    }
  }
}