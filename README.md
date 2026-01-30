# PROJECT2
Team Members: Iyene Essien & Eric Greenberg

System behavior (what does it do?): 
his project implements an enhanced vehicle safety and lighting control system using an ESP32-S3 microcontroller. 
The system check the following:
- if the driver and passenger seats are occupied 
- if the seatbelts are on
It also has an ignition button that needs to be pressed before allowing the engine to start and provides LED and buzzer feedback if conditions are unsafe. Once the system is running, the system controls headlights using OFF, ON, or AUTO modes. For the AUTO mode the system uses a light sensor with delays. Our system also has a high-beam switch that allows manual high-beam control when the headlights are on.

Starting repository: 
This project builds on the structure and concepts developed in Project 1 of Iyene's code. The original repository included basic GPIO input and output handling using ESP-IDF. Project 2 expands on this by adding ADC-based sensing, timing logic, and multi-condition decision making.

Summary of Testing Results:


Summary of testing results. Create a table by summarizing the list of project behaviors for the two subsystems to organize the results. Fill in “Test Process” for each behavior, and then fill in the results
