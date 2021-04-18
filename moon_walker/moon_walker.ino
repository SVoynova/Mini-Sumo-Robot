#include <Wire.h>
#include <ZumoShield.h>

#define LED 13
ZumoMotors motors; // Motor settings
Pushbutton button(ZUMO_BUTTON); // pushbutton on pin 12
ZumoBuzzer buzzer; // sound effects

// Accelerometer settings
#define RA_SIZE 3  // number of readings to include in running average of accelerometer readings
#define XY_ACCELERATION_THRESHOLD 2400  // for detection of contact

// Reflectance Sensor settings
#define NUM_SENSORS 6
unsigned int sensor_values[NUM_SENSORS];
#define QTR_THRESHOLD  1500 // ms
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

// initializing constants
#define REVERSE_SPEED     200 
#define TURN_SPEED        200
#define FORWORD_SPEED     200
#define FULL_SPEED        400
#define REVERSE_DURATION  200 // ms
#define TURN_DURATION     300 // ms
#define STOP_DURATION     100 // ms
#define FULL_SPEED_DURATION_LIMIT     250  // ms 

// time managing
unsigned long startTime;
unsigned long lastTurnTime;
unsigned long contactTime;
unsigned long fullSpeedTime;
#define MIN_DELAY_AFTER_TURN          400  // ms 
#define MIN_DELAY_BETWEEN_CONTACTS   1000  // ms 

// RunningAverage class (based on RunningAverage library for Arduino)
template <typename Time>
class RunningAverage
{
  public:
    RunningAverage(void);
    RunningAverage(int);
    ~RunningAverage();
    void clear();
    void addValue(Time);
    Time getAverage() const;
    void fillValue(Time, int);
  protected:
    int _size;
    int _cnt;
    int _idx;
    Time _sum;
    Time * _ar;
    static Time zero;
};

// Accelerometer Class -- extends the ZumoIMU class to support reading and averaging the x-y acceleration
class Accelerometer : public ZumoIMU
{
  typedef struct acc_data_xy
  {
    unsigned long timestamp;
    int x;
    int y;
    float dir;
  } acc_data_xy;

  public:
    Accelerometer() : ra_x(RA_SIZE), ra_y(RA_SIZE) {};
    ~Accelerometer() {};
    void getLogHeader(void);
    void readAcceleration(unsigned long timestamp);
    float len_xy() const;
    float dir_xy() const;
    int x_avg(void) const;
    int y_avg(void) const;
    long ss_xy_avg(void) const;
    float dir_xy_avg(void) const;
  private:
    acc_data_xy last;
    RunningAverage<int> ra_x;
    RunningAverage<int> ra_y;
};

Accelerometer acc;
bool contact;  // set when accelerometer detects contact with opposing robot

void setup()
{
  // initialize the Wire library
  Wire.begin();

  // initialize accelerometer
  acc.init();
  acc.enableDefault();

  // initialize the reflectance sensor
  sensors.init();
  
  randomSeed((unsigned int) millis());

  // initial calibration
  digitalWrite(LED, HIGH);
  buzzer.playMode(">g32>>c32");
  button.waitForButton();
  CalibrateReflectanceSensor();
  CountDownAndResetLoop(false);
}

void loop()
{
  if (button.isPressed())
  {
    // if button is pressed, stop and wait for another press to go again
    motors.setSpeeds(0, 0);
    button.waitForRelease();
    CountDownAndResetLoop(true);
  }

  startTime = millis();
  acc.readAcceleration(startTime);
  sensors.read(sensor_values);

  if ((startTime - fullSpeedTime > FULL_SPEED_DURATION_LIMIT))
  {
    motors.setSpeeds(FULL_SPEED,FULL_SPEED);
  }
  
  BlackBorderDetection();
}

// calibrating the reflection sensor
void CalibrateReflectanceSensor()
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
    sensors.calibrate();
    delay(20);
  }
  motors.setSpeeds(0,0);

  // Indicate the end of the calibration proccess
  digitalWrite(LED, LOW);
  buzzer.play(">g32>>c32");
}

// count down before beging the action
void CountDownAndResetLoop(bool restarting)
{
  digitalWrite(LED, HIGH);
  button.waitForButton();
  digitalWrite(LED, LOW);
  
  // play audible countdown
  for (int i = 0; i < 3; i++)
  {
    delay(1000);
    buzzer.playNote(NOTE_G(3), 200, 15);
  }
  delay(1000);
  buzzer.playNote(NOTE_G(4), 500, 15);
  delay(1000);

  // reset loop variables
  contact = false;  
  contactTime = 0;
  lastTurnTime = millis();
  fullSpeedTime = 0;
}

// turn to the left
void LeftTurn( bool randomize)
{
  static unsigned int incr = TURN_DURATION / 4;
  
  motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
  delay(REVERSE_DURATION);
  motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
  delay(randomize ? TURN_DURATION + (random(8) - 2) * incr : TURN_DURATION);
  motors.setSpeeds(FORWORD_SPEED, FORWORD_SPEED);
  lastTurnTime = millis();

  // assume contact lost
  ContactLost();
}

// turn to the right
void RightTurn( bool randomize)
{
  static unsigned int incr = TURN_DURATION / 4;

  motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
  delay(REVERSE_DURATION);
  motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
  delay(randomize ? TURN_DURATION + (random(8) - 2) * incr : TURN_DURATION);
  motors.setSpeeds(FORWORD_SPEED, FORWORD_SPEED);
  lastTurnTime = millis();

  // assume contact lost
  ContactLost();
}

// detecting the black borders and avoiding them
void BlackBorderDetection()
{
  if (sensor_values[0] > QTR_THRESHOLD)
  {
    // if leftmost sensor detects line, reverse and turn to the right
    RightTurn(true);
  }
  else if (sensor_values[5] > QTR_THRESHOLD)
  {
    // if rightmost sensor detects line, reverse and turn to the left
    LeftTurn(true);
  }
  else  // otherwise, go straight
  {
    if (CheckForContact())
    {
      ContactMade();
    }
    else
    {
    motors.setSpeeds(FORWORD_SPEED, FORWORD_SPEED);
    }
  }
}

// check for contact, ignoring readings immediately after turning or losing contact
bool CheckForContact()
{
  static long threshold_squared = (long) XY_ACCELERATION_THRESHOLD * (long) XY_ACCELERATION_THRESHOLD;
  return (acc.ss_xy_avg() >  threshold_squared) && \
    (startTime - lastTurnTime > MIN_DELAY_AFTER_TURN) && \
    (startTime - contactTime > MIN_DELAY_BETWEEN_CONTACTS);
}

// buzz and accelerate on contact
void ContactMade()
{
  contact = true;
  contactTime = startTime;
  buzzer.play(">g32>>c32");
  motors.setSpeeds(FULL_SPEED,FULL_SPEED);
  delay(300);
  
}

// reset forward speed
void ContactLost()
{
  contact = false;
  motors.setSpeeds(FORWORD_SPEED,FORWORD_SPEED);
}


// class Accelerometer -- member function definitions
void Accelerometer::readAcceleration(unsigned long timestamp)
{
  readAcc();
  if (a.x == last.x && a.y == last.y) return;

  last.timestamp = timestamp;
  last.x = a.x;
  last.y = a.y;

  ra_x.addValue(last.x);
  ra_y.addValue(last.y);
}

float Accelerometer::len_xy() const
{
  return sqrt(last.x*a.x + last.y*a.y);
}

float Accelerometer::dir_xy() const
{
  return atan2(last.x, last.y) * 180.0 / M_PI;
}

int Accelerometer::x_avg(void) const
{
  return ra_x.getAverage();
}

int Accelerometer::y_avg(void) const
{
  return ra_y.getAverage();
}

long Accelerometer::ss_xy_avg(void) const
{
  long x_avg_long = static_cast<long>(x_avg());
  long y_avg_long = static_cast<long>(y_avg());
  return x_avg_long*x_avg_long + y_avg_long*y_avg_long;
}

float Accelerometer::dir_xy_avg(void) const
{
  return atan2(static_cast<float>(x_avg()), static_cast<float>(y_avg())) * 180.0 / M_PI;
}

// RunningAverage class
template <typename Time>
Time RunningAverage<Time>::zero = static_cast<Time>(0);

template <typename Time>
RunningAverage<Time>::RunningAverage(int n)
{
  _size = n;
  _ar = (Time*) malloc(_size * sizeof(Time));
  clear();
}

template <typename Time>
RunningAverage<Time>::~RunningAverage()
{
  free(_ar);
}

// resets all counters
template <typename Time>
void RunningAverage<Time>::clear()
{
  _cnt = 0;
  _idx = 0;
  _sum = zero;
  for (int i = 0; i< _size; i++) _ar[i] = zero;  // needed to keep addValue simple
}

// adds a new value to the data-set
template <typename Time>
void RunningAverage<Time>::addValue(Time f)
{
  _sum -= _ar[_idx];
  _ar[_idx] = f;
  _sum += _ar[_idx];
  _idx++;
  if (_idx == _size) _idx = 0;  // faster than %
  if (_cnt < _size) _cnt++;
}

// returns the average of the data-set added so far
template <typename Time>
Time RunningAverage<Time>::getAverage() const
{
  if (_cnt == 0) return zero; // NaN ?  math.h
  return _sum / _cnt;
}

// fill the average with a value
// the param number determines how often value is added (weight)
// number should preferably be between 1 and size
template <typename Time>
void RunningAverage<Time>::fillValue(Time value, int number)
{
  clear();
  for (int i = 0; i < number; i++)
  {
    addValue(value);
  }
}
