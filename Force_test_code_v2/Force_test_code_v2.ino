// Force_test_code_v2.ino
// Use with Force_test_logger_v2.m
// Yeah, yeah I know, the code is dirty. But hey at least it seems to work!

// Include the string.h library, which contains the strtok() function. For
#include <string.h>
#include <AccelStepper.h>
// Define stepper motors
AccelStepper stepper1(AccelStepper::DRIVER, 2, 22); // Motor 1: Step pin 2, direction pin 3
AccelStepper stepper2(AccelStepper::DRIVER, 3, 23); // Motor 2: Step pin 4, direction pin 5
AccelStepper stepper3(AccelStepper::DRIVER, 4, 24); // Motor 3: Step pin 6, direction pin 7
AccelStepper stepper4(AccelStepper::DRIVER, 5, 25); // Motor 4: Step pin 8, direction pin 9
int positionStep[] = {0, 0, 0, 0};

// Define endstops
const int endstop1Pin = 30; // Endstop for motor 1 connected to pin 10
const int endstop2Pin = 31; // Endstop for motor 2 connected to pin 11
const int endstop3Pin = 32; // Endstop for motor 3 connected to pin 12
const int endstop4Pin = 33; // Endstop for motor 4 connected to pin 13

// Define sensors
const int SENSOR_PIN1 = A0; // analog pin connected to the sensor
const int SENSOR_PIN2 = A2; // analog pin connected to the sensor
const int SENSOR_PIN3 = A4; // analog pin connected to the sensor
const int SENSOR_PIN4 = A6; // analog pin connected to the sensor
const int SENSOR_PIN[] = {SENSOR_PIN1, SENSOR_PIN2, SENSOR_PIN3, SENSOR_PIN4};
int sensorVal[] = {0, 0, 0, 0};

int long initial_homing = 1;

bool homingComplete1 = false;
bool homingComplete2 = false;
bool homingComplete3 = false;
bool homingComplete4 = false;

bool moving = false;

unsigned int long time = 0;
unsigned int long startTime = 0;
unsigned int long lastTime = 0;
unsigned int long stopTime = 0;

int home = -17900;

// Snelheid van de motoren
unsigned int motorSpeed = 2000; // Adjust to preferenc in steps per second
unsigned int motorAccel = 500;
unsigned int homingSpeed = 2000;

const unsigned int long BAUD_RATE = 1000000;
boolean isRunning = false;    // boolean for running state
boolean confirm = false;      // confirmation during setup
boolean homed = false;        // boolean for homing

String input = "";

int startF1 = 0;
int startF2 = 0;
int startF3 = 0;
int startF4 = 0;
int endF1 = 0;
int endF2 = 0;
int endF3 = 0;
int endF4 = 0;
int difForce1 = 0;
int difForce2 = 0;
int difForce3 = 0;
int difForce4 = 0;
int targetF = 800;

boolean pull = false;

boolean runM1 = false;
boolean runM2 = false;
boolean runM3 = false;
boolean runM4 = false;

float testSpeedPull = -50;
float testSpeedRelease = 50;

// Setup function
void setup() {
  // Configure endstops as input with pull-up reistor
  pinMode(endstop1Pin, INPUT_PULLUP);
  pinMode(endstop2Pin, INPUT_PULLUP);
  pinMode(endstop3Pin, INPUT_PULLUP);
  pinMode(endstop4Pin, INPUT_PULLUP);
  // Set max speed for motors
  stepper1.setMaxSpeed(motorSpeed);
  stepper2.setMaxSpeed(motorSpeed);
  stepper3.setMaxSpeed(motorSpeed);
  stepper4.setMaxSpeed(motorSpeed);
  // Set the acceleration of the motors
  stepper1.setAcceleration(motorAccel);
  stepper2.setAcceleration(motorAccel);
  stepper3.setAcceleration(motorAccel);
  stepper4.setAcceleration(motorAccel);

  Serial.begin(BAUD_RATE);
  Serial.setTimeout(1000); // set timeout to 1 seconds
  
  Serial.println("Awaiting input...");
  
  // Wait for confirmation from Matlab before continuing
  while (confirm == 0) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
      if (input == "hello") {
        confirm = true;
      }
    }
    delay(1); 
  }
  delay(100);

  // send confirmation message to Matlab code
  Serial.println("Arduino is ready");

  // homing steppers on command from Matlab
  while (homed == 0){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
      if (input == "skip") {
        Serial.println("skipped homing");
        stepper1.setCurrentPosition(home);
        stepper2.setCurrentPosition(home);
        stepper3.setCurrentPosition(home);
        stepper4.setCurrentPosition(home);
        homed = true;
        break;
      }
      else if (input == "home") {
        homing();
      }
    }
  }
  delay(50);
  // Set the startforces depending on the current force reading
  startF1 = analogRead(SENSOR_PIN[0]);
  startF2 = analogRead(SENSOR_PIN[1]);
  startF3 = analogRead(SENSOR_PIN[2]);
  startF4 = analogRead(SENSOR_PIN[3]);
  // Calculate the endforce; when the motor needs to stop running
  endF1 = startF1 + targetF;
  endF2 = startF2 + targetF;
  endF3 = startF3 + targetF;
  endF4 = startF4 + targetF;
/*
  Serial.print(endF1);
  Serial.print(", ");
  Serial.print(endF2);
  Serial.print(", ");
  Serial.print(endF3);
  Serial.print(", ");
  Serial.println(endF4);
*/
  startTime = millis();
  lastTime = millis();
}

// Loop
void loop() {
  time = millis();
      
  if (isRunning == true) {
    if (millis() - startTime >= 4000){
      move();
    }
    if (millis() - lastTime >= 49){
      lastTime = millis();
      readSendValues();
    }
    if (millis() - stopTime >= 4000 && stopTime != 0){
      isRunning = false;
      Serial.println("done");
      stopTime = 0;
    }
  }

  if (Serial.available() > 0) {
    input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
    // Retreive target forces from serial
    if (input == "F1") {
      // Only enable motor 1
      runM1 = true;
      runM2 = false;
      runM3 = false;
      runM4 = false;
      pull = true;
    }
    else if (input == "F2") {
      // Only enable motor 2
      runM1 = false;
      runM2 = true;
      runM3 = false;
      runM4 = false;
      pull = true;
    }
    else if (input == "F3") {
      // Only enable motor 3
      runM1 = false;
      runM2 = false;
      runM3 = true;
      runM4 = false;
      pull = true;
    }
    else if (input == "F4") {
      // Only enable motor 4
      runM1 = false;
      runM2 = false;
      runM3 = false;
      runM4 = true;
      pull = true;
    }    
    // Enable running if command is given
    else if (input == "begin"){
      isRunning = true;
      startTime = millis();
      lastTime = millis();
    }
    // Terminate motion when stop command is given
    else if (input == "stop"){
      isRunning = false;
      runM1 = false;
      runM2 = false;
      runM3 = false;
      runM4 = false;
      pull = false;
    }
    // move to home position
    else if (input == "home") {
      homing();
    }
    else if (input == "zero") {
      zeroPos();
      runM1 = false;
      runM2 = false;
      runM3 = false;
      runM4 = false;
      pull = false;
    }
  }
}

// Move the motor according to input
void move() {
  //Serial.println("moving");
  // motor 1
  if (runM1 == true) {
    sensorVal[0] = analogRead(SENSOR_PIN[0]);
    difForce1 = endF1 - sensorVal[0];
    if (difForce1 >= 1 && pull == true) {
      stepper1.setSpeed(testSpeedPull);
      stepper1.runSpeed();
    }
    else {
      pull = false;
      stepper1.setSpeed(testSpeedRelease);
      stepper1.moveTo(home);
      if (stepper1.distanceToGo() != 0) {
        stepper1.run();
      }
      else if (stepper1.distanceToGo() == 0) {
        stopTime = millis();
        runM1 = false;
      }
    }
  }
  // motor 2
  if (runM2 == true) {
    sensorVal[1] = analogRead(SENSOR_PIN[1]);
    difForce2 = endF2 - sensorVal[1];
    if (difForce2 >= 1 && pull == true) {
      stepper2.setSpeed(testSpeedPull);
      stepper2.runSpeed();
    }
    else {
      pull = false;
      stepper2.setSpeed(testSpeedRelease);
      stepper2.moveTo(home);
      if (stepper2.distanceToGo() != 0) {
        stepper2.run();
      }
      else if (stepper2.distanceToGo() == 0) {
        stopTime = millis();
        runM2 = false;
      }
    }
  }
  // motor 3
  if (runM3 == true) {
    sensorVal[2] = analogRead(SENSOR_PIN[2]);
    difForce3 = endF3 - sensorVal[2];
    if (difForce3 >= 1 && pull == true) {
      stepper3.setSpeed(testSpeedPull);
      stepper3.runSpeed();
    }
    else {
      pull = false;
      stepper3.setSpeed(testSpeedRelease);
      stepper3.moveTo(home);
      if (stepper3.distanceToGo() != 0) {
        stepper3.run();
      }
      else if (stepper3.distanceToGo() == 0) {
        stopTime = millis();
        runM3 = false;
      }
    }
  }
  // motor 4
  if (runM4 == true) {
    sensorVal[3] = analogRead(SENSOR_PIN[3]);
    difForce4 = endF4 - sensorVal[3];
    if (difForce4 >= 1 && pull == true) {
      stepper4.setSpeed(testSpeedPull);
      stepper4.runSpeed();
    }
    else {
      pull = false;
      stepper4.setSpeed(testSpeedRelease);
      stepper4.moveTo(home);
      if (stepper4.distanceToGo() != 0) {
        stepper4.run();
      }
      else if (stepper4.distanceToGo() == 0) {
        stopTime = millis();
        runM4 = false;
      }
    }
  }
}

// Read and send values function. takes 5ms to run
void readSendValues() {
  sensorVal[0] = analogRead(SENSOR_PIN[0]);
  sensorVal[1] = analogRead(SENSOR_PIN[1]);
  sensorVal[2] = analogRead(SENSOR_PIN[2]);
  sensorVal[3] = analogRead(SENSOR_PIN[3]);

  positionStep[0] = stepper1.currentPosition();
  positionStep[1] = stepper2.currentPosition();
  positionStep[2] = stepper3.currentPosition();
  positionStep[3] = stepper4.currentPosition();

  time = millis() - startTime;
  // Create a single string to hold all the data
  String outputString = "";
  for (int i = 0; i <= 4; i++) {
    if (i < 4){
      outputString += String(positionStep[i]) + ", " + String(sensorVal[i]) + ", ";    
    }
    else if (i == 4) {
      outputString += time;
    }
  }

  // Print the entire string at once
  Serial.println(outputString);
}

// Homing function
void homing() {
  stepper1.setSpeed(homingSpeed);
  stepper2.setSpeed(homingSpeed);
  stepper3.setSpeed(homingSpeed);
  stepper4.setSpeed(homingSpeed);

  while (digitalRead(endstop1Pin) || digitalRead(endstop2Pin) || digitalRead(endstop3Pin) || digitalRead(endstop4Pin)) {
    if (digitalRead(endstop1Pin)) {
      stepper1.runSpeed();
    }
    if (digitalRead(endstop2Pin)) {
      stepper2.runSpeed();
    }
    if (digitalRead(endstop3Pin)) {
      stepper3.runSpeed();
    }
    if (digitalRead(endstop4Pin)) {
      stepper4.runSpeed();
    }
    initial_homing ++;
  }
  // Reset the current position of the motors
  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);
  stepper3.setCurrentPosition(0);
  stepper4.setCurrentPosition(0);

  stepper1.moveTo(home);
  stepper2.moveTo(home);
  stepper3.moveTo(home);
  stepper4.moveTo(home);
  // Move the steppers to the 0 position where you can attach the distal end
  while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0 || stepper3.distanceToGo() != 0 || stepper4.distanceToGo() != 0){
    if (stepper1.distanceToGo() != 0){
      stepper1.run();
    }
    if (stepper2.distanceToGo() != 0){
      stepper2.run();
    }
    if (stepper3.distanceToGo() != 0){
      stepper3.run();
    }
    if (stepper4.distanceToGo() != 0){
      stepper4.run();
    }
  }
  // Let Matlab know that the homing sequence has finished
  Serial.println("Homed");
  homed = true;
  delay(5);
}

void zeroPos() {
  Serial.println("moving to zero");

  stepper1.setSpeed(homingSpeed);
  stepper2.setSpeed(homingSpeed);
  stepper3.setSpeed(homingSpeed);
  stepper4.setSpeed(homingSpeed);

  stepper1.moveTo(home);
  stepper2.moveTo(home);
  stepper3.moveTo(home);
  stepper4.moveTo(home);
  // Move the steppers to the 0 position where you can attach the distal end
  while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0 || stepper3.distanceToGo() != 0 || stepper4.distanceToGo() != 0){
    //Serial.println("moving to zero while");
    if (stepper1.distanceToGo() != 0){
      stepper1.run();
    }
    if (stepper2.distanceToGo() != 0){
      stepper2.run();
    }
    if (stepper3.distanceToGo() != 0){
      stepper3.run();
    }
    if (stepper4.distanceToGo() != 0){
      stepper4.run();
    }
  }
}
