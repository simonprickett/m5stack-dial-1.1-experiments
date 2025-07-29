# CheerDial Experiment

This is a small experiment with the M5Dial device to replicate some aspects of the [CheerLights](https://cheerlights.com/learn/) project.

The dial connects to a wireless network on start up, and presents a series of color choices that the user can flip through by turning the encoder.

Pressing the button on the device sends a numeric representation of the user's color choice to Grafana Cloud using the Prometheus remote write protocol.

This data could then be visualized on a Grafana dashboard, and/or perhaps used to trigger alerts.

## Pre-requisites

You'll need a free tier [Grafana Cloud account](https://grafana.com/auth/sign-up/create-user), an M5Stack Dial and the SSID and password for a wireless network connection that's connected to the internet.

## Configuration

Before attempting to run the code, you'll need to change some values in the `config.h` file:

```c
// This file contains secrets.  Don't commit your secrets to source control!

// WiFi details.
#define WIFI_SSID "Your network SSID"
#define WIFI_PASSWORD "Your network password"

// Device identifier and configuration.
# define DEVICE_ID "simon" // Keep this to 10 chars or fewer.
# define ENCODER_SENSITIVITY 5 // Higher = more turning required.

// Grafana Cloud details.
#define GC_URL "The hostname of your Grafana Cloud instance Prometheus endpoing"
#define GC_PATH "/api/prom/push"
#define GC_PORT 443
#define GC_USER "Your Grafana Cloud user number"
#define GC_PASS "Your Grafana Cloud token" 
```

You can find the URL, username, and password for your metrics endpoint by clicking on "Details" in the Prometheus card of the Grafana Cloud Portal.

**Be sure to save your changes before continuing.**

## Running the Code

To run the code, compile it with the Arduino IDE with appropriate board support for the M5Dial installed then upload it to the M5Dial.

Use the Arduino IDE's serial monitor to check for any errors from the code.

If all goes well, the device should connect to your network, indicate that it has done this successfully, then display the "Red" color.

## Using the Code

Turn the encoder part of the dial in either direction to scroll back and forth through the colors.  Press the button (the bottom orange segment of the encoder dial) to choose a color.

Data is sent to your Grafana Cloud stack and the device displays a brief "Thanks" message before returning to your selected color.

## Metrics

Every time you press the button, the device sends a number representing to the color selected and a timestamp to Grafana Cloud.

The metric is called `grafanalights` and the color codes are as follows:

|Value|Color     |
|-----|----------|
|  0  | Red      |
|  1  | Green    |
|  2  | Blue     |
|  3  | Cyan     |
|  4  | White    |
|  5  | Old Lace |
|  6  | Purple   |
|  7  | Magenta  |
|  8  | Yellow   |
|  9  | Orange   |
| 10  | Pink     |

If you want to use color codes to visualize the colors, the official CheerLights codes can be found [here](https://cheerlights.com/learn/) in the "Supported Colors" section.