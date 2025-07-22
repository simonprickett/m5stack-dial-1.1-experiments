#include "M5Dial.h"

short NUM_COLORS = 11;
long oldPosition = -999;

struct CHEERLIGHT {
    int r;
    int g;
    int b;
    char *colorName;
    // TODO add text color stuff.
};

struct CHEERLIGHT cheerlights[] = {
    { 255, 0, 0, "red" },       // OK
    { 0, 255, 0, "green" },     // OK
    { 0, 0, 255, "blue" },      // OK
    { 0, 255, 255, "cyan" },    // OK
    { 255, 255, 255, "white" }, // OK
    { 253, 245, 240, "oldlace" }, // Too similar to white...
    { 128, 0, 128, "purple" },    // OK
    { 255, 0, 255, "magenta" },   // Too similar to purple...
    { 255, 255, 0, "yellow" },    // OK
    { 255, 165, 0, "orange" },    // OK
    { 255, 192, 203, "pink" }     // Too light not pink enough!
};

short idx = 0;

M5GFX display;
M5Canvas canvas(&display);

void setup() {
    Serial.begin(9600);
    auto cfg = M5.config();
    // true, false = Enable encoder, disable RFID.
    M5Dial.begin(cfg, true, false);

    display.begin();
    // Some other stuff...
}

void loop() {
    M5Dial.update();
    
    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition) {
        // Position has changed! 
        // TODO consider only updating after a couple of position clicks?
        M5Dial.Speaker.tone(8000, 20);
        display.clear();
        display.startWrite();
        canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());
        canvas.fillSprite(M5Dial.Display.color888(cheerlights[idx].r, cheerlights[idx].g, cheerlights[idx].b));
        canvas.setTextColor(M5Dial.Display.color888(0, 0, 0));
        canvas.setTextDatum(middle_center);
        canvas.setFont(&fonts::Orbitron_Light_32);
        canvas.setTextSize(1);
        canvas.drawString(cheerlights[idx].colorName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        canvas.pushSprite(0, 0);
        display.endWrite();

        // Some of the things below are constants.
        // Should I clear the screen between things.
        // What are the bounds of newPosition?
        // Sort out the save location here... in cheerdial folder.
    
        Serial.println(idx);

        // TODO this will need fixing so it starts on idx 0.
        // TODO also needs fixing for both directions.
        idx++;
        if (idx < 0) {
            idx = NUM_COLORS;
        }

        if (idx == NUM_COLORS) {
            idx = 0;
        }
        
        oldPosition = newPosition;
    }

    // TODO something with the button being pressed...
}
