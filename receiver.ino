

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ESP32Servo.h>

// NRF24L01 pins
#define CE_PIN 5
#define CSN_PIN 4
#define SCK_PIN 18
#define MOSI_PIN 22  // Custom MOSI pin
#define MISO_PIN 19

// Servo pin
#define SERVO_PIN 13

// L298N Motor B pins
#define MOTOR_IN3 26
#define MOTOR_IN4 27
#define MOTOR_ENB 25

// Create objects
RF24 radio(CE_PIN, CSN_PIN);
Servo steeringServo;

// Communication address (must match transmitter)
const byte address[6] = "00001";

// Data structure to receive (must match transmitter)
struct ControlData {
  int leftX;      // Not used
  int leftY;      // Motor speed/direction  
  bool leftSW;    // Left button
  int rightX;     // Steering
  int rightY;     // Not used
  bool rightSW;   // Right button (Emergency stop)
};

ControlData receivedData;

// Control variables
int currentServoPos = 90;       // Center position
bool emergencyStop = false;
unsigned long lastReceived = 0;
const unsigned long timeoutMs = 500;  // Stop if no signal for 500ms

void setup() {
  Serial.begin(115200);
  
  // Initialize custom SPI pins BEFORE radio.begin()
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
  
  // Initialize servo
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(90);  // Center position
  
  // Initialize motor pins
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);
  
  // Set ENB to HIGH for full speed (no PWM)
  digitalWrite(MOTOR_ENB, HIGH);
  
  // Stop motor initially
  stopMotor();
  
  // Small delay for SPI initialization
  delay(100);
  
  // Initialize radio
  if (!radio.begin()) {
    Serial.println("NRF24L01+ not detected. Check wiring!");
    while (1);
  }
  
  // Configure radio (MUST match transmitter exactly)
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);    // MAX power for better range
  radio.setChannel(76);             // Channel 76  
  radio.setDataRate(RF24_250KBPS);  // Data rate
  radio.setAutoAck(true);           // Enable auto acknowledgment
  radio.setRetries(5, 15);          // 5 delay, 15 retries
  radio.startListening();
  
  Serial.println("Receiver Ready!");
  Serial.println("Address: 00001, Channel: 76, Power: MAX");
  Serial.println("Waiting for transmitter...");
  Serial.println("Controls: Steering + Motor (Digital ON/OFF)");
  
  // Print radio details for debugging
  radio.printDetails();
  
  // Test servo sweep
  Serial.println("Testing servo...");
  for (int pos = 60; pos <= 120; pos += 2) {
    steeringServo.write(pos);
    delay(15);
  }
  for (int pos = 120; pos >= 60; pos -= 2) {
    steeringServo.write(pos);
    delay(15);
  }
  steeringServo.write(90);  // Back to center
  Serial.println("Servo test complete!");
}

void loop() {
  // Check for incoming data
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    lastReceived = millis();
    emergencyStop = false;
    
    // Process received data
    processControls();
    
    // Debug output every 1 second to avoid spam
    static unsigned long lastDebugOutput = 0;
    if (millis() - lastDebugOutput > 1000) {
      Serial.print("Steering: "); Serial.print(receivedData.rightX);
      Serial.print(" | Motor: "); Serial.print(receivedData.leftY);
      Serial.print(" | L_SW: "); Serial.print(receivedData.leftSW);
      Serial.print(" | R_SW: "); Serial.print(receivedData.rightSW);
      Serial.print(" | Emergency: "); Serial.println(emergencyStop);
      lastDebugOutput = millis();
    }
  }
  
  // Check for timeout (safety feature)
  if (millis() - lastReceived > timeoutMs && !emergencyStop) {
    emergencyStop = true;
    stopMotor();
    steeringServo.write(90);  // Center steering
    
    static unsigned long lastTimeoutMsg = 0;
    if (millis() - lastTimeoutMsg > 2000) {
      Serial.println("⚠️ TIMEOUT - No signal from transmitter!");
      lastTimeoutMsg = millis();
    }
  }
  
  delay(10);
}

void processControls() {
  // Process steering (rightX: -512 to +512)
  // Map to servo range (60 to 120 degrees for good steering range)
  int servoPos = map(receivedData.rightX, -512, 512, 60, 120);
  servoPos = constrain(servoPos, 60, 120);
  
  // Smooth servo movement
  if (abs(servoPos - currentServoPos) > 2) {
    if (servoPos > currentServoPos) {
      currentServoPos += 2;
    } else {
      currentServoPos -= 2;
    }
  } else {
    currentServoPos = servoPos;
  }
  steeringServo.write(currentServoPos);
  
  // Process motor control (leftY: -512 to +512)
  // Positive = forward, Negative = reverse, 0 = stopped
  if (!emergencyStop) {
    controlMotor(receivedData.leftY);
  }
  
  // Handle button presses
  if (receivedData.leftSW) {
    // Left button - could be used for lights, horn, etc.
    Serial.println("🔵 Left button pressed!");
  }
  
  if (receivedData.rightSW) {
    // Right button - emergency stop toggle
    emergencyStop = !emergencyStop;
    if (emergencyStop) {
      stopMotor();
      Serial.println("🛑 Emergency stop activated!");
    } else {
      Serial.println("✅ Emergency stop deactivated!");
    }
    delay(200);  // Debounce
  }
}

void controlMotor(int speed) {
  // speed range: -512 to +512
  // Negative = reverse, Positive = forward, 0 = stop
  
  if (abs(speed) < 100) {  // Larger deadzone for digital control
    stopMotor();
    return;
  }
  
  if (speed > 0) {
    // Forward - full speed
    digitalWrite(MOTOR_IN3, HIGH);
    digitalWrite(MOTOR_IN4, LOW);
  } else {
    // Reverse - full speed
    digitalWrite(MOTOR_IN3, LOW);
    digitalWrite(MOTOR_IN4, HIGH);
  }
}

void stopMotor() {
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  // ENB stays HIGH - just direction pins control the motor
}
