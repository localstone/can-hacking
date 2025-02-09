#include <SPI.h>
#include <mcp2515.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>

// Farbdefinitionen
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Touchscreen-Pins
#define YP A2
#define XM A3
#define YM 8
#define XP 9

#define TS_MINX 100
#define TS_MAXX 940
#define TS_MINY 120
#define TS_MAXY 900

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Initialisierung von TFT und Touchscreen
MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// CAN-Bus Pin-Definitionen
#define CS_PIN 53
#define INT_PIN 19

// MCP2515 CAN-Controller
MCP2515 mcp2515(CS_PIN);

// Globale Variablen für Seitenanzeige
int currentPage = 0;
const int totalPages = 4;

// Globale Variablen für CAN-Daten
float oilTemp = 0.0;       // ID 308: Öltemperatur = 6. Byte - 40
float value408 = 0.0;      // ID 408: Wert = 1. Byte * 0.5
float coolantTemp = 0.0;   // ID 608: Kühlmitteltemperatur = 1. Byte - 40
char gear = 'N';           // ID 418: Gang (Mapping aus 1. Byte)
float speedVal = 0.0;      // ID 23A: Geschwindigkeit = 1. Byte * 0.1

// Zur Flacker-Vermeidung: Letzter angezeigter Gang
static char lastGear = ' ';

// Dynamischer Hintergrund – Ändere diesen Wert, wenn du den Hintergrund ändern möchtest
uint16_t currentBGColor = BLACK;

void setup() {
  Serial.begin(115200);

  // Display initialisieren
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(currentBGColor);
  tft.setTextColor(WHITE, currentBGColor);
  tft.setTextSize(3);
  tft.setCursor(50, tft.height() / 2 - 20);
  tft.print("Initialisiere CAN...");

  // CAN-Bus initialisieren
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Display mit aktuellem Hintergrund neu zeichnen
  tft.fillScreen(currentBGColor);
  delay(1000);
  displayPage(currentPage);
  // Damit updateDisplayValues() nicht sofort aktualisiert:
  lastGear = gear;
}

void loop() {
  // Touchscreen für Seitenwechsel (unten am Display)
  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    if (p.y > tft.height() - 30) {
      if (p.x < tft.width() / 10) {
        previousPage();
      } else if (p.x > tft.width() - tft.width() / 10) {
        nextPage();
      }
    }
    delay(300);
  }

  // CAN-Nachricht verarbeiten
  struct can_frame frame;
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    // Rohdaten des CAN-Bus ausgeben
    Serial.print("ID: ");
    Serial.print(frame.can_id, HEX);
    Serial.print(" DLC: ");
    Serial.print(frame.can_dlc);
    Serial.print(" Data: ");
    for (int i = 0; i < frame.can_dlc; i++) {
      Serial.print(frame.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Werte aus den CAN-Nachrichten extrahieren:
    switch (frame.can_id) {
      case 0x308: // Öltemperatur (6. Byte - 40)
        if (frame.can_dlc >= 6) {
          oilTemp = round(frame.data[5] - 40);
          Serial.print("OilTemp: ");
          Serial.println(oilTemp);
        }
        break;
      case 0x408: // Wert = 1. Byte * 0.5
        if (frame.can_dlc >= 1) {
          value408 = round(frame.data[0] * 0.5);
          Serial.print("Value408: ");
          Serial.println(value408);
        }
        break;
      case 0x608: // Kühlmitteltemperatur = 1. Byte - 40
        if (frame.can_dlc >= 1) {
          coolantTemp = round(frame.data[0] - 40);
          Serial.print("CoolantTemp: ");
          Serial.println(coolantTemp);
        }
        break;
      case 0x418: // Gang: Mapping des 1. Bytes
        if (frame.can_dlc >= 1) {
          switch (frame.data[0]) {
            case 0x35: gear = '5'; break;
            case 0x34: gear = '4'; break;
            case 0x33: gear = '3'; break;
            case 0x32: gear = '2'; break;
            case 0x31: gear = '1'; break;
            case 0x4E: gear = 'N'; break;
            case 0x52: gear = 'R'; break;
            default:   gear = '?'; break;
          }
          Serial.print("Gear: ");
          Serial.println(gear);
        }
        break;
      case 0x23A: // Geschwindigkeit = 1. Byte * 0.1
        if (frame.can_dlc >= 1) {
          speedVal = round(frame.data[0] * 0.1);
          Serial.print("Speed: ");
          Serial.println(speedVal);
        }
        break;
    }
    // Nur den Gang im unteren mittleren Bereich aktualisieren
    updateDisplayValues();
  }
}

// Zeichnet die statische Seite inkl. Layout, statischer Texte und Buttons
void displayPage(int page) {
  beforeDisplay();
  tft.fillScreen(currentBGColor);
  drawLayout(page);
  drawFuelText(10, 10);
  drawButton(0, tft.height() - 30, tft.width() / 10, 30, "<", GREEN);
  drawButton(tft.width() - tft.width() / 10, tft.height() - 30, tft.width() / 10, 30, ">", BLUE);
  afterDisplay();
}

// Aktualisiert ausschließlich den dynamischen Wert (Gang) im unteren mittleren Bereich
void updateDisplayValues() {
  int colWidth = tft.width() / 4;
  int rowHeight = tft.height() / 4;
  int gearCellX = colWidth;         // Beginn in der 2. Spalte
  int gearCellY = 3 * rowHeight;      // unterste Zeile
  int gearCellWidth = 2 * colWidth;   // mittlere zwei Spalten
  int gearCellHeight = rowHeight;

  // Nur aktualisieren, wenn sich der Gang ändert
  if (gear != lastGear) {
    // Den Bereich mit dem aktuellen Hintergrund füllen (statt BLACK)
    tft.fillRect(gearCellX, gearCellY, gearCellWidth, gearCellHeight, currentBGColor);
    tft.setTextSize(4);
    // Text mit Hintergrundfarbe zeichnen, damit kein "weißes" Überschreiben erfolgt
    tft.setTextColor(YELLOW, currentBGColor);

    // Text zentrieren
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    String gearStr = String(gear);
    tft.getTextBounds(gearStr, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int gearX = gearCellX + (gearCellWidth - textWidth) / 2;
    int gearY = gearCellY + (gearCellHeight - textHeight) / 2;

    tft.setCursor(gearX, gearY);
    tft.print(gear);
    lastGear = gear;
  }
}

void drawFuelText(int x, int y) {
  tft.setTextColor(WHITE, currentBGColor);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print("Fuel");
  tft.setTextSize(3);
  tft.setTextColor(CYAN, currentBGColor);
  tft.setCursor(x, y + 30);
  tft.print("17,5L");
}

void drawLayout(int page) {
  int colWidth = tft.width() / 4;
  int rowHeight = tft.height() / 4;
  if (page == 0) {
    tft.drawLine(colWidth, 0, colWidth, tft.height(), WHITE);
    tft.drawLine(3 * colWidth, 0, 3 * colWidth, tft.height(), WHITE);
    tft.drawLine(0, 3 * rowHeight, tft.width(), 3 * rowHeight, WHITE);
    tft.drawLine(0, rowHeight, tft.width(), rowHeight, WHITE);
    tft.drawLine(0, 2 * rowHeight, colWidth, 2 * rowHeight, WHITE);
    tft.drawLine(3 * colWidth, 2 * rowHeight, 4 * colWidth, 2 * rowHeight, WHITE);
  }
}

void drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 10, color);
  tft.setTextColor(WHITE, color);
  tft.setTextSize(3);
  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  tft.getTextBounds(label, 0, 0, &x1, &y1, &textWidth, &textHeight);
  tft.setCursor(x + (w - textWidth) / 2, y + (h - textHeight) / 2);
  tft.print(label);
}

void previousPage() {
  currentPage = (currentPage - 1 + totalPages) % totalPages;
  displayPage(currentPage);
  lastGear = gear;
}

void nextPage() {
  currentPage = (currentPage + 1) % totalPages;
  displayPage(currentPage);
  lastGear = gear;
}

void beforeDisplay() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
}

void afterDisplay() {
  pinMode(XP, INPUT);
  pinMode(YM, INPUT);
}
