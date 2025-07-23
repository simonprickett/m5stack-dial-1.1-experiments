#include <WiFi.h>
#include "M5Dial.h"

const short ENCODER_SENSITIVITY = 5; // Lower = less turning required.

struct CHEERLIGHT {
    int r;
    int g;
    int b;
    char *colorName;
};

const struct CHEERLIGHT cheerlights[] = {
    { 255, 0, 0, "red" },         // OK
    { 0, 255, 0, "green" },       // OK
    { 0, 0, 255, "blue" },        // OK
    { 0, 255, 255, "cyan" },      // OK
    { 255, 255, 255, "white" },   // OK
    { 253, 245, 240, "oldlace" }, // Too similar to white...
    { 128, 0, 128, "purple" },    // OK
    { 255, 0, 255, "magenta" },   // Too similar to purple...
    { 255, 255, 0, "yellow" },    // OK
    { 255, 165, 0, "orange" },    // OK
    { 255, 192, 203, "pink" }     // Too light not pink enough!
};

const short NUM_COLORS = sizeof(cheerlights) / sizeof(cheerlights[0]);

M5GFX display;
M5Canvas canvas(&display);

short idx = -1;
long oldPosition = -999;

void setColorAndText(int r, int g, int b, char *text) {
    display.clear();
    display.startWrite();
    canvas.deleteSprite();
    canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());
    canvas.fillSprite(M5Dial.Display.color888(r, g, b));
    canvas.setTextColor(M5Dial.Display.color888(0, 0, 0));
    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::Orbitron_Light_32);
    canvas.setTextSize(1.5);
    canvas.drawString(text, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    canvas.pushSprite(0, 0);
    display.endWrite();  
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    // true, false = Enable encoder, disable RFID.
    M5Dial.begin(cfg, true, false);

    display.begin();
    // Some other stuff...

    // Connect to a wifi network...
    WiFi.mode(WIFI_STA);
    // TODO move these to secrets file!
    WiFi.begin("SSID", "Password");

    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println(WiFi.localIP());
}

void loop() {
    M5Dial.update();
    
    // This will initially return 0.
    long newPosition = M5Dial.Encoder.read();
    if (abs(newPosition - oldPosition) >= ENCODER_SENSITIVITY) {
        // Encoder position has changed! 
        idx = (newPosition > oldPosition ? idx + 1 : idx - 1);

        // Update the array index, deal with roll arounds.
        if (idx < 0) {
            idx = NUM_COLORS - 1;
        } else if (idx == NUM_COLORS) {
            idx = 0;
        }
        
        M5Dial.Speaker.tone(8000, 20);        
        setColorAndText(cheerlights[idx].r, cheerlights[idx].g, cheerlights[idx].b, cheerlights[idx].colorName);

        // Sort out the save location here... in cheerdial folder.
            
        // Reset the encoder value.
        M5Dial.Encoder.write(0);
        oldPosition = 0;
    }

    if (M5Dial.BtnA.wasClicked()) {
        // TODO something with the button being pressed...
        char *selectedColor = cheerlights[idx].colorName;
        Serial.println(selectedColor);

        // Example code for sending to Grafana Cloud:
        // https://github.com/grafana/prometheus-arduino/blob/main/examples/prom_02_grafana_cloud/prom_02_grafana_cloud.ino
    }
}