#include <Servo.h>
#include <IRremote.h>

// ========================== PIN DEFINITIONS ==========================
// Motor Driver (PWM pins)
const int MOTOR_L_FWD = 6;
const int MOTOR_L_BWD = 11;
const int MOTOR_R_FWD = 5;
const int MOTOR_R_BWD = 3;

// Servo
const int PIN_SERVO = 9;

// Ultrasonic Sensor
const int PIN_TRIG = 2;
const int PIN_ECHO = 4;

// IR Receiver
const int PIN_IR_RECV = 12;

// Line Tracking Sensors
const int PIN_LINE_L = A0;
const int PIN_LINE_M = A1;
const int PIN_LINE_R = A2;

// Obstacle IR Sensors
const int PIN_OBS_L = A3;
const int PIN_OBS_R = A4;

// ========================== OBJECTS & STATES ==========================
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

// Settings
int speedCruise = 180;
int speedTurn = 255;
int followDist = 20;

// ========================== SETUP ==========================
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
  Serial.println("AutoTrack System Ready");
}

// ========================== MAIN LOOP ==========================
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

// ========================== INPUT HANDLING ==========================
void checkInput() {
  if (Serial.available()) {
    char cmd = Serial.read();
    executeCommand(cmd);
  }

  if (irrecv.decode(&results)) {
    unsigned long key = results.value;
    irrecv.resume();

    if (key == 0xFF629D) executeCommand('F');
    else if (key == 0xFFA857) executeCommand('B');
    else if (key == 0xFF22DD) executeCommand('L');
    else if (key == 0xFFC23D) executeCommand('R');
    else if (key == 0xFF02FD) executeCommand('S');
    else if (key == 0xFF6897) currentState = STATE_LINE_FOLLOW;
    else if (key == 0xFF9867) currentState = STATE_OBSTACLE;
    else if (key == 0xFFB04F) currentState = STATE_CAR_FOLLOW;
  }
}

// ========================== COMMAND LOGIC ==========================
void executeCommand(char cmd) {
  switch (cmd) {
    case 'F': case 'f': case '8':
      currentState = STATE_MANUAL;
      moveForward(speedCruise);
      break;

    case 'B': case 'b': case '2':
      currentState = STATE_MANUAL;
      moveBackward(speedCruise);
      break;

    case 'L': case 'l': case '4':
      currentState = STATE_MANUAL;
      turnLeft(speedTurn);
      break;

    case 'R': case 'r': case '6':
      currentState = STATE_MANUAL;
      turnRight(speedTurn);
      break;

    case 'S': case 's': case '0':
      currentState = STATE_STOP;
      stopMotors();
      break;

    case 'X':
      currentState = STATE_LINE_FOLLOW;
      break;

    case 'Y':
      currentState = STATE_OBSTACLE;
      break;

    case 'Z':
      currentState = STATE_CAR_FOLLOW;
      break;
  }
}

// ========================== AUTONOMOUS MODES ==========================
void runObstacleAvoidance() {
  long dist = getDistance();

  if (dist < 25 || digitalRead(PIN_OBS_L) == LOW || digitalRead(PIN_OBS_R) == LOW) {
    stopMotors();
    delay(200);

    moveBackward(150);
    delay(300);
    stopMotors();

    headServo.write(170);
    delay(400);
    int leftDist = getDistance();

    headServo.write(10);
    delay(400);
    int rightDist = getDistance();

    headServo.write(90);
    delay(200);

    if (leftDist > rightDist) {
      turnLeft(150);
    } else {
      turnRight(150);
    }
    delay(600);
  } else {
    moveForward(130);
  }
}

void runLineFollower() {
  int L = digitalRead(PIN_LINE_L);
  int M = digitalRead(PIN_LINE_M);
  int R = digitalRead(PIN_LINE_R);

  if (M == HIGH) {
    moveForward(110);
  } else if (L == HIGH) {
    turnLeft(140);
  } else if (R == HIGH) {
    turnRight(140);
  } else {
    stopMotors();
  }
}

void runCarFollower() {
  long d = getDistance();
  int buffer = 5;

  if (d == 0 || d > 60) {
    stopMotors();
  } else if (d > followDist + buffer) {
    moveForward(130);
  } else if (d < followDist - buffer) {
    moveBackward(130);
  } else {
    stopMotors();
  }
}

// ========================== HELPERS ==========================
long getDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long dur = pulseIn(PIN_ECHO, HIGH, 25000);
  if (dur == 0) return 999;
  return dur * 0.034 / 2;
}

// ========================== MOTOR CONTROL ==========================
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
