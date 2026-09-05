An IoT-based smart irrigation system built on the ESP32-S3, combining real-time environmental sensing, remote monitoring, and an Edge AI–driven crop detection layer to automate watering decisions. Built as the capstone project for an industrial training program.
Features
-Automated pump control based on real-time soil moisture, with a rain-sensor override to prevent watering during rainfall
-Environmental monitoring via a DHT11 temperature and humidity sensor
-Motion/intrusion alerting via a PIR sensor
-Remote monitoring & control through the Blynk IoT app — live gauges for all sensors, pump status, and manual override controls
-Crop-specific irrigation: a web interface accepts a phone-captured crop image, classifies the crop type using an Edge Impulse–trained model, and adjusts the soil-moisture watering threshold to match that crop's ideal requirement (e.g., wheat at ~70%)

Software & Tools
-Wokwi / Proteus — circuit simulation and validation before/alongside physical wiring
-Arduino IDE — firmware development (C++)
-Blynk IoT — mobile app dashboard for live monitoring and manual control
-Edge Impulse — training the image classification models (crop type, crop health)
-Firebase Realtime Database — storing crop-health classification results
