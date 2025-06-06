/*
  Grabs front page image from Linode server
*/

#include "HTTPClient.h"          //Include library for HTTPClient
#include "Inkplate.h"            //Include Inkplate library to the sketch
#include "WiFi.h"                //Include library for WiFi
#include "config.h"

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 300 /* Time ESP32 will go to sleep (in seconds) */

// Create Inplate object and set library to 3 bit
Inkplate display(INKPLATE_3BIT);

// in config.h
//const char ssid[] = "";
//const char *password = "";

void setup()
{
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    display.setRotation(3);  // portrait
    display.begin();        // Init Inkplate library (you should call this function ONLY ONCE)
    display.setTextColor(0, 7); 
    display.setTextSize(4);
    display.setCursor(20, 20);
    display.print("Connecting to WiFi...");
    display.partialUpdate();
    display.display();

    // Connect to the WiFi network.
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(F("."));
        delay(1000);
        ++cnt;

      if (cnt == 20)
      {
        Serial.println("Can't connect to WIFI, restarting");
        delay(100);
        ESP.restart();
      }
    }

    display.setCursor(20, 80);
    display.println("WiFi Connected!");
    display.partialUpdate();
    display.display();
    display.clearDisplay(); // Clear frame buffer of display

    //if (!display.drawImage("http://192.168.86.65:5000/imagejpg", Image::JPG, 0, 0, false, false)) //PNG
    if (!display.drawImage(url, Image::JPG, 0, 0, false, false)) //PNG
    {
        display.println("Image open error");
        display.display();
    }
    display.display();
    delay(5000);
    (void)esp_deep_sleep_start();

}

void loop()
{
    // Nothing...
}
