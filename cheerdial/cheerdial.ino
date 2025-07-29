#include "M5Dial.h"
#include "config.h"
#include "certificates.h"
#include <PromLokiTransport.h>
#include <PrometheusArduino.h>

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

PromLokiTransport transport;
PromClient client(transport);

short idx = -1;
long oldPosition = -999;

// Create a write request for 1 series and a timeseries for 1 sample at a time.
WriteRequest req(1);
TimeSeries ts(1, "grafanalights", "");  // Labels?

void setColorAndText(int r, int g, int b, char *text, float textSize) {
    display.clear();
    display.startWrite();
    canvas.deleteSprite();
    canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());
    canvas.fillSprite(M5Dial.Display.color888(r, g, b));
    canvas.setTextColor(M5Dial.Display.color888(0, 0, 0));
    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::Orbitron_Light_32);
    canvas.setTextSize(textSize);
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

    // Display the device ID for a moment.
    setColorAndText(255, 255, 255, DEVICE_ID, 1);
    delay(4000);

    // Connect to a wifi network...
    setColorAndText(255, 165, 0, "Connecting...", 1);

    transport.setUseTls(true);
    transport.setCerts(grafanaCert, strlen(grafanaCert));
    transport.setWifiSsid(WIFI_SSID);
    transport.setWifiPass(WIFI_PASSWORD);
    transport.setDebug(Serial);

    if (!transport.begin()) {
        Serial.println("WiFi connection failed!");
        Serial.println(transport.errmsg);
        setColorAndText(255, 0, 0, "WiFi fail!", 1);
        while (1) {
            delay(1000);
        }
    }

    // Configure the client.
    client.setUrl(GC_URL);
    client.setPath((char *)GC_PATH);
    client.setPort(GC_PORT);
    client.setUser(GC_USER);
    client.setPass(GC_PASS);
    client.setDebug(Serial);

    if (! client.begin()) {
        setColorAndText(255, 0, 0, "Client error!", 1);
        while (1) {
            delay(1000);
        }    
    }

    //char buf[100];
    //sprintf(buf, "{unit=\"%s\"}", DEVICE_ID);
    //Serial.println(buf);

    req.addTimeSeries(ts);

    setColorAndText(0, 255, 0, "Connected!", 1);
    Serial.println(WiFi.localIP());
    M5Dial.Speaker.tone(8000, 20);        
    delay(3000);
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

        // Announce the new setting and update the screen.
        M5Dial.Speaker.tone(8000, 20);        
        setColorAndText(cheerlights[idx].r, cheerlights[idx].g, cheerlights[idx].b, cheerlights[idx].colorName, 1.5);
            
        // Reset the encoder value.
        M5Dial.Encoder.write(0);
        oldPosition = 0;
    }

    if (M5Dial.BtnA.wasClicked()) {
        ts.addSample(transport.getTimeMillis(), idx);

        // Send the data...
        PromClient::SendResult res = client.send(req);
        if (res == PromClient::SendResult::SUCCESS) {
            setColorAndText(0, 255, 0, "Thanks!", 1.5);
            M5Dial.Speaker.tone(8000, 20);        
        } else {
            // Something went wrong :(
            Serial.println(client.errmsg);
            setColorAndText(255, 0, 0, "Error!", 1.5);
        }

        ts.resetSamples();
        delay(2000);

        // Put the display back to the selected color.
        setColorAndText(cheerlights[idx].r, cheerlights[idx].g, cheerlights[idx].b, cheerlights[idx].colorName, 1.5);
    }
}