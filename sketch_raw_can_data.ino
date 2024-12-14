#include <SPI.h>
#include <mcp_can.h>

// Pin-Definitionen
#define CS_PIN 5   // Chip Select (CS) Pin
#define INT_PIN 21 // Interrupt Pin

// MCP2515 Objekt
MCP_CAN CAN(CS_PIN);

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
}

void loop() {
  // Überprüfen, ob Daten verfügbar sind
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char buf[8];

    // Daten empfangen
    CAN.readMsgBuf(&rxId, &len, buf);

    // Rohe Ausgabe der CAN-ID und der Daten
    Serial.print("ID: ");
    Serial.print(rxId, HEX);  // Ausgabe der CAN-ID im Hex-Format
    Serial.print(" ");

    // Ausgabe der rohen Daten
    for (int i = 0; i < len; i++) {
      Serial.print(buf[i], HEX);  // Ausgabe jedes Datenbytes im Hex-Format
      Serial.print(" ");
    }

    Serial.println();  // Neue Zeile nach der Ausgabe
  }
}


