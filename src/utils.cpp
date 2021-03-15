/**
    TTGO-Calendar-Dashboard
    @file utils.cpp
    @author Anuradha Wickramarachchi
*/

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

#include "utils.h"

static const char *TAG = "UTILS";

int printMultiline(TFT_eSPI &tft, String str, int x, int y)
{
    String substr = "";
    int pl = 0;

    for (size_t i = 0; i < str.length(); i++)
    {
        if (tft.textWidth(substr + str[i]) >= 240 - x)
        {
            tft.drawString(substr, x, y);
            y += 20;
            substr = str[i];

            if (substr != " " && str[i - 1] != ' ')
            {
                substr = "-" + substr;
            }

            substr.trim();
            pl += substr.length();
        }
        else
        {
            substr += str[i];
        }
    }
    substr.trim();

    if (substr.length() > 0)
    {
        tft.drawString(substr, x, y);
        y += 20;
    }

    return y;
}

String get_request(WiFiClientSecure &client, String host, String url, int redir_limit = 2)
{
    String line, result = "";
    char c;

    unsigned int statusCode = -1;
    unsigned int pos = -1;
    unsigned int pos2 = -1;

    client.setInsecure();

    for (size_t i = 0; i < redir_limit; i++)
    {
        ESP_LOGD(TAG, "%s", host.c_str());
        ESP_LOGD(TAG, "%s", url.c_str());
        ESP_LOGD(TAG, "\n\nStarting connection to server...");

        if (!client.connect(host.c_str(), 443))
        {
            ESP_LOGD(TAG, "Connection failed!");
        }
        else
        {
            ESP_LOGD(TAG, "Connected to server!");
            client.println(String("GET ") + url + " HTTP/1.0");
            client.println(String("Host: " + host));
            client.println();

            while (client.connected())
            {
                // skip empty lines
                do
                {
                    line = client.readStringUntil('\n');
                } while (line.length() == 0);

                pos = line.indexOf("HTTP/1.0 ");
                pos2 = line.indexOf(" ", 9);

                if (!pos)
                {
                    statusCode = line.substring(9, pos2).toInt();
                }

                ESP_LOGD(TAG, "%s", line.c_str());
                if (statusCode == 302)
                {
                    ESP_LOGD(TAG, "Redirection Found");
                    client.find("Location: ");
                    client.readStringUntil('/');
                    client.readStringUntil('/');
                    host = client.readStringUntil('/');
                    url = String("https://") + host + String('/') + client.readStringUntil('\n');

                    while (1)
                    {
                        line = client.readStringUntil('\n');
                        if (line == "\r")
                        {
                            ESP_LOGD(TAG, "headers received redirect");
                            break;
                        }
                    }

                    // read all incoming bytes
                    while (client.available())
                        client.read();
                    client.stop();
                    break;
                }

                // complete reading header without redirects
                else
                {
                    while (1)
                    {
                        line = client.readStringUntil('\n');
                        if (line == "\r")
                        {
                            ESP_LOGD(TAG, "Headers received, no redirects");
                            break;
                        }
                    }

                    // read all incoming bytes
                    while (client.available())
                    {
                        c = client.read();
                        result += c;
                    }
                    client.stop();
                    result.trim();
                    return result;
                }
            }
        }
    }
    return String("[]");
}

StaticJsonDocument<512> fetchEvents(WiFiClientSecure &client, String host, String url)
{

    String result = get_request(client, host, url);
    
    ESP_LOGD(TAG, "%s", result.c_str());
    ESP_LOGD(TAG, "parsing");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, result);

    // Test if parsing succeeds.
    if (error)
    {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.f_str());
    }

    ESP_LOGD(TAG, "parsed");

    return doc;
}
