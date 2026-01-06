/* AutoTrack PRO EDITION
Smart Line Follower: Adds Collision Detection (stops for walls) and Memory (remembers direction for sharp turns).
Smart Car Follower: Adds "Search Mode". If it loses you, it looks Left/Right to find you again.
Smart Obstacle Avoidance: Adds "Anti-Trap" logic. If it gets stuck in a corner, it performs a U-Turn escape.
Smoother Turns: Adjusted turn speeds to reduce wobbling.
*/

#include <Servo.h>
#include <IRremote.h>

// ========================== PIN DEFINITIONS (DIRECT WIRING) ==========================
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

// ========================== INTELLIGENCE VARIABLES ==========================
// Memory for Line Follower (Where was the line last seen?)
int lastLineDirection = 0; // -1 = Left, 1 = Right, 0 = Center

// Anti-Trap Memory for Obstacle Mode
unsigned long lastObstacleTime = 0;
int obstacleCounter = 0;

// Tuning
int speedCruise = 140; 
int speedTurn   = 170; 
int followDist  = 20;   

// ========================== SETUP ==========================
void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_L_FWD, OUTPUT); pinMode(MOTOR_L_BWD, OUTPUT);
  pinMode(MOTOR_R_FWD, OUTPUT); pinMode(MOTOR_R_BWD, OUTPUT);

  pinMode(PIN_LINE_L, INPUT); pinMode(PIN_LINE_M, INPUT); pinMode(PIN_LINE_R, INPUT);
  pinMode(PIN_OBS_L, INPUT); pinMode(PIN_OBS_R, INPUT);
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);

  headServo.attach(PIN_SERVO);
  headServo.write(90); 

  irrecv.enableIRIn();

  Serial.println("AutoTrack PRO Initialized.");
}

// ========================== MAIN LOOP ==========================
void loop() {
  checkInput(); 

  switch (currentState) {
    case STATE_MANUAL:      break;
    case STATE_LINE_FOLLOW: runLineFollowerSmart(); break;
    case STATE_OBSTACLE:    runObstacleAvoidanceSmart(); break;
    case STATE_CAR_FOLLOW:  runCarFollowerSmart(); break;
    case STATE_STOP:        stopMotors(); break;
  }
}

// ========================== INPUT HANDLING ==========================
void checkInput() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if(cmd != '\n' && cmd != '\r') executeCommand(cmd);
  }

  if (irrecv.decode(&results)) {
    unsigned long key = results.value;
    irrecv.resume();
    
    // REPLACE THESE HEX CODES WITH YOUR REMOTE'S CODES
    if (key == 0xFF629D) executeCommand('F');      
    else if (key == 0xFFA857) executeCommand('B'); 
    else if (key == 0xFF22DD) executeCommand('L'); 
    else if (key == 0xFFC23D) executeCommand('R'); 
    else if (key == 0xFF02FD) executeCommand('S'); 
    
    else if (key == 0xFF6897) { 
      currentState = STATE_LINE_FOLLOW; 
      Serial.println("Mode: Smart Line Follower"); 
    }
    else if (key == 0xFF9867) { 
      currentState = STATE_OBSTACLE; 
      Serial.println("Mode: Smart Obstacle"); 
      obstacleCounter = 0; // Reset trap counter
    }
    else if (key == 0xFFB04F) { 
      currentState = STATE_CAR_FOLLOW; 
      Serial.println("Mode: Smart Car Follower"); 
    }
  }
}

void executeCommand(char cmd) {
  if (strchr("FBLR2468fb", cmd)) currentState = STATE_MANUAL;

  switch (cmd) {
    case 'F': case '8': moveForward(speedCruise); break;
    case 'B': case '2': moveBackward(speedCruise); break;
    case 'L': case '4': turnLeft(speedTurn); break;
    case 'R': case '6': turnRight(speedTurn); break;
    case 'S': case '0': stopMotors(); currentState = STATE_STOP; break;
    
    case 'X': currentState = STATE_LINE_FOLLOW; break;
    case 'Y': currentState = STATE_OBSTACLE; obstacleCounter = 0; break;
    case 'Z': currentState = STATE_CAR_FOLLOW; break;
  }
}

// ========================== 1. SMART LINE FOLLOWER ==========================
// Fixes: Stop on Obstacle, Memory for Sharp Turns
void runLineFollowerSmart() {
  
  // FIX 1: Safety Check (Don't hit walls!)
  long safetyDist = getDistanceAvg();
  if (safetyDist > 0 && safetyDist < 15) {
    stopMotors();
    Serial.println("Line Follower: Obstacle Detected! Stopping.");
    return;
  }

  int L = digitalRead(PIN_LINE_L);
  int M = digitalRead(PIN_LINE_M);
  int R = digitalRead(PIN_LINE_R);

  // Assumption: Black = LOW (0), White = HIGH (1)
  
  if (M == LOW) {
    moveForward(110);
    lastLineDirection = 0; // We are centered
  } 
  else if (L == LOW) {
    turnLeft(140);
    lastLineDirection = -1; // Remember line was Left
  } 
  else if (R == LOW) {
    turnRight(140);
    lastLineDirection = 1;  // Remember line was Right
  } 
  else {
    // FIX 2: Memory Logic for Sharp Turns
    // If all sensors see White, don't stop. Spin in the last known direction.
    if (lastLineDirection == -1) {
       turnLeft(140); // Spin Left to find line
    } 
    else if (lastLineDirection == 1) {
       turnRight(140); // Spin Right to find line
    } 
    else {
       stopMotors(); // Lost completely
    }
  }
}

// ========================== 2. SMART OBSTACLE AVOIDANCE ==========================
// Fixes: Corner Trap (Stuck Detection)
void runObstacleAvoidanceSmart() {
  long dist = getDistanceAvg();
  int irL = digitalRead(PIN_OBS_L);
  int irR = digitalRead(PIN_OBS_R);

  if ((dist > 0 && dist < 25) || irL == LOW || irR == LOW) {
    stopMotors(); delay(100);
    
    // FIX: Anti-Trap Logic
    // If we hit obstacles rapidly (within 3 seconds), increase counter
    unsigned long now = millis();
    if (now - lastObstacleTime < 3000) {
      obstacleCounter++;
    } else {
      obstacleCounter = 0; // Reset if it's been a while
    }
    lastObstacleTime = now;

    if (obstacleCounter >= 3) {
      // TRAP DETECTED! Do a U-Turn Escape
      Serial.println("Stuck in Corner! Escaping...");
      moveBackward(150); delay(600); // Long back up
      turnRight(180); delay(800);    // 180 degree spin
      obstacleCounter = 0;
      return;
    }

    // Standard Avoidance
    moveBackward(140); delay(300);
    stopMotors();

    // Look Left/Right
    headServo.write(170); delay(400);
    long leftDist = getDistanceAvg();
    
    headServo.write(10); delay(400);
    long rightDist = getDistanceAvg();
    
    headServo.write(90); delay(200);

    if (leftDist > rightDist) {
      turnLeft(speedTurn); delay(500);
    } else {
      turnRight(speedTurn); delay(500);
    }
  } else {
    moveForward(130);
  }
}

// ========================== 3. SMART CAR FOLLOWER ==========================
// Fixes: Search Mode when target lost
void runCarFollowerSmart() {
  long d = getDistanceAvg();
  int buffer = 5;

  if (d == 0 || d > 60) {
    // FIX: Search Mode
    // Instead of stopping, quickly check Left and Right
    stopMotors();
    
    // Quick glance Left
    headServo.write(140); delay(250);
    long scanL = getDistanceAvg();
    if (scanL > 0 && scanL < 50) {
      turnLeft(150); delay(200); // Found you on Left! Turn there.
      headServo.write(90);
      return;
    }

    // Quick glance Right
    headServo.write(40); delay(250);
    long scanR = getDistanceAvg();
    if (scanR > 0 && scanR < 50) {
      turnRight(150); delay(200); // Found you on Right!
      headServo.write(90);
      return;
    }

    headServo.write(90); // Reset and wait
  } 
  else if (d > followDist + buffer) {
    moveForward(120); 
  } 
  else if (d < followDist - buffer) {
    moveBackward(120); 
  } 
  else {
    stopMotors(); 
  }
}

// ========================== HELPERS ==========================
long getDistanceAvg() {
  long sum = 0;
  int validReadings = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    long duration = pulseIn(PIN_ECHO, HIGH, 15000); // 15ms timeout
    if (duration > 0) {
      sum += duration;
      validReadings++;
    }
    delay(5);
  }
  if (validReadings == 0) return 999;
  return (sum / validReadings) * 0.034 / 2;
}

// ========================== MOTOR CONTROL ==========================
void moveForward(int s) {
  analogWrite(MOTOR_L_FWD, s); digitalWrite(MOTOR_L_BWD, LOW);
  analogWrite(MOTOR_R_FWD, s); digitalWrite(MOTOR_R_BWD, LOW);
}
void moveBackward(int s) {
  digitalWrite(MOTOR_L_FWD, LOW); analogWrite(MOTOR_L_BWD, s);
  digitalWrite(MOTOR_R_FWD, LOW); analogWrite(MOTOR_R_BWD, s);
}
void turnLeft(int s) {
  digitalWrite(MOTOR_L_FWD, LOW); analogWrite(MOTOR_L_BWD, s);
  analogWrite(MOTOR_R_FWD, s); digitalWrite(MOTOR_R_BWD, LOW);
}
void turnRight(int s) {
  analogWrite(MOTOR_L_FWD, s); digitalWrite(MOTOR_L_BWD, LOW);
  digitalWrite(MOTOR_R_FWD, LOW); analogWrite(MOTOR_R_BWD, s);
}
void stopMotors() {
  digitalWrite(MOTOR_L_FWD, LOW); digitalWrite(MOTOR_L_BWD, LOW);
  digitalWrite(MOTOR_R_FWD, LOW); digitalWrite(MOTOR_R_BWD, LOW);
}