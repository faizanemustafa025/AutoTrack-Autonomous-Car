/* AutoTrack - Basic Version
   Features:
   - Line Following (3 Sensors)
   - Obstacle Avoidance (Ultrasonic + IR)
   - Car Following (Maintain Distance)
   - Manual Control (Bluetooth + IR Remote)
*/

#include <Servo.h>
#include <IRremote.h>

// ----- PIN DEFINITIONS -----
const int MOTOR_L_FWD = 6;
const int MOTOR_L_BWD = 11;
const int MOTOR_R_FWD = 5;
const int MOTOR_R_BWD = 3;

const int PIN_SERVO     = 9;
const int PIN_TRIG      = 2;
const int PIN_ECHO      = 4;
const int PIN_IR_RECV   = 12;

const int PIN_LINE_L    = A0;
const int PIN_LINE_M    = A1;
const int PIN_LINE_R    = A2;

const int PIN_OBS_L     = A3;
const int PIN_OBS_R     = A4;

// ----- OBJECTS -----
Servo headServo;
IRrecv irrecv(PIN_IR_RECV);
decode_results results;

enum RobotState {
  STATE_MANUAL,
  STATE_LINE_FOLLOW,
  STATE_OBSTACLE,
  STATE_CAR_FOLLOW,
  STATE_STOP
};
RobotState currentState = STATE_STOP;

// ----- SETTINGS -----
int speedCruise = 180;
int speedTurn   = 255;
int followDist  = 20;

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_L_FWD, OUTPUT);
  pinMode(MOTOR_L_BWD, OUTPUT);
  pinMode(MOTOR_R_FWD, OUTPUT);
  pinMode(MOTOR_R_BWD, OUTPUT);
  pinMode(PIN_LINE_L, INPUT);
  pinMode(PIN_LINE_M, INPUT);
  pinMode(PIN_LINE_R, INPUT);
  pinMode(PIN_OBS_L, INPUT);
  pinMode(PIN_OBS_R, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  headServo.attach(PIN_SERVO);
  headServo.write(90);
  irrecv.enableIRIn();
  Serial.println("AutoTrack Basic Ready");
}

void loop() {
  checkInput();
  switch (currentState) {
    case STATE_MANUAL:
      break;
    case STATE_LINE_FOLLOW:
      runLineFollower();
      break;
    case STATE_OBSTACLE: 
      runObstacleAvoidance();
      break;
    case STATE_CAR_FOLLOW:
      runCarFollower();
      break;
    case STATE_STOP:  
      stopMotors();
      break;
  }
}

void checkInput() {
  if (Serial.available()) {
    char cmd = Serial.read();
    executeCommand(cmd);
  }
  if (irrecv.decode(&results)) {
    unsigned long key = results.value;
    irrecv.resume();
    // REPLACE HEX CODES
    if (key == 0xFF629D) executeCommand('F');
    else if (key == 0xFFA857) executeCommand('B');
    else if (key == 0xFF22DD) executeCommand('L');
    else if (key == 0xFFC23D) executeCommand('R');
    else if (key == 0xFF02FD) executeCommand('S');
    else if (key == 0xFF6897)
    {
      currentState = STATE_LINE_FOLLOW; Serial.println("Line Mode");
    }
    else if (key == 0xFF9867) {
      currentState = STATE_OBSTACLE;
      Serial.println("Obstacle Mode");
    }
    else if (key == 0xFFB04F) {
      currentState = STATE_CAR_FOLLOW;
      Serial.println("Follow Mode");
    }
  }
}

void executeCommand(char cmd) {
  if (strchr("FBLRfbGHD2468", cmd))
    currentState = STATE_MANUAL;
  switch (cmd) {
    case 'F': case 'f': case '8': moveForward(speedCruise);
      break;
    case 'B': case 'b': case '2': moveBackward(speedCruise);
      break;
    case 'L': case 'l': case '4': turnLeft(speedTurn);
      break;
    case 'R': case 'r': case '6': turnRight(speedTurn);
      break;
    case 'S': case '0': stopMotors(); currentState = STATE_STOP;
      break;
    case 'X': currentState = STATE_LINE_FOLLOW;
      break;
    case 'Y': currentState = STATE_OBSTACLE;
      break;
    case 'Z': currentState = STATE_CAR_FOLLOW;
      break;
  }
}

// --- BASIC MODES ---
void runLineFollower() {
  // 1 = Black, 0 = White
  if (digitalRead(PIN_LINE_M) == 1) moveForward(110);
  else if (digitalRead(PIN_LINE_L) == 1) turnLeft(140);
  else if (digitalRead(PIN_LINE_R) == 1) turnRight(140);
  else stopMotors();
}

void runObstacleAvoidance() {
  long d = getDistance();
  if (d < 25 || digitalRead(PIN_OBS_L) == 0 || digitalRead(PIN_OBS_R) == 0) {
    stopMotors(); delay(200);
    moveBackward(150); delay(300);
    turnRight(180); delay(500); // Simple blind turn
  } else {
    moveForward(130);
  }
}

void runCarFollower() {
  long d = getDistance();
  if (d == 0 || d > 60) stopMotors();
  else if (d > followDist + 5) moveForward(130);
  else if (d < followDist - 5) moveBackward(130);
  else stopMotors();
}

long getDistance() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 25000);
  return (dur == 0) ? 999 : dur * 0.034 / 2;
}

void moveForward(int s) {
  analogWrite(MOTOR_L_FWD, s); 
  digitalWrite(MOTOR_L_BWD, LOW);
  analogWrite(MOTOR_R_FWD, s);
  digitalWrite(MOTOR_R_BWD, LOW);
}
void moveBackward(int s) {
  digitalWrite(MOTOR_L_FWD, LOW);
  analogWrite(MOTOR_L_BWD, s);
  digitalWrite(MOTOR_R_FWD, LOW);
  analogWrite(MOTOR_R_BWD, s);
}
void turnLeft(int s) { 
  digitalWrite(MOTOR_L_FWD, LOW);
  analogWrite(MOTOR_L_BWD, s);
  analogWrite(MOTOR_R_FWD, s);
  digitalWrite(MOTOR_R_BWD, LOW);
}
void turnRight(int s) {
  analogWrite(MOTOR_L_FWD, s);
  digitalWrite(MOTOR_L_BWD, LOW); 
  digitalWrite(MOTOR_R_FWD, LOW);
  analogWrite(MOTOR_R_BWD, s);
}
void stopMotors() {
  digitalWrite(MOTOR_L_FWD, LOW);
  digitalWrite(MOTOR_L_BWD, LOW);
  digitalWrite(MOTOR_R_FWD, LOW);
  digitalWrite(MOTOR_R_BWD, LOW);
}