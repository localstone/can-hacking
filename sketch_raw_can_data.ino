#include <SPI.h>
#include <mcp_can.h>

// Pin definitions
#define CS_PIN 5   // Chip Select (CS) pin for MCP2515
#define INT_PIN 21 // Interrupt pin for MCP2515

// Create MCP2515 object with the specified CS pin
MCP_CAN CAN(CS_PIN);

void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate

  // Initialize the MCP2515 CAN controller
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 OK"); // Print success message if initialization is successful
  } else {
    Serial.println("MCP2515 FAIL"); // Print failure message if initialization fails
    while (1); // Enter infinite loop to prevent further execution
  }

  // Set MCP2515 to normal operating mode
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
    Serial.printf("%03lX ", rxId); 
    
    // Print the length of the message (number of bytes) without any label
    Serial.print(len); 
    
    // Print each byte of the message in hexadecimal format, separated by spaces
    for (int i = 0; i < len; i++) {
        Serial.printf(" %02X", buf[i]); // Each byte is formatted as a 2-digit hexadecimal number
    }
    
    Serial.println(); // Print a newline to separate each message
  }
} 
