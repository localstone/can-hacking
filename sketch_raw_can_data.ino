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

