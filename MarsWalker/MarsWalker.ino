#include <ZumoShield.h>
#include <Wire.h>
#include <NewPing.h>

//on-board LED
#define LED 13 

//reflectance sensor array
#define NUM_SENSORS 6 //number of sensors on the module
#define QTR_THRESHOLD  1500 //threshold in microseconds used for detecting surface's color

//ultrasonic sensor
#define ECHO_PIN     11   // Arduino pin tied to echo pin on the ultrasonic sensor
#define TRIGGER_PIN  5  // Arduino pin tied to trigger pin on the ultrasonic sensor 
#define MAX_DISTANCE 500 // Maximum range of the ultrasonic sensor
#define MAX_DISTANCE_TO_ACT  20 //The maximum distance in centimeters on which the robot recognizes the detected object as a tread


//setting different speeds in order to optimize movement
#define REVERSE_SPEED     300 
#define TURN_SPEED        300
#define SCAN_SPEED        250
#define SEARCH_SPEED      300
#define FORWARD_SPEED     400


// declare the objects used
ZumoReflectanceSensorArray reflectanceSensors(QTR_NO_EMITTER_PIN);
ZumoBuzzer buzzer;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);


// define some global variables
int searchDuration = 4000; //duration of a search proccess in milliseconds
int scanDuration = 3000; //duration of a scan proccess in milliseconds
int turnDuration = 300; //duration of a turn proccess in milliseconds
int reverseDuration = 200; //duration of a reverse proccess in milliseconds
unsigned int sensor_values[NUM_SENSORS]; //the values of the reflectance sensor array
unsigned long currentTime; //a variable used for storing the last current time in milliseconds
unsigned long searchTime; //a variable used for storing the last time of a search in milliseconds
unsigned long scanTime; //a variable used for storing the last time of a scan in milliseconds
char lastTurnsDirection = 'L'; //a variable containing the direction of the last turn
enum proccesses // define some processes as enumerables
{
  MOVEFORWARD,
  AVOIDBORDER,
  SEARCHOPPONENT
};
enum proccesses currentProccess; // an enum variable for storing the ongoing proccess


//Some functions that are defined below
void waitForButtonAndCountDown(bool countDown);
void CalibrateTheReflectanceSensorArray();
bool CheckForBorder();
void AvoidBorder();
bool CheckIfOpponentIsDetected();
void SearchForOpponent();
void MoveForward();


void setup()
{
  // switch on the on-board LED
  digitalWrite(LED, HIGH);
  
  // play a sound
  buzzer.play(">g32>>c32");

  // set the motors 
  motors.setSpeeds(0,0);

  // Initialize the reflectance sensor array
  reflectanceSensors.init();

  // wait for a button press in order to start calibrating
  digitalWrite(LED, HIGH);
  button.waitForButton();
  digitalWrite(LED, LOW);

  // initialise the sensor array
  CalibrateTheReflectanceSensorArray();
  
  // wait for another button press in order to start the competition
  waitForButtonAndCountDown();

  // set the search timer
  searchTime = millis();

  // initially set the current process to search
  currentProccess = SEARCHOPPONENT;
}

void loop()
{
   
  // increase the timer with the milliseconds passed since switched on 
  currentTime = millis(); 

  /* check which proccess should be performed:
  * not crossing the border is of highest importance 
  * only then we check for other circumstances
  * the attacking/moving forward process starts if there is an opponent detected
  * otherwise the robot starts searching/scanning the ring for the opponent 
  */
  if (CheckForBorder()) 
  {
    currentProccess = AVOIDBORDER;
  }
  else 
  {
    if (CheckIfOpponentIsDetected())
    {
      currentProccess = MOVEFORWARD;
    }
    else 
    {
      currentProccess = SEARCHOPPONENT;
    }
    
   }

   // take a correct action depending on what proccess the robot is in
  switch (currentProccess)
  {
    case AVOIDBORDER: AvoidBorder();
      break;
    case MOVEFORWARD: MoveForward();
      break;
    case SEARCHOPPONENT: SearchForOpponent();
      break;
  }
 }


/**
 * A function used for calibrating the reflectance sensor array 
 */
void CalibrateTheReflectanceSensorArray()
{
  // Turn on the LED to indicate a calibration proccess and wait 1 second
  digitalWrite(LED, HIGH);
  delay(1000);

  // Rotate in place to sweep the sensors over the line
  for(int i = 0; i < 80; i++)
  {
    if ((i > 10 && i <= 30) || (i > 50 && i <= 70))
    {
      motors.setSpeeds(-200, 200);
    }
    else
    {
      motors.setSpeeds(200, -200);
    }
    reflectanceSensors.calibrate();
    delay(20);
  }
  motors.setSpeeds(0,0);

  // Indicate the end of the calibration proccess
  digitalWrite(LED, LOW);
  buzzer.play(">g32>>c32");
}


/**
 * A function used to play some sounds and indicate the start
 */
void waitForButtonAndCountDown()
{
  digitalWrite(LED, HIGH);
  button.waitForButton();
  digitalWrite(LED, LOW);

  // playing a countdown
  for (int i = 0; i < 3; i++)
  {
    delay(1000);
    buzzer.playNote(NOTE_G(3), 200, 15);
  }
  delay(1000);
  buzzer.playNote(NOTE_G(4), 500, 15);
  delay(1000);
}


/**
 * Function used to check whether the robot has stepped on a border and stores where the border is
 * if any sensor detects a black line, the function returns true, otherwise returns false
 */
bool CheckForBorder()
{
  reflectanceSensors.read(sensor_values);
  
  if (sensor_values[5] > QTR_THRESHOLD )
  {
    lastTurnsDirection = 'R';
    return true;
  }
  else if (sensor_values[0] > QTR_THRESHOLD)
  {
    lastTurnsDirection = 'L';
    return true;
  }
  return false;
}


/**
 *Function used to make the robot avoid the border
 *if a black line is detected by the rightmost sensor, it calls the TurnRight() function to make the move
 *if a black line is detected by the leftmost sensor, it calls the TurnRight() function to make the move
 */
void AvoidBorder()
{
  motors.setSpeeds(0,0);
  
  //while there is a border under the robot
  while (CheckForBorder())
  {
    //the robot should reverse
    motors.setSpeeds(REVERSE_SPEED, REVERSE_SPEED);
  }
  
  //if a border is detected on the left
  if (lastTurnsDirection == 'L')
  {
    //the robot should turn to the right
    TurnRight();
  }
  //otherwise if a border is detected on the right
  else if (lastTurnsDirection == 'R')
  {
    //the robot should turn to the left
    TurnLeft();
  }  

  // pause for some time after a turn
  delay(turnDuration);

  // after making a turn the robot should start searching its opponent
  currentProccess = SEARCHOPPONENT;
}


/**
 * Function used to make a right turn 
 */
void TurnRight()
{
    motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
    delay(reverseDuration);
    motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
    delay(turnDuration);
    motors.setSpeeds(0, 0);
    
    //save the turn made into the variable 
    //so that the next move decision is made properly 
    //and the robot does not move in a circle endlessly
    lastTurnsDirection = 'R';
}


/**
 * Function used to make a left turn 
 */
void TurnLeft()
{
    motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
    delay(reverseDuration);
    motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
    delay(turnDuration);
    motors.setSpeeds(0, 0);
    
    //store the turn made into the variable 
    //so that the next move decision is made properly 
    //and the robot does not move in a circle endlessly
    lastTurnsDirection = 'L';
}

/*
 * Function used to make a forward move
 */
void MoveForward()
{
  motors.setSpeeds(FORWARD_SPEED,FORWARD_SPEED);
}


/** 
 * Function used to check wheter an opponent is being detected 
 * by the ultrasonic sensor in the defined max range
 */ 
bool CheckIfOpponentIsDetected()
{
  //check if the the opponent is detected by the sensor 
  if(sonar.ping_cm() <= MAX_DISTANCE_TO_ACT  &&  sonar.ping_cm() != 0)
  {
    return true;
  }
  
  return false;
}


/** 
 * Function used for searching for opponent on the ring/ scanning the ring 
 * it uses the timers which are being set in the beginning of each proccess 
 * and the defined durations of each proccess 
 * to estimate whether a procces has been done fully 
 * and to chose the next correct move
 */
void SearchForOpponent()
{
  // check if the time for moving forward has expired
    if (currentTime - searchTime >= searchDuration)
    {
      // check if the time for scanning has expired
      if (currentTime - scanTime <= scanDuration)
      {
        // make the zumo turn depending on the last turn's direction
        if (lastTurnsDirection == 'L')
        {
          motors.setSpeeds(SCAN_SPEED, SCAN_SPEED * 0.2);
        }
        else if (lastTurnsDirection == 'R')
        {
          motors.setSpeeds(SCAN_SPEED * 0.2, SCAN_SPEED);
        }
      }
      else
      {
        // reset the search timer
        searchTime = millis();
      }
    }
    else
    {
      // move forward with speed for search 
      motors.setSpeeds(SEARCH_SPEED,SEARCH_SPEED);
      
      // reset the scan timer
      scanTime = millis();
    }
}
