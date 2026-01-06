# AutoTrack: Intelligent Autonomous Car with Hybrid Navigation

**AutoTrack** is a differential-drive robotic vehicle capable of seamless transitions between autonomous behaviors (Line Following, Obstacle Avoidance, Object Tracking) and manual Bluetooth control. Developed as a Complex Engineering Activity (CEA) for the Electrical Network Analysis course.
Auto track Basic Version & Auto Track Advance Version included
![AutoTrack Front View](Front view.jpg)

## 🚀 Key Features
* **Hybrid Navigation:** Finite State Machine (FSM) architecture allows switching between modes instantly.
* **Line Following:** 3-sensor array with logic for sharp turn correction.
* **Obstacle Avoidance:** Ultrasonic + IR fusion with servo-scanning to map free space and escape corners.
* **Car Follower:** PID-like distance regulation to shadow a moving target (20cm safety distance).
* **Bluetooth Control:** Manual override via Android app using HC-05 module.

## 🛠️ Hardware Components
* **Microcontroller:** Arduino Uno R3 (ATmega328P)
* **Motor Driver:** L298N Dual H-Bridge
* **Sensors:** * 3x IR Line Tracking Modules
    * 2x IR Obstacle Sensors
    * 1x HC-SR04 Ultrasonic Sensor
* **Actuators:** 4x DC Gear Motors, 1x SG90 Servo
* **Power:** 3-Cell Li-ion Battery Pack (11.1V)

## 🔌 Pin Mapping
| Interface | Pin | Function |
| :--- | :--- | :--- |
| **Left Motor** | D6, D11 | PWM Speed & Direction |
| **Right Motor** | D5, D3 | PWM Speed & Direction |
| **Servo** | D9 | Ultrasonic Rotation |
| **Sonar** | D2 (Trig), D4 (Echo) | Distance Ranging |
| **Line Sensors** | A0, A1, A2 | L/M/R Trajectory |
| **Bluetooth** | D0 (RX), D1 (TX) | Remote Command |

## 📂 Project Structure
* `AutoTrack_Main.ino`: The complete C++ firmware for the robot.
* `Project_Report.pdf`: Detailed documentation including circuit diagrams, algorithms, and cost analysis.

## 👥 Team Members
* **Faizan E Mustafa** (24F-6018)
* **Hammad Ahmad** (24F-6120)
* **Usama Tariq** (24F-6056)
* **Muhammad Soban** (24F-6047)

## 🎓 Acknowledgments
Submitted to **Dr. Irfan Ishaq** (Department of Electrical Engineering, FAST NUCES Chiniot-Faisalabad Campus).
