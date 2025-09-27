

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// NRF24L01 pins
#define CE_PIN 5
#define CSN_PIN 4

// Joystick pins
#define LEFT_X_PIN 34
#define LEFT_Y_PIN 35
#define LEFT_SW_PIN 27
#define RIGHT_X_PIN 32
#define RIGHT_Y_PIN 33
#define RIGHT_SW_PIN 14

// Buzzer pin
#define BUZZER_PIN 26

// Create RF24 object
RF24 radio(CE_PIN, CSN_PIN);

// Communication address (must match receiver)
const byte address[6] = "00001";

// Data structure to send
struct ControlData {
  int leftX;      // Not used in this setup
  int leftY;      // Motor speed/direction (LEFT stick Y)
  bool leftSW;    // Left joystick button
  int rightX;     // Steering (RIGHT stick X)
  int rightY;     // Not used
  bool rightSW;   // Right joystick button (Emergency stop)
};

ControlData data;

// Variables for smooth control
int deadzone = 50;  // Deadzone for joysticks
unsigned long lastSend = 0;
const unsigned long sendInterval = 50;  // Send every 50ms (20Hz)

void setup() {
  Serial.begin(115200);
  
  // Initialize joystick switch pins
  pinMode(LEFT_SW_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SW_PIN, INPUT_PULLUP);
  
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize radio
  if (!radio.begin()) {
    Serial.println("NRF24L01+ not detected. Check wiring!");
    // Beep pattern for error
    for (int i = 0; i < 5; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
    }
    while (1);
  }
  
  // Configure radio (MUST match receiver exactly)
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MAX);    // MAX power for better range
  radio.setChannel(76);             // Channel 76
  radio.setDataRate(RF24_250KBPS);  // Data rate
  radio.setAutoAck(true);           // Enable auto acknowledgment
  radio.setRetries(5, 15);          // 5 delay, 15 retries
  radio.stopListening();
  
  Serial.println("Transmitter Ready!");
  Serial.println("Address: 00001, Channel: 76, Power: MAX");
  Serial.println("Controls: RIGHT stick X = Steering, LEFT stick Y = Speed");
  
  // Print radio details for debugging
  radio.printDetails();
  
  // Success beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  if (millis() - lastSend >= sendInterval) {
    readControls();
    sendData();
    lastSend = millis();
  }
}

void readControls() {
  // Read analog values (0-4095 on ESP32)
  int rawLeftX = analogRead(LEFT_X_PIN);
  int rawLeftY = analogRead(LEFT_Y_PIN);
  int rawRightX = analogRead(RIGHT_X_PIN);
  int rawRightY = analogRead(RIGHT_Y_PIN);
  
  // Map to -512 to +512 range and apply deadzone
  data.leftX = mapWithDeadzone(rawLeftX, 0, 4095, -512, 512);
  data.leftY = mapWithDeadzone(rawLeftY, 4095, 0, -512, 512);  // Inverted: UP = Forward
  data.rightX = mapWithDeadzone(rawRightX, 0, 4095, -512, 512); // RIGHT stick for steering
  data.rightY = mapWithDeadzone(rawRightY, 0, 4095, -512, 512);
  
  // Read button states (inverted because of pullup)
  data.leftSW = !digitalRead(LEFT_SW_PIN);
  data.rightSW = !digitalRead(RIGHT_SW_PIN);
  
  // Debug output every 1 second
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 1000) {
    Serial.print("Steering (Right X): "); Serial.print(data.rightX);
    Serial.print(" | Speed (Left Y): "); Serial.print(data.leftY);
    Serial.print(" | Left SW: "); Serial.print(data.leftSW);
    Serial.print(" | Right SW: "); Serial.println(data.rightSW);
    lastDebug = millis();
  }
}

int mapWithDeadzone(int value, int inMin, int inMax, int outMin, int outMax) {
  // Map the value
  int mapped = map(value, inMin, inMax, outMin, outMax);
  
  // Apply deadzone
  if (abs(mapped) < deadzone) {
    return 0;
  }
  return mapped;
}

void sendData() {
  bool result = radio.write(&data, sizeof(data));
  
  // Only show connection status occasionally to avoid spam
  static unsigned long lastStatusReport = 0;
  static bool lastResult = false;
  
  if (result != lastResult || millis() - lastStatusReport > 2000) {
    if (result) {
      Serial.println("✅ Connected to receiver!");
    } else {
      Serial.println("❌ Connection failed!");
      // Short error beep
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
    }
    lastResult = result;
    lastStatusReport = millis();
  }
  
  // Beep on button press
  if (data.leftSW || data.rightSW) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(10);
    digitalWrite(BUZZER_PIN, LOW);
  }
}
