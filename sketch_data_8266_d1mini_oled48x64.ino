#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <mcp_can.h>

// Pinbelegung für den ESP8266 und MCP2515
#define OLED_RESET -1  // GPIO0 für das Display
#define CS_PIN 12   // Chip Select (CS) Pin für MCP2515
#define INT_PIN 2 // Interrupt Pin für MCP2515

Adafruit_SSD1306 display(OLED_RESET);
MCP_CAN CAN(CS_PIN);

void setup() {
  Serial.begin(115200);

  // OLED Display Initialisierung
  Serial.println("Initialisiere Display...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Serial.println("Display OK");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  // Initialisierung des MCP2515
  Serial.println("Initialisiere MCP2515...");
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 OK");
  } else {
    Serial.println("MCP2515 FAIL");
    while (1); // Stoppe hier, wenn MCP2515 nicht funktioniert
  }

  // Setze MCP2515 in den normalen Modus
  CAN.setMode(MCP_NORMAL);

  // Nachricht '17' auf das Display schreiben
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("17");
  display.display();



void loop() {
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
