const int SENSOR_PIN1 = A0; // analog pin connected to the sensor
const int SENSOR_PIN2 = A2; // analog pin connected to the sensor
const int SENSOR_PIN3 = A4; // analog pin connected to the sensor
const int SENSOR_PIN4 = A6; // analog pin connected to the sensor

const int SENSOR_PIN[] = {SENSOR_PIN1, SENSOR_PIN2, SENSOR_PIN3, SENSOR_PIN4};

unsigned int long time = 0;

int sensorVal[] = {0, 0, 0, 0};

void setup() {
  Serial.begin(2000000); // initialize serial communication at 2000000 baud rate
  Serial.println("Starting now...");
  
}

void loop() {
  time = millis();
  if (time % 250 == 0){
    for (int i = 0; i < 4; i ++){
      sensorVal[i] = analogRead(SENSOR_PIN[i]);
      if (i != 3){
        Serial.print(sensorVal[i]);
        Serial.print(", ");
      }
      else if (i == 3){
        Serial.println(sensorVal[i]);
      }
    }
    delay(5);
  }
  
}
