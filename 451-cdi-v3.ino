# NOTE: TRY THIS MECHA: if (rxId == 0x23A && len >= 2) updateValue(lastSpeed, String(((buf[0] << 8) | buf[1]) * 0.1, 1), SPEED, "Speed", "km/h");


#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <U8g2lib.h>

// Pin-Definitionen
#define CS_PIN     5
#define TOUCH_PIN 33
#define SDA_PIN   32
#define SCL_PIN   25

// Hardware-I2C Display (SH1106/SSD1306)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,              // Rotation
  /* reset=*/ U8X8_PIN_NONE
);

// Seiten (ohne SPEED)
enum Page { OIL_TEMP, FUEL_LEVEL, COOLANT_TEMP, GEAR, RPM, CONSUMPTION };
Page currentPage = OIL_TEMP;

// Letzte angezeigte Werte (6 statt 7)
String lastValue[6] = { "--","--","--","-","--","--" };

// Titel und Einheiten (6 statt 7)
const char* titles[6] = {
  "Oil Temp", "Fuel Level", "Coolant Temp",
  "Gear", "RPM", "Consumption"
};
const char* units[6]  = {
  "C", "L", "C", "", "", "L/h"
};

MCP_CAN CAN(CS_PIN);

void setup() {
  Serial.begin(115200);

  // Hardware-I2C mit 400 kHz
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  u8g2.begin();

  // Touch-Pin (kein Pullup/Pulldown)
  pinMode(TOUCH_PIN, INPUT);

  // CAN init
  SPI.begin();
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ)!=CAN_OK) {
    Serial.println("MCP FAIL");
    while(1);
  }
  CAN.setMode(MCP_NORMAL);

  // Start-Screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0,30,"Start");
  u8g2.sendBuffer();
  delay(800);

  drawFullPage();
}

void loop() {
  // Blättern
  if (digitalRead(TOUCH_PIN)==HIGH) {
    currentPage = Page((currentPage+1)%6);
    drawFullPage();
    delay(200);  // entprellen
  }
  // CAN-Empfang
  else if (CAN.checkReceive()==CAN_MSGAVAIL) {
    uint32_t id; uint8_t len, buf[8];
    CAN.readMsgBuf(&id,&len,buf);
    handleCAN(id,len,buf);
  }
}

void drawFullPage() {
  u8g2.clearBuffer();

  // Titel
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0,12,titles[currentPage]);

  // Wert
  u8g2.setFont(u8g2_font_fub30_tr);
  u8g2.drawStr(0,52,lastValue[currentPage].c_str());

  // Einheit
  if (*units[currentPage]) {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0,64-4,units[currentPage]);
  }

  u8g2.sendBuffer();
}

void updateValue(Page p, const String &v) {
  if (v==lastValue[p] || p!=currentPage) return;
  lastValue[p]=v;
  drawFullPage();
}

void handleCAN(uint32_t id, uint8_t len, uint8_t *b) {
  switch(id) {
    case 0x308:  // Öltemperatur
      if(len>=8) updateValue(OIL_TEMP, String(b[5]-40));
      break;

    case 0x408:  // Kraftstofflevel
      if(len>=1) updateValue(FUEL_LEVEL, String(b[0]*0.5,1));
      break;

    case 0x608:  // Kühlmitteltemp + Verbrauch
      if(len>=1) updateValue(COOLANT_TEMP, String(b[0]-40));
      if(len>=8) updateValue(CONSUMPTION,
         String(((b[5]<<8)|b[6])/100.0,2));
      break;

    case 0x418:  // Gang
      if(len>=1) updateValue(GEAR, gearToStr(b[0]));
      break;

    case 0x212:  // Motordrehzahl
      if(len>=4) updateValue(RPM, String((b[2]<<8)|b[3]));
      break;

    // kein 0x23A mehr!
  }
}

String gearToStr(uint8_t g) {
  switch(g) {
    case 0x35: return "5"; case 0x34: return "4";
    case 0x33: return "3"; case 0x32: return "2";
    case 0x31: return "1"; case 0x4E: return "N";
    case 0x52: return "R"; default:   return "-";
  }
}
