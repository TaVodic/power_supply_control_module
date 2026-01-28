#include <Arduino.h>

extern "C" {
#include "tm1640snf.h"
}

#define pin_MCP_SC1  9
#define pin_MCP_SC2  10
#define pin_MOSI     11
#define pin_MISO     12
#define pin_SCK      13
#define pin_LED_V    16
#define pin_LED_C    17
#define pin_SW_V     18
#define pin_SW_C     19
#define pin_ENC_V_A  4
#define pin_ENC_V_B  5
#define pin_ENC_C_A  6
#define pin_ENC_C_B  7
#define pin_DISP_CLK 2
#define pin_DISP_DIN 3

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