#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// =====================================================
// USER SETTINGS
// =====================================================

const char* ssid       = "JST-Home6";
const char* password   = "2233445566";

const char* apiKey     = "AIzaSyBvUTY317ej3Z6YazkNGJSRd36XZ7H5uTY";
const char* channelId  = "UCxtr-CW3Dph6QytblF4SWTQ";

// =====================================================
// DISPLAY PINS
// =====================================================

#define TFT_CS      15
#define TFT_DC       2
#define TFT_RST     -1

#define TFT_MOSI    13
#define TFT_SCLK    14

#define TFT_BL      21

// =====================================================
// COLORS FOR YOUR DISPLAY
// =====================================================

#define UI_BG       ST77XX_WHITE
#define UI_TEXT     ST77XX_BLACK
#define UI_RED      ST77XX_CYAN
#define UI_GREEN    ST77XX_MAGENTA

// =====================================================

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000;

// =====================================================
// FORMAT NUMBER
// =====================================================

String formatNumber(long number)
{
    String s = String(number);

    for (int i = s.length() - 3; i > 0; i -= 3)
    {
        s = s.substring(0, i) + "." + s.substring(i);
    }

    return s;
}

// =====================================================
// WIFI
// =====================================================

void connectWiFi()
{
    tft.fillScreen(UI_BG);

    tft.setTextColor(UI_TEXT);
    tft.setTextSize(2);

    tft.setCursor(60, 110);
    tft.print("Connecting...");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    tft.fillScreen(UI_BG);

    tft.setCursor(70, 110);
    tft.print("Connected");

    delay(1000);
}

// =====================================================
// DRAW SCREEN
// =====================================================

void drawSubscribers(long subscribers)
{
    long milestones[] =
    {
        1000,
        5000,
        10000,
        25000,
        50000,
        100000,
        250000,
        500000,
        1000000,
        2500000,
        5000000,
        10000000
    };

    long target = milestones[0];

    for (int i = 0; i < 12; i++)
    {
        if (subscribers < milestones[i])
        {
            target = milestones[i];
            break;
        }
    }

    float progress =
        (float)subscribers /
        (float)target;

    if (progress > 1.0)
        progress = 1.0;

    tft.fillScreen(UI_BG);

    // =================================================
    // TITLE
    // =================================================

    tft.setTextColor(UI_RED);
    tft.setTextSize(2);

    String title = "SUBSCRIBERS";

    int16_t x1, y1;
    uint16_t w, h;

    tft.getTextBounds(
        title,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h
    );

    tft.setCursor((320 - w) / 2, 15);
    tft.print(title);

    // =================================================
    // BIG NUMBER
    // =================================================

    String subText =
        formatNumber(subscribers);

    tft.setTextColor(UI_TEXT);
    tft.setTextSize(7);

    tft.getTextBounds(
        subText,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h
    );

    tft.setCursor((320 - w) / 2, 55);
    tft.print(subText);

    // =================================================
    // PROGRESS BAR
    // =================================================

    int barX = 20;
    int barY = 150;
    int barW = 280;
    int barH = 22;

    tft.drawRect(
        barX,
        barY,
        barW,
        barH,
        UI_TEXT
    );

    tft.fillRect(
        barX + 1,
        barY + 1,
        barW - 2,
        barH - 2,
        UI_BG
    );

    tft.fillRect(
        barX + 1,
        barY + 1,
        (int)((barW - 2) * progress),
        barH - 2,
        UI_GREEN
    );

    // =================================================
    // TARGET TEXT
    // =================================================

    String goalText =
        formatNumber(subscribers) +
        " / " +
        formatNumber(target);

    tft.setTextColor(UI_TEXT);
    tft.setTextSize(2);

    tft.getTextBounds(
        goalText,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h
    );

    tft.setCursor(
        (320 - w) / 2,
        190
    );

    tft.print(goalText);
}

// =====================================================
// YOUTUBE API
// =====================================================

void getSubscribers()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;

    String url =
        "https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" +
        String(channelId) +
        "&key=" +
        String(apiKey);

    if (https.begin(client, url))
    {
        int httpCode = https.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload =
                https.getString();

            JsonDocument doc;

            DeserializationError error =
                deserializeJson(doc, payload);

            if (!error)
            {
                long subscribers =
                    atol(
                        doc["items"][0]["statistics"]["subscriberCount"]
                    );

                drawSubscribers(subscribers);

                Serial.print("Subscribers: ");
                Serial.println(subscribers);
            }
        }

        https.end();
    }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    SPI.begin(
        TFT_SCLK,
        -1,
        TFT_MOSI,
        TFT_CS
    );

    tft.init(240, 320);
    tft.setRotation(1);

    connectWiFi();

    configTzTime(
        "CET-1CEST,M3.5.0,M10.5.0/3",
        "pool.ntp.org"
    );

    getSubscribers();

    lastUpdate = millis();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    if (millis() - lastUpdate > updateInterval)
    {
        getSubscribers();
        lastUpdate = millis();
    }

    delay(100);
}