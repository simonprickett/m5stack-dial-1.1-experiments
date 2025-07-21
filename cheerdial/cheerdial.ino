#include "M5Dial.h"

short NUM_COLORS = 11;
long oldPosition = -999;
// TODO set up the colors...

uint32_t colors[11];
short idx = 0;

void setup() {
    Serial.begin(9600);
    auto cfg = M5.config();
    // true, false = Enable encoder, disable RFID.
    M5Dial.begin(cfg, true, false);
    // TODO some other setup stuff.
    M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
    M5Dial.Display.setTextSize(2);

    // Some other stuff...
    // TODO declare literal array more obviously?
    colors[0] = M5Dial.Display.color888(255, 0, 0); // Red OK
    colors[1] = M5Dial.Display.color888(0, 255, 0); // Green OK
    colors[2] = M5Dial.Display.color888(0, 0, 255); // Blue OK
    colors[3] = M5Dial.Display.color888(0, 255, 255); // Cyan OK
    colors[4] = M5Dial.Display.color888(255, 255, 255); // White OK
    colors[5] = M5Dial.Display.color888(253, 245, 240); // Oldlace needs better distinguish from white
    colors[6] = M5Dial.Display.color888(128, 0, 128); // Purple OK
    colors[7] = M5Dial.Display.color888(255, 0, 255); // Magenta not very distinguished from purple?
    colors[8] = M5Dial.Display.color888(255, 255, 0); // Yellow OK
    colors[9] = M5Dial.Display.color888(255, 165, 0); // Orange OK
    colors[10] = M5Dial.Display.color888(255, 192, 203); // Pink very light needs more pinkness!
}

void loop() {
    M5Dial.update();
    
    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition) {
        // Position has changed!
        M5Dial.Speaker.tone(8000, 20);
        M5Dial.Display.clear();

        //Serial.println(newPosition);

        // Some of the things below are constants.
        // How do colors work?
        // Should I clear the screen between things.
        // What are the bouncs of newPosition?
        // Sort out the save location here... in cheerdial folder.
    
        int x      = M5Dial.Display.width() / 2;
        int y      = M5Dial.Display.height() / 2;
        int r      = x;
        //uint16_t c = M5Dial.Display.color888(255, 128, 0);
        M5Dial.Display.clear();
        //Serial.println(colors[idx]);
        Serial.println(idx);
        M5Dial.Display.fillCircle(x, y, r, colors[idx]);

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
