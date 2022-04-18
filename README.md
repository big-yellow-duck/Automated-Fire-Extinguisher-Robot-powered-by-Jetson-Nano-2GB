# Automated-Fire-Extinguisher-Robot-powered-by-Jetson-Nano-2GB

This project uses a Deep learning on the Jetson Nano 2GB paired with an ESP32 used for motor controls to extinguish small fires in a small area.
Familirity of the [jetson-inference](https://github.com/dusty-nv/jetson-inference) repo is a requirement to run this project. 

[Video Demo of this project](https://www.youtube.com/watch?v=Xg-PzxqsTTw&t=4s)

<img src= "https://user-images.githubusercontent.com/83417790/163781241-674ca08f-cb6b-4246-828a-d68f7cf4171e.jpg" width="800px" height="800px">


The link to the assembly of this project can found [here](https://drive.google.com/drive/folders/1Qaj5e5Lfy3mRaZVl2hqvJ6ceHgBbuAHm?usp=sharing)

The electronic components in the project are not included in the assembly. They can be placed anywhere since there is much free space behind
<img src="https://user-images.githubusercontent.com/83417790/163812943-fc0ba3ef-ac4d-4765-873b-42c64cd0cd10.png" width="800px" height="850px">
## Wiring the Project

The electronic componenents used here are:
* Jetson Nano 2GB
* ESP32 
* 2x MG996R Servo Motor
* 2x 12V 200Rpm DC geared motor
* L298N DC motor driver
* Raspberry Pi Noir V2.1 camera
* XL4015 DC-DC Buck Converter
* Any button switch
* 3000mAH 11.1v Lithium-Polymer Battery

## Wiring diagram
<img src="https://user-images.githubusercontent.com/83417790/163813286-be751741-ab2a-4865-b7bb-ef16bae6cbd6.png">


The Pins 8 and 10 on the Jetson are wired to the pins 16 and 17 on the ESP32 for serial commnication.

## Programming the ESP32

The programming of the ESP32 is done is a way that makes the Jetson the "master" while the ESP32 is a "slave" The ESP32 will listen to the to the instructions provided its "master" and act accordingly. The motors used for movement are normal DC motors. The code needs to be tuned to make sure that both left and right motors are rotating at the right speed to make sure the robot can move predictably to the fire.

## Retraining SSD mobilenet-V2
If you want to retrain the model yourself, the dataset is provided. adding more pictures to make the dataset larger is a good idea to make your model more robust! Feel free to do!

The SSD mobilenet-V2 was retrained using transfer learning with pytorch that lessens the time and amount of data needed for training. A detailed guide to [Retraining SSD mobilenet-V2](https://github.com/dusty-nv/jetson-inference/blob/master/docs/pytorch-ssd.md) is provided by the jetson-inference Github repo.

The model provided here is traned for 100 epochs on the Jetson Nano 2GB. The time take to train the model was roughly 30 hours.

## Notes

There is a bug I was not able to fix where the pyserial library will cause errors and crash the program when it is run for the first time after booting up the Jetson Nano 2GB. 

**Be aware of the battery voltage** while operating the robot. Lipo Batteries are not meant to be drained below their nominal voltage. They will die prematurely ( I've learnt this the hard way). Using the **XL4015 DC-DC Buck Converter** with an LED display to display the voltage of the battery is a good practice to make sure that the voltage does not fall to low. 
