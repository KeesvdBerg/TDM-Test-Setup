// Running_Prototype_Arduino.ino
/*
This is the code for the test setup run it with the Matlab script: Prototype_Run_Code.m
*/

// Include the string.h library, which contains the strtok() function. For
#include <string.h>
#include <AccelStepper.h>

// Define stepper motors
// Initialize the stepper motors as global variables
AccelStepper stepper1(AccelStepper::DRIVER, 2, 22);
AccelStepper stepper2(AccelStepper::DRIVER, 3, 23);
AccelStepper stepper3(AccelStepper::DRIVER, 4, 24);
AccelStepper stepper4(AccelStepper::DRIVER, 5, 25);

// Create an array of pointers to the stepper objects
AccelStepper* steppers[] = {&stepper1, &stepper2, &stepper3, &stepper4};
const int numSteppers = sizeof(steppers) / sizeof(steppers[0]);

int long positionStep[numSteppers] = {0};  // Initialize vector containing the current step of the motors
int long pretensionStep[numSteppers] = {0}; // Inintialize vector containing the step location of pretension

// Define endstops array
const int endstopPins[] = {30, 31, 32, 33}; // Array of endstop pins
const int numOfEndstops = sizeof(endstopPins) / sizeof(endstopPins[0]); // Calculate the number of endstops

// Define sensor pins directly in an array
const int sensorPins[] = {A0, A2, A4, A6};
const int numSensors = sizeof(sensorPins) / sizeof(sensorPins[0]);  // Calculate the number of sensors

int sensorVal[numSensors] = {0};

int startForces[numSensors] = {0}; // Initialize the startfoce 
int targetForces[numSensors] = {0}; // Initialize the targetforce 
int currentForces[numSensors] = {0}; // Initialize the current force reading used in the move to target forces void
int offset  = 15; // This is the sensor ADC value offset for the motor preload

unsigned long lastReportTime = 0;
const unsigned long reportInterval = 99; // Reporting interval in milliseconds (10Hz)

int home = -17900;

// Speed of motors
unsigned int maxSpeed = 2500; // Set max motor speed
unsigned int motorSpeed = 800; // Adjust to preference in steps per second
unsigned int motorAccel = 1000; // Acceleration of motors
unsigned int homingSpeed = 2000; // Speed used for homing
int tensionSpeed = 400; // Speed used to reach pretension

int long targetSteps_input = 0;  // target received from Matlab
long int targetSteps[numSensors] = {0}; // Target value retreived from the Matlab script + offset from pretension 
long int pretensionStepsHome[numSensors] = {0}; // Initialize the location of the pretension steps

const unsigned int long BAUD_RATE = 2000000;
boolean isRunning = false;    // boolean for running state
boolean isCircling = false;    // boolean for circling state
boolean confirm = false;      // confirmation during setup
boolean homed = false;        // boolean for homing
boolean pretension = false;   // boolean to indicate end

String input = "";

// timing variables
//int startTime = 0;
//int lastTime = 0;
unsigned long sequenceStartTime = 0; // variable to store the start time of moveMotorsInSequence()

int setForce[4];
int difForce[4]; // Array to hold the differences

// Setup function
void setup() {
  // doing things with the led
  pinMode(LED_BUILTIN, OUTPUT);

  // Configure endstops as input with pull-up reistor
  for (int i = 0; i < numOfEndstops; i++) {
    pinMode(endstopPins[i], INPUT_PULLUP);
  }

  for (int i = 0; i < numSteppers; ++i) {
    steppers[i]->setMaxSpeed(maxSpeed);
    steppers[i]->setAcceleration(motorAccel);
  }

  Serial.begin(BAUD_RATE);
  Serial.setTimeout(1000); // set timeout to 1 seconds
  
  Serial.println("Arduino ready");
  
  // Wait for confirmation from Matlab before continuing
  while (!confirm) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
      if (input.equals("Hello")) {
        confirm = true;
        Serial.println("Handshake"); // Send confirmation message back to MATLAB
        break; // Exit the loop once the handshake is confirmed
      }
    }
  }

  // homing steppers on command from Matlab
  bool homed = false;
  while (!homed){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
      input.trim(); // Remove any whitespace or newline characters
      
      if (input.equals("skip")) {
        Serial.println("Skipped homing");
        // Setting current position to 0
        for (int i = 0; i < numSteppers; ++i) {
          steppers[i]->setCurrentPosition(0);
        }
        homed = true; // Exit the loop
      }
      else if (input == "Home") {
        homing();
        homed = true;
      }
    }
  }
  //digitalWrite(LED_BUILTIN, HIGH);
  //delay(500);
  //digitalWrite(LED_BUILTIN, LOW);
  // Wait until confirmation from Matlab is received that the distal end is connected
  bool pretension = false;
  while (!pretension){
    if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
    input.trim(); // Remove any whitespace or newline characters
    //Serial.println(input);
      
    if (input.equals("connected")) {
      Serial.println("Enabling pre-tension");
      for (int i = 0; i < numSensors; i++) {
        startForces[i] = analogRead(sensorPins[i]);
        targetForces[i] = startForces[i] + offset;
      }
      moveToTargetForces();
      Serial.println("Pretension");
      pretension = true;
      }
    }
  }

  // Wait for confirmation from Matlab before continuing
  confirm = false;
  while (!confirm) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
      if (input.equals("start")) {
        confirm = true;
        Serial.println("starting"); // Send confirmation message back to MATLAB
        break; // Exit the loop once the handshake is confirmed
      }
    }
  }
}

// Loop
void loop() {
        
  if (isRunning == true) {
    //speedCalc(); 
    //move();
    reportData();
    moveMotorsInSequence();
  }

  if (isCircling == true) {
    reportData();
    moveMotorsInCircle();
  }

  if (Serial.available() > 0) {
    input = Serial.readStringUntil('\n'); // Read a line of text until a newline character is received
    
    // Enable running if command is given
    if (input == "begin") {
      Serial.println("Starting the motion sequence...");
      isRunning = true;
    }
    else if (input.startsWith("Circle_")) {
      targetSteps_input = input.substring(7).toInt();
      Serial.println("Steps for circle received");
    }
    else if (input == "circle") {
      Serial.println("Starting the circling sequence...");
      isCircling = true;
    }
    else if (input.startsWith("Steps_")) {
      targetSteps_input = input.substring(6).toInt();
      Serial.println("Steps received");
    }
    // Terminate motion when stop command is given
    else if (input == "stop") {
      isRunning = false;
    }
    // move to home position
    else if (input == "Home") {
      homing();
    }
    // Enable pretension sequence
    else if (input == "pretension") {
      moveToTargetForces();
    }
    else if (input == "zero") {
      zeroPos();
    }
  }
}

void reportData() {
  unsigned long currentMillis = millis();
  unsigned long runtime = currentMillis - sequenceStartTime; // Calculate runtime

  if (currentMillis - lastReportTime >= reportInterval) {
    Serial.print(runtime); // First, print the runtime
    for (int i = 0; i < numSteppers; i++) {
      Serial.print(","); // Delimiter
      Serial.print(steppers[i]->currentPosition()); // Motor position
      Serial.print(","); // Delimiter
      Serial.print(analogRead(sensorPins[i])); // Sensor value
    }
    Serial.println(); // End of the data packet
    lastReportTime = currentMillis; // Update the last report time
  }
}

void moveMotorsInSequence() {
  // Set motor speeds
  for (int i = 0; i < numSteppers; ++i) {
    steppers[i]->setMaxSpeed(motorSpeed);
  }
  
  sequenceStartTime = millis(); // Capture the start time of the sequence
    
  for (int i = 0; i < numSteppers; i++) {
    moveMotorAndAntagonist(i);
    //delay(1000); // Delay between cycles for clarity, adjust as needed
    int long time_wait2 = millis();
    while (millis() - time_wait2 < 3000) {
      reportData();
    }
  }
  Serial.println("Finished");
  isRunning = false;
}

void moveMotorAndAntagonist(int leadMotorIndex) {
  int antagonistIndex = (leadMotorIndex + 2) % numSteppers; // Calculate antagonist index

  // Set the targets
  steppers[leadMotorIndex]->moveTo(pretensionStepsHome[leadMotorIndex] - targetSteps_input);
  steppers[antagonistIndex]->moveTo(pretensionStepsHome[antagonistIndex] + targetSteps_input);

  // Move until both motors reach their target
  while (steppers[leadMotorIndex]->distanceToGo() != 0 || steppers[antagonistIndex]->distanceToGo() != 0) {
    steppers[leadMotorIndex]->run();
    steppers[antagonistIndex]->run();
    reportData();
  }

  // Hold position for 5 seconds
  int long time_wait5 = millis();
    while (millis() - time_wait5 < 3000) {
      reportData();
    }

  // Move back to pretension position
  steppers[leadMotorIndex]->moveTo(pretensionStepsHome[leadMotorIndex]);
  steppers[antagonistIndex]->moveTo(pretensionStepsHome[antagonistIndex]);
  while (steppers[leadMotorIndex]->distanceToGo() != 0 || steppers[antagonistIndex]->distanceToGo() != 0) {
    steppers[leadMotorIndex]->run();
    steppers[antagonistIndex]->run();
    reportData();
  }
}

void pullMotor(int leadMotorIndex, int steps) {
  int antagonistIndex = (leadMotorIndex + 2) % numSteppers; // Calculate antagonist index
  // Assuming 'steps' is the number of steps to pull
  steppers[leadMotorIndex]->moveTo(steppers[leadMotorIndex]->currentPosition() - steps);
  steppers[antagonistIndex]->moveTo(steppers[antagonistIndex]->currentPosition() + steps);
}

void easeMotor(int leadMotorIndex, int steps) {
  int antagonistIndex = (leadMotorIndex + 2) % numSteppers; // Calculate antagonist index
  // Assuming 'steps' is the number of steps to ease
  steppers[leadMotorIndex]->moveTo(steppers[leadMotorIndex]->currentPosition() + steps);
  steppers[antagonistIndex]->moveTo(steppers[antagonistIndex]->currentPosition() - steps);
}

void moveMotorsInCircle() {
  //const int steps = targetSteps_input; // Define steps for pull and ease operations
  const int numSequence = 8; // Total operations in the sequence
  int operationOrder[numSequence] = {1, 0, 2, 1, 3, 2, 0, 3}; // Sequence of operations based on your description
  int antagonistIndex[numSequence] = {0};

  // Set motor speeds
  for (int i = 0; i < numSteppers; ++i) {
    steppers[i]->setSpeed(motorSpeed);
  }

  // Calculate antagonist index
  for (int i = 0; i < numSequence; i++) {
      antagonistIndex[i] = (operationOrder[i] + 2) % numSteppers; 
  }
    
  // First, move to the circle perimeter with 1p
  pullMotor(0, targetSteps_input); // Motor indices are 0-based in this example
  while (steppers[0]->distanceToGo() != 0) {
    steppers[0]->run();
    steppers[2]->run();
    reportData();
  }

  // Execute the circular motion sequence
  for (int i = 0; i < numSequence; i++) {
    if (i % 2 == 0) { // Even indices in the sequence are for pulling
      pullMotor(operationOrder[i], targetSteps_input);
    } else { // Odd indices are for easing
      easeMotor(operationOrder[i], targetSteps_input);
    }

    // Wait for the current operation to complete before moving to the next
    while (steppers[operationOrder[i]]->distanceToGo() != 0) {
      steppers[operationOrder[i]]->run();
      steppers[antagonistIndex[i]]->run();
      reportData();
    }
  }

  // Finally, ease motor 1 to move to neutral position
  easeMotor(0, targetSteps_input); // Motor indices are 0-based
  while (steppers[0]->distanceToGo() != 0) {
    steppers[0]->run();
    steppers[2]->run();
    reportData();
  }
  isCircling = false;
  Serial.println("Finished Circle Motion");
}



void moveToTargetForces() {
  bool targetReached[numSensors] = {false}; // Track if each motor reached its target force
  bool allTargetsReached = false;

  // Set motor speeds
  for (int i = 0; i < numSteppers; ++i) {
    steppers[i]->setSpeed(-tensionSpeed);
  }
  
  while (!allTargetsReached) {
    allTargetsReached = true; // Assume all targets are reached, verify in the loop

    for (int i = 0; i < numSensors; i++) {
      if (!targetReached[i]) {
        currentForces[i] = analogRead(sensorPins[i]); // Update current force reading

        // Check if the current force is less than the target force
        if (targetForces[i] - currentForces[i] > 1) {
          // Move motor to increase force
          //steppers[i]->setSpeed(-tensionSpeed); // Set a positive speed to move
          steppers[i]->runSpeed();
          allTargetsReached = false; // At least one motor hasn't reached its target
        } else {
          // Stop motor, as it reached or exceeded the target force
          steppers[i]->stop(); // Stop the motor
          targetReached[i] = true; // Mark this motor as having reached the target
        }
      }
    }

    // Remember current motor steps
    for (int i = 0; i < numSensors; i++) {
      pretensionStepsHome[i] = steppers[i]->currentPosition();
    }
    // Optional: Report current forces and positions
    //reportData();
  }
}

// Homing function
void homing() {
  bool anyEndstopActive = true;

  // Set homing speed
  for (int i = 0; i < numSteppers; ++i) {
    steppers[i]->setMaxSpeed(homingSpeed);
    steppers[i]->setSpeed(homingSpeed);
  }

  // Move till endstops are triggered
  while(anyEndstopActive) {
    anyEndstopActive = false; // Reset flag at the start of each loop iteration

    for (int i = 0; i < numSteppers; i++) {
      if (digitalRead(endstopPins[i])) {
        steppers[i]->runSpeed();
        anyEndstopActive = true; // At least one endstop is active
      }
    }
    // Optional: Add a small delay to slow down the loop and reduce noise on the endstop pins
    //delay(5);
  }

  for(int i = 0; i < numSteppers; i++) {
    steppers[i]->setCurrentPosition(0); // Set the current position as 0 after homing
  }

  // Move to home
  // Set the target home position for each stepper
  for (int i = 0; i < numSteppers; i++) {
    steppers[i]->setMaxSpeed(motorSpeed);
    steppers[i]->moveTo(home);
  }
  
  // Keep running until all steppers reach their target position
  bool anyStepperMoving = true;
  while (anyStepperMoving) {
    anyStepperMoving = false; // Assume no stepper is moving, verify in the loop
    
    for (int i = 0; i < numSteppers; i++) {
      if (steppers[i]->distanceToGo() != 0) {
        steppers[i]->run();
        anyStepperMoving = true; // At least one stepper is still moving
      }
    }
  }

  // Set current position to 0
  for(int i = 0; i < numSteppers; i++) {
    steppers[i]->setCurrentPosition(0); // Set the current position as 0 after homing
  }

  // Let Matlab know that the homing sequence has finished
  Serial.println("Homed");
  homed = true;
}

// Move to the zero position, this function is not called by the void loop
void zeroPos() {
  Serial.println("moving to zero");
  
  // Move to home
  // Set the target home position for each stepper
  for (int i = 0; i < numSteppers; i++) {
    steppers[i]->setMaxSpeed(homingSpeed);
    steppers[i]->moveTo(0);
  }
  
  // Keep running until all steppers reach their target position
  bool anyStepperMoving = true;
  while (anyStepperMoving) {
    anyStepperMoving = false; // Assume no stepper is moving, verify in the loop
    
    for (int i = 0; i < numSteppers; i++) {
      if (steppers[i]->distanceToGo() != 0) {
        steppers[i]->run();
        anyStepperMoving = true; // At least one stepper is still moving
      }
    }
  }
}
