# Mini-Sumo-Robot-Competition

The objective of this project is to design, build and test two autonomous mini-sumo robots which aim to push each other out of the ring.
Its focus is intended to be towards embedded software and sensors, more specifically.


# 1. Idea overview

Two self-controlled robots are placed in a ring. The robots try to avoid falling out or avoid being pushed out by the opponent robot. The first robot that touches outside of the ring loses the round.

Autonomous Sumo robots are self-propelled and self-controlled, without tethers.
After positioning and starting the robot, no remote control, power, positioning, or other help can be provided. The robot must care for itself until the round ends.

The first robot to win two rounds, wins the match. Different robots compete one-on-one against each other throughout the contest. The robot that wins the most matches wins the contest.

# 2. The ring
Unlike most mini-sumo competitions, the ring used for this project is white with black borders. For the purpose of the project, the robots are programmed to function on that board. All of this is done for convenience and rapidity.

# 3. Robots 

This project features two Pololu Zumo Robots for Arduino v1.2.

https://user-images.githubusercontent.com/60289976/115118357-6fde5d80-9fab-11eb-91cf-018b5d353d87.mp4

The Zumo robot is a low-profile tracked robot platform intended for use with an Arduino as its main controller. It measures less than 10 cm on each side and weighs approximately 300 g with an Arduino Uno and batteries, so it is both small enough and light enough to qualify for Mini-Sumo competitions.


They both have two micro metal gearmotors coupled to a pair of silicone tracks, a stainless steel bulldozer-style blade, an array of six infrared reflectance sensors for line following or edge detection, a 3-axis accelerometer and magnetometer, and a buzzer.


The robots's hardware and software are slightly different. Here we will look at both of them:

# The MoonWalker

https://user-images.githubusercontent.com/60289976/115119066-4a535300-9faf-11eb-92d9-e85514a0a266.mp4

1. Hardware 
  - Arduino Uno
  - Pololu Zumo Shield
  - 2 micro metal gearmotors 
  - dual motor drivers
  - 2 silicone tracks 
  - stainless steel blade
  - IR reflectance sensor array
  - 3-axis accelerometer 
  - buzzer  
  - 4 AA batteries
 
 
2. Software
   
MoonWalker's software uses the data given by the sensors attached to the Arduino Uno to perform the following actions, listed by importance:
- navigating in the scopes of the ring indefinitely without going out of it, using the leftmost and the rightmost sensors on the reflectance sensor array.
- attacking the opponent if contact is detected by the 3-axis accelerometer 
- scanning the ring 
  

# The MarsWalker

https://user-images.githubusercontent.com/60289976/115119076-50493400-9faf-11eb-9055-bb6edd04076b.mp4

1. Hardware 
  - Arduino Uno
  - Pololu Zumo Shield
  - 2 micro metal gearmotors 
  - dual motor drivers
  - 2 silicone tracks 
  - stainless steel blade
  - IR reflectance sensor array 
  - HC-SR04 ultrasonic distance sensor
  - buzzer  
  - 4 AA batteries


2. Software
MarsWalker's software uses the data given by the sensors attached to the Arduino Uno to perform the following actions, listed by importance:
- navigating in the scopes of the ring indefinitely without going out of it, using the leftmost and the rightmost sensors on the reflectance sensor array.
- attacking the opponent if detected by the ultrasonic distance sensor
- scanning the ring while trying to detect the opponent

# Contest 

https://user-images.githubusercontent.com/60289976/115122732-22211f80-9fc2-11eb-80a9-90d2e445cabc.mp4




