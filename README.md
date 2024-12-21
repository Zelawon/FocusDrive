# FocusDrive

## Description
FocusDrive is an Cyber-Physical System designed to enhance road safety by detecting and mitigating driving distractions. 
By leveraging sensor data analysis, the system identifies risky behaviors such as head tilts, sharp steering wheel turns, and unsafe following distances. 
It then triggers corrective actions like activating warning lights or automatic braking to prevent accidents.

### Features
- **Sensors:**
  - Gyroscope: Detects head tilt.
  - Steering wheel rotation sensor: Detects erratic driving.
  - Speedometer: Monitors vehicle speed.
  - Ultrasonic distance sensor: Ensures safe distance from other vehicles.
- **Actuators:**
  - Warning lights (Yellow and Red LEDs) for risk levels.
  - Display for driver notifications.
  - Automatic braking system with multiple intensity levels.

## System Requirements
- Raspberry Pi 3 Model B
- Raspberry Pi OS
- Python 3.9 or later
- The sensors and actuators

### Pin Connections
| Component           | GPIO Pin (Pin)     | Description   |
|---------------------|--------------------|---------------|
| Ultrasonic Trigger  | GPIO17 (Pin 6)     | Output        |
| Ultrasonic Echo     | GPIO27 (Pin 5)     | Input         |
| Infrared Sensor     | GPIO25             | Input         |
| Button              | GPIO17             | Input         |
| LED 1               | GPIO23             | Output        |
| LED 2               | GPIO24             | Output        |
| Servo Motor         | GPIO13             | PWM Output    |
| ADC                 | SPI Pins           | Data transfer |
| Accelerometer       | I2C Pins (SDA/SCL) | Data transfer |

### GPIO Connections
![Raspberry Pi GPIO Connections](https://github.com/Zelawon/FocusDrive/blob/main/images/gpio_connections_diagram.png?raw=true)


## Installation
1. **Clone the Repository**
   ```bash
   git clone https://github.com/Zelawon/FocusDrive.git
   cd FocusDrive
   ```

2. **Build the Code**
   Compile the necessary C files:
   ```bash
   make C
   ```

3. **Run the Application**
   After building the code, run the threads executable:
   ```bash
   sudo ./threads
   ```

## Usage

This system monitors driver behavior and vehicle conditions to detect potential risk situations using sensor data. The system performs the following:

### Symptoms Detection:
- **Head Tilt (S1)**: The Gyro is read every **400 ms**. If the tilt in the X-axis is greater than 30° in at least two consecutive readings, it is interpreted as possible drowsiness or distraction.
- **Safety Distance (D1, D2, D3)**: The system measures the distance every **300 ms**. It will identify:
  - **D1**: Insecure Distance (distance < (Speed / 10)^2).
  - **D2**: Imprudent Distance (distance < half of safe distance).
  - **D3**: Danger Collision (distance < one-third of safe distance).
- **Sharp Turns (S2)**: The position of the steering wheel is read every **350 ms**. If there are sharp turns (over 20°) at speeds above 40 km/h, it indicates erratic driving behavior.

### Actions:
- **Light 1**: Turns on when **(S1 or S2)** and **D0**.
- **Light 2**: Turns on when **(S1 and S2)** and **D0**.
- **Brake Level 1**: Activated when **(S1 or S2)** and **D1**.
- **Brake Level 2**: Activated when **(S1 or S2)** and **D2**.
- **Brake Level 3**: Activated when **D3**.

The system continuously monitors these conditions every **200 ms** and takes actions as needed until the risk situations are resolved.

### Process Map Image
![Process Map](https://github.com/Zelawon/FocusDrive/blob/main/images/process_map.png?raw=true)

## Safety Warnings:
- This system is designed for experimental and educational purposes
- Do not rely solely on this system for vehicle safety
- Always follow proper vehicle safety guidelines and regulations
- Test all features thoroughly in a safe environment before actual use

## License
This project is licensed under the GPL-3.0 License. See the `LICENSE` file for details.
