# M5Stack Dial 1.1 Experiments

## Shopping List

* Arduino IDE ([download](https://www.arduino.cc/en/software/)).  Use version 2.x, don't use the cloud editor.
* M5Stack Dial 1.1 ([buy here](https://shop.m5stack.com/products/m5stack-dial-v1-1) (USA/global) or [here](https://thepihut.com/products/m5stack-dial-v1-1) (UK)).
* USB C-C or C-A cable, whatever you need to connect the dial to the machine you're developing on.

## Environment Setup

These instructions have been tested on macOS 15.5.

1. Install the [Arduino IDE](https://www.arduino.cc/en/software/).
1. Follow the M5Stack tutorial [here](https://docs.m5stack.com/en/arduino/m5dial/program) to get a demo C program compiling and running on the device in your local environment.  Note that this guide refers to other guides for setting up boards and installing libraries in your environment - be sure to follow those too!  This guide also talks about selecting "Preferences" from the "File" menu in ArduinoIDE.  On macOS, select "ArduinoIDE" then "Settings" instead.
1. Continue onwards once you have the M5Stack test "display" program running on your device.

## Code Experiments

This repository contains the following code experiments that use Grafana APIs with the M5Stack Dial device.

* [CheerDial](./cheerdial/) - a CheerLights like project.
* TODO other experiments...

## Libraries Used

* [prometheus-arduino](https://github.com/grafana/prometheus-arduino) for Prometheus remote write.  Note that this library has its own dependencies listed in its README file.

## Resources

* [M5Stack Dial 1.1 documentation](https://docs.m5stack.com/en/core/M5Dial%20V1.1).
* [Arduino IDE setup / tutorial](https://docs.m5stack.com/en/arduino/m5dial/program).
* [M5Stack Dial library on GitHub](https://github.com/m5stack/M5Dial).
* [Source code for the factory installed M5Stack Dial demo app](https://github.com/m5stack/M5Dial-UserDemo).
* [Example project that sends data to Grafana from a M5Stack device](https://www.hackster.io/ecasti/m5stack-as-sensor-for-prometheus-5bb162).
* [Interesting example project that exposes a web server on the M5Stack Dial](https://github.com/kevinlieb/AlmostSimplestHelloWorld/tree/webandwifiandspiffsandwebsocket).
* A potential [3D printable stand](https://www.thingiverse.com/thing:6277342) for the M5Dial device.