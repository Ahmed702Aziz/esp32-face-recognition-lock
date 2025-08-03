# esp32-face-recognition-lock

An online face recognition access control system using the ESP32-CAM AI Thinker module. This project uses Python,Local storage to detect and C++ for Streaming and Subscribing MQTT Messages , enroll, and recognize faces for unlocking doors using a relay. No cloud, no PC, and no Arduino required.



 # Overview

This project turns the ESP32-CAM into a smart security lock by:
- Detecting and recognizing faces in real-time
- Controlling a relay to unlock the door for authorized users
- Storing enrolled faces internally using SPIFFS or in local memory (PC or server)
- Performing all operations locally or remotly (with some configuration) [local server]

 

 # Features of Project

-  Relay-controlled door lock
-  Face enrollment and matching stored in local storage(PC or Server)
-  Fully embedded on ESP32-CAM using C++
- publish access via mqtt message by server(PC or cloud server) and Subscription by esp32-cam  
-  Uses Local Server for storing face data (or SPIFFS)
-  Access granted only to known faces
-  Flash LED used during face capture
-  Uses OpenCV and Face recognition libraries 

 

# Hardware Requirements

 Component        Description                     

 ESP32-CAM          AI Thinker Module               
 Relay Module       To control door lock or switch  
 Power Supply       5V 2A recommended               
 Web Dashboard Button (opt)  For enrollment and control door  
 MQTT Broker (HimeQ)     
 Breadboard & Wires For quick prototyping           



# Software & Tools

 Python(3.10)
 C++ programming with Arduino Ide
 MQTT Broker (HivemQ)
 Local Storage or remote for image and metadata storage

