#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LOLIN_I2C_BUTTON.h>

// Pin-Definitionen
#define CS_PIN 5   // Chip Select (CS) Pin
#define INT_PIN 21 // Interrupt Pin

// Displaygröße für externes OLED-Display (64x48 Pixel)
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 48
#define OLED_RESET    -1 // Reset-Pin (nicht genutzt)
#define SCREEN_ADDRESS 0x3C // Standard-I2C-Adresse des Displays

// I2C-Pins für Wemos D1 Mini (ESP8266)
#define SDA_PIN 4 // D2
#define SCL_PIN 5 // D1

// MCP2515 Objekt
MCP_CAN CAN(CS_PIN);
// OLED-Display Objekt
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_RESET);
// Button Objekt
I2C_BUTTON button(DEFAULT_I2C_BUTTON_ADDRESS); // I2C Address 0x31

void setup() {
  Serial.begin(115200);

  // Initialisierung des MCP2515
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 OK");
  } else {
    Serial.println("MCP2515 FAIL");
    while (1);
  }

  // Setze MCP2515 in den normalen Modus
  CAN.setMode(MCP_NORMAL);

  // Initialisierung des Displays mit benutzerdefinierten I2C-Pins
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
}

void loop() {
  int displayNumber = 3;

  // Überprüfung, ob eine Taste gedrückt wurde
  if (button.get() == 0) {
    if (button.BUTTON_A) {
      displayNumber = 4;
    }
    if (button.BUTTON_B) {
      displayNumber = 3;
    }
  }

  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.print(displayNumber);
  display.display();

  // Check if a CAN message is available
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    long unsigned int rxId; // Variable to store the received CAN ID
    unsigned char len = 0;  // Variable to store the length of the received message
    unsigned char buf[8];   // Buffer to store up to 8 bytes of CAN data

    // Read the incoming CAN message
    CAN.readMsgBuf(&rxId, &len, buf);

    // Print the CAN ID in hexadecimal format (3 digits, padded with zeros if necessary)
    Serial.printf("%03lX; ", rxId); 
    
    // Print the length of the message (number of bytes) without any label
    Serial.print(len); 
    Serial.print("; ");

    // Print each byte of the message in hexadecimal format, separated by commas
    for (int i = 0; i < len; i++) {
        if (i > 0) Serial.print(", ");  // Add a comma between bytes
        Serial.printf("%02X", buf[i]); // Each byte is formatted as a 2-digit hexadecimal number
    }
    
    Serial.println(); // Print a newline to separate each message
  }
}
