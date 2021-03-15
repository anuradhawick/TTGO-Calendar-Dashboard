/**
    TTGO-Calendar-Dashboard
    @file utils.h
    @author Anuradha Wickramarachchi
*/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <vector>
#include <WiFiClientSecure.h>
#include <pgmspace.h>
#include <ArduinoJson.h>
#include "time.h"
#include "esp_wpa2.h"

#include "utils.h"
#include "ani.h"

TFT_eSPI tft = TFT_eSPI(135, 240);
WiFiClientSecure client;

String script_url = "https://script.google.com/macros/s/AKfycbzPw3eXFKjdor6kU_9jspluc23ZtTcjYxdinqfjZtLOyZFIEHf-6pcjdO3VQBmohHo/exec";
String script_host = "script.google.com";
String ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;
static const char *TAG = "MAIN";
const byte interruptPin1 = 35;
const byte interruptPin2 = 0;

// context control
volatile int global_status = 0;
StaticJsonDocument<512> doc;
unsigned long last_update = 0;
bool updated = false;
volatile bool privacy = false;

void connectToWifi()
{
#ifndef EAP_MODE
	// Personal Mode
	WiFi.begin(WIFI_SSID, WIFI_PASSWD);
	ESP_LOGD(TAG, "Connecting to wifi: %s", WIFI_SSID);
#else
	// WPA2 Enterprise Mode
	WiFi.disconnect(true);
	WiFi.mode(WIFI_STA);
	ESP_LOGD(TAG, "Connecting to wifi: %s", EAP_SSID);
	esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); //provide identity
	esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); //provide username
	esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)); //provide password
	esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
	esp_wifi_sta_wpa2_ent_enable(&config);
	WiFi.begin(EAP_SSID);
#endif

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("WiFi connected ");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

// UI thread
void ui_task(void *pvParameters)
{
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	int current_x = 0, current_y = 0;
	int aniFrame = 0;
	String tmp;
	struct tm timeinfo;
	bool time_success;
	char time_buff[3];
	char day_buff[10];
	char year_buff[5];

	for (;;)
	{
		if (global_status == 0 || privacy)
		{
			tft.pushImage(0, 0, ani_width, ani_height, ani_imgs[aniFrame]);
			aniFrame++;
			if (aniFrame == ani_frames)
				aniFrame = 0;
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
		else if (global_status == 1)
		{
			tft.setTextSize(2);
			tft.setTextDatum(TL_DATUM);
			tft.fillScreen(TFT_BLACK);
			for (JsonObject elem : doc.as<JsonArray>())
			{
				// if the global state has changed, immediately break the loop
				if (global_status != 1 || privacy)
				{
					tft.fillScreen(TFT_BLACK);
					break;
				}
				if ((bool)elem["allDay"])
				{
					tft.setTextColor(TFT_GREEN, TFT_BLACK);
					tmp = String("* ") + String((const char *)elem["title"]);
					current_y = printMultiline(tft, tmp, current_x, current_y);
					current_y += 10;
					tft.drawLine(current_x, current_y + 1, 240, current_y + 1, TFT_WHITE);
					tft.drawLine(current_x, current_y + 2, 240, current_y + 2, TFT_WHITE);
					tft.drawLine(current_x, current_y + 3, 240, current_y + 3, TFT_WHITE);
					tft.drawLine(current_x, current_y + 4, 240, current_y + 4, TFT_WHITE);
					current_y += 20;

					tft.setTextColor(TFT_YELLOW, TFT_BLACK);
					tmp = String("- ") + String((const char *)elem["start"]);
					current_y = printMultiline(tft, tmp, current_x, current_y);
					// clear strings
					tmp = "";

					vTaskDelay(5000 / portTICK_PERIOD_MS);
					current_y = 0;
					tft.fillScreen(TFT_BLACK);
				}
				else
				{
					tft.setTextColor(TFT_RED, TFT_BLACK);
					tmp = String("* ") + String((const char *)elem["title"]);
					current_y = printMultiline(tft, tmp, current_x, current_y);
					current_y += 10;
					tft.drawLine(current_x, current_y + 1, 240, current_y + 1, TFT_WHITE);
					tft.drawLine(current_x, current_y + 2, 240, current_y + 2, TFT_WHITE);
					tft.drawLine(current_x, current_y + 3, 240, current_y + 3, TFT_WHITE);
					tft.drawLine(current_x, current_y + 4, 240, current_y + 4, TFT_WHITE);
					current_y += 20;

					tft.setTextColor(TFT_YELLOW, TFT_BLACK);
					tmp = String("- ") + String((const char *)elem["start"]);
					current_y = printMultiline(tft, tmp, current_x, current_y);

					tft.setTextColor(TFT_YELLOW, TFT_BLACK);
					tmp = String("- ") + String((const char *)elem["end"]);
					current_y = printMultiline(tft, tmp, current_x, current_y);
					// clear strings
					tmp = "";
					vTaskDelay(5000 / portTICK_PERIOD_MS);
					current_y = 0;
					tft.fillScreen(TFT_BLACK);
				}
			}
			global_status = 2;
		}
		else if (global_status == 2)
		{
			tft.pushImage(0, 0, ani_width, ani_height, ani_imgs[0]);
			// show time for 5 seconds
			for (size_t i = 0; i < 10; i++)
			{
				time_success = getLocalTime(&timeinfo);

				if (time_success)
				{
					tft.setTextSize(3);
					tft.setTextDatum(MC_DATUM);

					// printing time
					tft.setTextColor(TFT_GOLD, TFT_BLACK);
					strftime(time_buff, 3, "%H", &timeinfo);
					tmp = String(time_buff) + ":";
					strftime(time_buff, 3, "%M", &timeinfo);
					tmp += String(time_buff) + ":";
					strftime(time_buff, 3, "%S", &timeinfo);
					tmp += String(time_buff);
					tft.drawString(tmp, 120, 67 - 16);

					// Gap line
					tft.drawLine(30, 67 - 1, 210, 67 - 1, TFT_WHITE);
					tft.drawLine(30, 67, 210, 67, TFT_WHITE);
					tft.drawLine(30, 67 + 1, 210, 67 + 1, TFT_WHITE);
					tft.drawLine(30, 67 + 2, 210, 67 + 2, TFT_WHITE);

					// printing date
					tft.setTextSize(2);
					tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
					strftime(time_buff, 3, "%d", &timeinfo);
					tmp = String(time_buff) + " ";
					strftime(day_buff, 10, "%A", &timeinfo);
					strftime(year_buff, 5, "%Y", &timeinfo);
					tmp += String(day_buff) + " " + String(year_buff);

					tft.drawString(tmp, 120, 67 + 20);
					// clear strings
					tmp = "";

					vTaskDelay(1001 / portTICK_PERIOD_MS);
				}
			}
			global_status = 1;
		}
	}
}

void isr1()
{
	ESP_LOGD(TAG, "Enable privacy mode ISR");
	privacy = true;
}

void isr2()
{
	ESP_LOGD(TAG, "Disable privacy mode ISR");
	privacy = false;
}

void setup(void)
{
	global_status = 0;
	esp_log_level_set("*", ESP_LOG_NONE);
	esp_log_level_set("UTILS", ESP_LOG_DEBUG);
	Serial.begin(112500);

	tft.init();
	tft.setRotation(3);
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 0);
	tft.setTextDatum(TL_DATUM);
	tft.setTextSize(2);

	pinMode(interruptPin1, INPUT_PULLUP);
	pinMode(interruptPin2, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin1), isr1, FALLING);
	attachInterrupt(digitalPinToInterrupt(interruptPin2), isr2, FALLING);

	connectToWifi();
	configTime(3600 * 11, 0, ntpServer.c_str());
	tft.setSwapBytes(true);

	xTaskCreate(ui_task, "ui", 2048, NULL, 5, NULL);
}

// Main thread
void loop()
{
	if (!updated || millis() - last_update > 3600 * 1000)
	{
		global_status = 0;
		doc = fetchEvents(client, script_host, script_url);
		updated = true;
		last_update = millis();
		global_status = 1;
	}
}
