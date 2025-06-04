/*
  Inkplate TRMNL Client
  Connects to TRMNL server, fetches display configuration, and shows image on e-ink display
*/

#include "HTTPClient.h"
#include "Inkplate.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include "config.h"

#define uS_TO_S_FACTOR 1000000

Inkplate display(INKPLATE_3BIT);

int current_refresh_rate = default_refresh_rate;
String debug_messages = "";
int debug_line = 0;

void setup() {
    // Set up sleep timer with default refresh rate (will be updated after API call)
    esp_sleep_enable_timer_wakeup(default_refresh_rate * uS_TO_S_FACTOR);
    
    Serial.begin(115200);
    
    display.setRotation(0);  // landscape
    display.begin();
    display.setTextColor(0, 7);
    display.setTextSize(3);
    
    showStatus("Connecting to WiFi...");
    
    connectToWiFi();
    
    showStatus("WiFi Connected!");
    delay(1000);
    
    fetchAndDisplayImage();
    
    // Update sleep timer with actual refresh rate from server
    esp_sleep_enable_timer_wakeup(current_refresh_rate * uS_TO_S_FACTOR);
    
    //addDebugMessage("Going to sleep for " + String(current_refresh_rate) + " seconds");
    //delay(2000);  // Give time to see the message
    
    esp_deep_sleep_start();
}

void loop() {
    // Nothing - device sleeps after setup() completes
}

void connectToWiFi() {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        Serial.print(".");
        delay(1000);
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        addDebugMessage("Failed to connect to WiFi, restarting...");
        ESP.restart();
    }
    
    addDebugMessage("WiFi connected!");
    addDebugMessage("IP: " + WiFi.localIP().toString());
}

void showStatus(const char* message) {
    display.clearDisplay();
    display.setCursor(20, 20);
    display.print(message);
    display.partialUpdate();
    display.display();
    //Serial.println(message);
}

void addDebugMessage(String message) {
    //Serial.println(message);
    debug_messages += message + "\n";
    debug_line++;
    
    // Keep only last 25 lines to fit on screen
    if (debug_line > 25) {
        int firstNewline = debug_messages.indexOf('\n');
        if (firstNewline > 0) {
            debug_messages = debug_messages.substring(firstNewline + 1);
            debug_line--;
        }
    }
    
    displayDebugMessages();
}

void displayDebugMessages() {
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(5, 5);
    display.print(debug_messages);
    display.partialUpdate();
    display.display();
}

void fetchAndDisplayImage() {
    HTTPClient http;
    http.begin(String(trmnl_server) + "/api/display");
    
    // Add required headers
    http.addHeader("ID", device_id);
    http.addHeader("Access-Token", access_token);
    http.addHeader("Content-Type", "application/json");
    
    // Add device info headers
    http.addHeader("BATTERY_VOLTAGE", String(getBatteryVoltage()));
    http.addHeader("FW_VERSION", fw_version);
    http.addHeader("HOST", WiFi.localIP().toString());
    http.addHeader("REFRESH_RATE", String(current_refresh_rate));
    http.addHeader("RSSI", String(WiFi.RSSI()));
    http.addHeader("USER_AGENT", user_agent);
    http.addHeader("WIDTH", String(device_width));
    http.addHeader("HEIGHT", String(device_height));
    
    addDebugMessage("Making API request to TRMNL server...");
    addDebugMessage("URL: " + String(trmnl_server) + "/api/display");
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        addDebugMessage("API Response received:");
        addDebugMessage(response.substring(0, 100) + "...");
        
        String imageUrl = parseDisplayResponse(response);
        
        if (imageUrl.length() > 0) {
            displayImageFromUrl(imageUrl);
        } else {
            addDebugMessage("No image URL found");
        }
    } else {
        addDebugMessage("HTTP Error: " + String(httpResponseCode));
        addDebugMessage("The display API request failed");
    }
    
    http.end();
}

String parseDisplayResponse(String jsonResponse) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonResponse);
    
    String imageUrl = doc["image_url"].as<String>();
    int newRefreshRate = doc["refresh_rate"].as<int>();
    
    if (newRefreshRate > 0) {
        current_refresh_rate = newRefreshRate;
        addDebugMessage("Updated refresh rate to: " + String(current_refresh_rate));
    }
    
    return imageUrl;
}

void displayImageFromUrl(String imageUrl) {
    addDebugMessage("Downloading image from:");
    addDebugMessage(imageUrl);
    
    display.clearDisplay();
    
    bool imageLoaded = false;
    
    if (imageUrl.endsWith(".bmp")) {
        imageLoaded = display.drawImage(imageUrl, Image::BMP, 0, 0, false, false);
    } else if (imageUrl.endsWith(".jpg") || imageUrl.endsWith(".jpeg")) {
        imageLoaded = display.drawImage(imageUrl, Image::JPG, 0, 0, false, false);
    } else if (imageUrl.endsWith(".png")) {
        imageLoaded = display.drawImage(imageUrl, Image::PNG, 0, 0, false, false);
    }
    
    if (!imageLoaded) {
        addDebugMessage("Failed to load image");
        return;
    }
    
    display.display();
    //addDebugMessage("Image displayed successfully");
}

float getBatteryVoltage() {
    // For Inkplate devices, battery voltage can be read from analog pin
    // This is a simplified implementation - adjust based on your hardware
    int rawValue = analogRead(A0);
    float voltage = (rawValue / 4095.0) * 3.3 * 2; // Assuming voltage divider
    return voltage;
}
