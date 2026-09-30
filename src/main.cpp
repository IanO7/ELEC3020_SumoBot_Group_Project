// --- SENSOR FUNCTIONALITY TEST CODE ---
// LILYGO T-Display-S3 Pin Mapping
#include <Arduino.h>
#include <TFT_eSPI.h>
// --- SENSOR FUNCTIONALITY TEST CODE ---
// LILYGO T-Display-S3 Pin Mapping
 
// Ultrasonic Sensors (Shared Trigger on Pin 1)
const int trigPin = 1;         
const int leftEchoPin = 44;    
const int midEchoPin = 43;     
const int rightEchoPin = 18;   
 
// Infrared (IR) Sensors
const int irTopLeft = 16;      
const int irTopRight = 21;     
const int irBottomRight = 17;  
const int irBottomLeft = 2;    
 
// --- HELPER FUNCTION DECLARATIONS & DEFINITIONS ---
// Placing them here before setup() ensures the compiler sees them first
 
long readUltrasonic(int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout (~5 meters max)
  long distance = duration * 0.034 / 2;          // Convert to cm
  return distance;
}
 
void printDistance(long dist) {
  if (dist <= 0 || dist > 400) {
    Serial.println("Out of range / No echo");
  } else {
    Serial.print(dist);
    Serial.println(" cm");
  }
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Starting Sensor Diagnostics ---");
 
  // Ultrasonic Pin Modes
  pinMode(trigPin, OUTPUT);
  pinMode(leftEchoPin, INPUT);
  pinMode(midEchoPin, INPUT);
  pinMode(rightEchoPin, INPUT);
 
  // IR Sensor Pin Modes (Using INPUT_PULLUP to ensure stable readings)
  pinMode(irTopLeft, INPUT_PULLUP);
  pinMode(irTopRight, INPUT_PULLUP);
  pinMode(irBottomRight, INPUT_PULLUP);
  pinMode(irBottomLeft, INPUT_PULLUP);
}
 
void loop() {
  // 1. Read Ultrasonic Distances
  long leftDist = readUltrasonic(leftEchoPin);
  long midDist = readUltrasonic(midEchoPin);
  long rightDist = readUltrasonic(rightEchoPin);
 
  // 2. Read IR Sensor Digital States
  int tl_state = digitalRead(irTopLeft);
  int tr_state = digitalRead(irTopRight);
  int br_state = digitalRead(irBottomRight);
  int bl_state = digitalRead(irBottomLeft);
 
  // 3. Print Results Cleanly to Serial Monitor
  Serial.println("========================================");
  Serial.println("ULTRASONIC SENSORS (Distance in cm):");
  Serial.print("  [Left (Pin 44)]: ");   printDistance(leftDist);
  Serial.print("  [Middle (Pin 43)]: "); printDistance(midDist);
  Serial.print("  [Right (Pin 18)]: ");  printDistance(rightDist);
 
  Serial.println("\nIR SENSORS (0 = Triggered/Line, 1 = Clear):");
  Serial.print("  Top-Left (Pin 16):     "); Serial.println(tl_state);
  Serial.print("  Top-Right (Pin 21):    "); Serial.println(tr_state);
  Serial.print("  Bottom-Right (Pin 17): "); Serial.println(br_state);
  Serial.print("  Bottom-Left (Pin 2):   "); Serial.println(bl_state);
 
  delay(500); // Half-second delay between checks
}
 