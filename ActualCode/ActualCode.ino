#include <Arduino.h>
#include <Servo.h>
#include "pinNumbers.h"
#include "constants.h"

int sensors[8]     = {0};
int firstLineIndex = -1;
int lastLineIndex  = -1;
int targetIndex    =  3;
int amountSeen     = 0;
bool running       = false;
bool turning       = false;
bool on            = false;

int state      = 0;
int turnIndex  = 0;
int iterations = 0;

Servo rightArm;
Servo leftArm;

void setup() {

  // initialize line sensors
  for(int i = 0; i < 8; i++)
  {
    pinMode(LINE_SENSOR[i], INPUT);
  }
  
  // initialize motor controllers
  pinMode(WHEEL_DIR_LB, OUTPUT);
  pinMode(WHEEL_DIR_LF, OUTPUT);
  pinMode(WHEEL_DIR_RB, OUTPUT);
  pinMode(WHEEL_DIR_RF, OUTPUT);

  pinMode(LEDY, OUTPUT);
  pinMode(LEDG, OUTPUT);

  pinMode(WHEEL_SPEED_L, OUTPUT);
  pinMode(WHEEL_SPEED_R, OUTPUT);
  
  pinMode(WHEEL_STBY, OUTPUT);

  pinMode(BUTTON1, INPUT_PULLUP);

  rightArm.attach(R_SCOOP);
  leftArm.attach(L_SCOOP);

  /*
   * The following output configurations set both motors 
   * to move forward. The two Robot solution doesn't require
   * backwards movement, so the wheels should be permanently
   * forward.
   *
   * - Note: when the new boards are installed, this code will be 
   *        changed because the board is going to tie these to high
   *  - Issues here the right wheel is only spinning backwards
   */
  writeWheelDirection(true);
  digitalWrite(WHEEL_STBY  , HIGH);

  Serial.begin(115200);
  Serial3.begin(115200);
  Serial3.write("Does this thing really work?!");
}

void readLine() {
  amountSeen = 0;
  lastLineIndex = -1;
  for(int i = 7; i >= 0; --i) {
    sensors[i] = digitalRead(LINE_SENSOR[i]);
    if(sensors[i] == HIGH) {
      if(lastLineIndex == -1)
      {
        lastLineIndex = i;
      }
      ++amountSeen;
      firstLineIndex = i;
    }
  }
}

void writeWheelDirection(bool dir) {
  if(dir) {
    digitalWrite(WHEEL_DIR_LB, LOW );
    digitalWrite(WHEEL_DIR_LF, HIGH);
    digitalWrite(WHEEL_DIR_RF, HIGH);
    digitalWrite(WHEEL_DIR_RB, LOW );
  } else {
    digitalWrite(WHEEL_DIR_LB, HIGH);
    digitalWrite(WHEEL_DIR_LF, LOW);
    digitalWrite(WHEEL_DIR_RF, LOW);
    digitalWrite(WHEEL_DIR_RB, HIGH);
  }
}

void writeToWheels(int ls, int rs) {
  analogWrite(WHEEL_SPEED_L, ls);
  analogWrite(WHEEL_SPEED_R, rs);
}

bool lineFollow(int ts, int strictness) {
  int offset = firstLineIndex - TARGET_INDEX;
  int rightSpeed = ts - offset*strictness;
  int leftSpeed = ts + offset*strictness;
  writeToWheels(leftSpeed, rightSpeed);

  //return true if line follow state is done
  return amountSeen > TURN_AMOUNT;
}

bool turn(int ts, int strictness, char direction) {
  if(direction == LEFT){
    writeToWheels(ts - strictness, ts + strictness);
    return firstLineIndex >= TARGET_INDEX  && amountSeen < 3;
  }else{
    writeToWheels(ts + strictness, ts - (strictness / 2));
    return firstLineIndex <= TARGET_INDEX + 1 && amountSeen < 3;
  }
}

bool delayState(int ms) {
  static int milliseconds = -1;
  if(milliseconds == -1) {
    milliseconds = millis();
  }
  else if(millis() - milliseconds >= ms) {
    milliseconds = -1;
    return true;
  }
  return false;
}

bool swoopTurn(char dir, int ts, int d) {
  if(dir == LEFT) {
    writeToWheels(0, ts);
  } else {
    writeToWheels(ts, 0);
  }

  return delayState(d);
}

void loop() {
  readLine();
  switch(state)
  {
    case 0:
      writeToWheels(0, 0);
      if(digitalRead(BUTTON1) == LOW) {
        state = 2;
        turnIndex = 0;
      }
      break;
    case 1:

      switch(turnIndex) {
        case 3:
          rightArm.write(50);
          break;
        case 10:
          leftArm.write(145);
          break;

        default:
          rightArm.write(100);
          leftArm.write(95);
          break;
          
      }
  
      if(turnIndex == 14)
      {
        state++;
      }
      if(turning) {
        if(turn(125, 125, TURN_SEQUENCE[turnIndex])) {
          turning = false;
          turnIndex++;
        }
      } else {
        turning = lineFollow(150, 20);
      }
      break;
    case 2:
      writeWheelDirection(false);
      if(swoopTurn(RIGHT, 255, 2000))
      {
        state = 0;
      }
      break;
  }

  iterations++;
  if(iterations == BLUETOOTH_LIMITER)
  {
    iterations = 0;
    Serial3.print(state);
    Serial3.print(":");
    Serial3.print(turning);
    Serial3.print(" - [ ");
    Serial3.print(firstLineIndex);
    Serial3.print(", ");
    Serial3.print(lastLineIndex);
    Serial3.print(", ");
    Serial3.print(amountSeen);
    Serial3.println(" ]");
  }
}
