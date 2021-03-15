/**
    TTGO-Calendar-Dashboard
    @file utils.h
    @author Anuradha Wickramarachchi
*/

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#pragma once

String get_request(WiFiClientSecure &client, String host, String url, int redir_limit);

StaticJsonDocument<512> fetchEvents(WiFiClientSecure &client, String host, String url);

int printMultiline(TFT_eSPI &tft, String str, int x, int y);
