#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin-Definitionen
#define CS_PIN 5   // Chip Select (CS) Pin für MCP2515
#define INT_PIN 21 // Interrupt Pin für MCP2515
#define SDA_PIN 32  // SDA Pin auf GPIO 32 für I2C
#define SCL_PIN 25  // SCL Pin auf GPIO 25 für I2C
#define OLED_RESET -1  // GPIO0 (unbenutzt)
#define TOUCH_PIN 33  // Touchsensor an GPIO 33

// MCP2515 Objekt
MCP_CAN CAN(CS_PIN);
Adafruit_SSD1306 display(OLED_RESET);

// Seiten-Index
enum Page { OIL_TEMP, FUEL_LEVEL, COOLANT_TEMP, GEAR, RPM, SPEED };
Page currentPage = SPEED;

// Werte als Strings
String lastOilTemp = "--";
String lastFuelLevel = "--";
String lastCoolantTemp = "--";
String lastSpeed = "--";
String lastRPM = "--";
String lastGear = "-";

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("69");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
  displayPage("Speed", lastSpeed, "km/h");

  SPI.begin();
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 OK");
  } else {
    Serial.println("MCP2515 FAIL");
    while (1);
  }
  CAN.setMode(MCP_NORMAL);
  pinMode(TOUCH_PIN, INPUT);
}

void loop() {
  if (digitalRead(TOUCH_PIN) == HIGH) {  
    // Wenn der Touchsensor gedrückt wird, Seite wechseln
    currentPage = static_cast<Page>((currentPage + 1) % 6);
    updateDisplay();
    delay(300);  // Debounce für den Touchsensor
  } else {  
    // Sonst normal CAN-Daten verarbeiten
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      long unsigned int rxId;
      unsigned char len = 0;
      unsigned char buf[8];
      CAN.readMsgBuf(&rxId, &len, buf);
      processCANMessage(rxId, len, buf);
    }
  }
}

void processCANMessage(long unsigned int rxId, unsigned char len, unsigned char *buf) {
  
  Serial.print("Received CAN ID: 0x");
  Serial.print(rxId, HEX);
  Serial.print(" | Data: ");
  for (int i = 0; i < len; i++) {
    Serial.print(buf[i], DEC);
    Serial.print(" ");
  }
  Serial.println();
  
  
  
  
  if (rxId == 0x308 && len >= 8) updateValue(lastOilTemp, String(buf[5] - 40), OIL_TEMP, "Oil Temp", "C");
  if (rxId == 0x408 && len >= 8) updateValue(lastFuelLevel, String((buf[0]) * 0.5, 1), FUEL_LEVEL, "Fuel Level", "L");
  if (rxId == 0x608 && len >= 8) updateValue(lastCoolantTemp, String(buf[0] - 40), COOLANT_TEMP, "Coolant Temp", "C");
  if (rxId == 0x418 && len >= 8) updateValue(lastGear, getGear(buf[0]), GEAR, "Gear", "");
  if (rxId == 0x212 && len >= 8) updateValue(lastRPM, String((buf[2] << 8) | buf[3]), RPM, "RPM", "");
  if (rxId == 0x23A && len >= 2) updateValue(lastSpeed, String(buf[0] << 8) | buf[1] * 0.1, 1), SPEED, "Speed", "km/h");
}

void updateValue(String &lastValue, String newValue, Page page, String title, String unit) {
  if (newValue != lastValue) {
    lastValue = newValue;
    if (currentPage == page) displayPage(title, newValue, unit);
  }
}

void updateDisplay() {
  switch (currentPage) {
    case OIL_TEMP: displayPage("Oil Temp", lastOilTemp, "C"); break;
    case FUEL_LEVEL: displayPage("Fuel Level", lastFuelLevel, "L"); break;
    case COOLANT_TEMP: displayPage("Coolant Temp", lastCoolantTemp, "C"); break;
    case GEAR: displayPage("Gear", lastGear, ""); break;
    case RPM: displayPage("RPM", lastRPM, ""); break;
    case SPEED: displayPage("Speed", lastSpeed, "km/h"); break;
  }
}

void displayPage(String title, String value, String unit) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(title);
  display.setTextSize(2);
  display.println(value);
  if (unit != "") {
    display.setTextSize(1);
    display.println(unit);
  }
  display.display();
}

String getGear(byte gearByte) {
  switch (gearByte) {
    case 0x35: return "5";
    case 0x34: return "4";
    case 0x33: return "3";
    case 0x32: return "2";
    case 0x31: return "1";
    case 0x4E: return "N";
    case 0x52: return "R";
    default: return "-";
  }
}
