#include <Arduino.h>
#include <WiFi.h>
#include <DBAPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <TFT_eSPI.h> // Include the graphics library for the TTGO TFT Display

const char* ssid = "SSID";
const char* password = "password";
const char* fromStationName = "Hannnover Hbf";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define BACKGROUND_COLOR 0x001F // Blue
#define FOREGROUND_COLOR 0xFFFF // White
#define HIGHLIGHT_COLOR  0xFFE0 // Yellow

// Optional view with large destinations, does interfere with platform when station names are longer
#define WIDE_MODE
#define SCROLL_CHARS     13
#define SCROLL_STALL      5
#define SCROLL_INTERVAL 400

DBAPI db;
DBstation* fromStation;
DBdeparr* da = NULL;
DBdeparr* depature = NULL;
uint32_t nextCheck;
uint32_t nextTime;
uint32_t nextScroll;
time_t old_time;

TFT_eSPI tft = TFT_eSPI(); // Create an instance of the library

void loop() {
    // Update time client if it's time
    if (nextTime < millis()) {
        updateTimeClient();
        updateDisplayTime();
        nextTime = millis() + 1000;
    }

    // Check WiFi status and reload departures if it's time
    if (nextCheck < millis()) {
        checkWiFiStatus();
        reloadDepartures();
        nextCheck = millis() + 50000;
    }

#ifdef WIDE_MODE
    // Scroll the display if it's time
    if (nextScroll < millis()) {
        scrollDisplay();
        nextScroll = millis() + SCROLL_INTERVAL;
    }
#endif
}

void updateTimeClient() {
    timeClient.update();
}

void updateDisplayTime() {
    time_t currentTime = timeClient.getEpochTime();
    time_t dstTime = dst(currentTime);

    if (old_time / 60 != dstTime / 60) {
        displayTime(dstTime);
        old_time = dstTime;
    }
}

void checkWiFiStatus() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Disconnected");
    }
}

void reloadDepartures() {
    Serial.println("Reload");
    da = db.getDepatures(fromStation->stationId, NULL, NULL, NULL, 11, PROD_RE | PROD_S);
    Serial.println();

    depature = da;
    uint16_t pos = 21;

    while (depature != NULL) {
        pos += 18;
        if (pos + 16 > tft.height()) break;

        displayDeparture(pos);
        depature = depature->next;
    }
}

void displayDeparture(uint16_t pos) {
#ifdef WIDE_MODE
    displayWideModeDeparture(pos);
#else
    displayNormalModeDeparture(pos);
#endif

#ifndef WIDE_MODE
    displayDelayInfo(pos);
#endif

    displayPlatformInfo(pos);
}

void displayWideModeDeparture(uint16_t pos) {
// ...
}

void displayNormalModeDeparture(uint16_t pos) {
// ...
}

void displayDelayInfo(uint16_t pos) {
// ...
}

void displayPlatformInfo(uint16_t pos) {
// ...
}

void scrollDisplay() {
// ...
}


uint32_t scroll;
void loop() {
	if (nextTime < millis()) {
		timeClient.update();
		time_t tnow = timeClient.getEpochTime();
		time_t tdst = dst(tnow);
		if (old_time / 60 != tdst / 60) {
			tft.setTextColor(TFT_WHITE);
			tft.setTextSize(2);
			tft.setCursor(2, 2);
			tft.fillRect(2, 2, 5 * 6 * 2 , 16, TFT_RED);
			if (hour(tdst) < 10) tft.write('0');
			tft.print(hour(tdst));
			tft.write(':');
			if (minute(tdst) < 10) tft.write('0');
			tft.print(minute(tdst));
			old_time = tdst;
		}
		nextTime = millis() + 1000;
	}
	if (nextCheck < millis()) {
		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("Disconnected");
		}
		Serial.println("Reload");
		da = db.getDepatures(fromStation->stationId, NULL, NULL, NULL, 11, PROD_RE | PROD_S);
		Serial.println();
		depature = da;
		uint16_t pos = 21;
	    while (depature != NULL) {
			pos += 18;
			if (pos + 16 > tft.height()) break;
#ifdef WIDE_MODE
			tft.fillRect(0, pos - 1, 11 * 6 + 4 , 17, (TFT_RED);
#else
			tft.fillRect(0, pos - 1, 9 * 6 + 4 , 17, TFT_WHITE);
#endif
			tft.setTextColor(TFT_BLACK);
			tft.setTextSize(1);
			tft.setCursor(2, pos);
			tft.print(depature->time);
#ifdef WIDE_MODE
			if (strcmp("cancel", depature->textdelay) != 0 && strcmp("0", depature->textdelay) != 0 && strcmp("-", depature->textdelay) != 0) {
				tft.write(' ');
				tft.print(depature->textdelay);
			}
#endif
			tft.setCursor(2, pos + 8);
			tft.print(depature->product);
			if (strcmp("", depature->textline) != 0) {
				tft.write(' ');
				tft.print(depature->textline);
			}
			
#ifndef WIDE_MODE
            // Display the target station and platform number without scrolling
            tft.setTextColor(TFT_WHTIE);
            tft.setCursor(9 * 6 + 6, pos);
            tft.setTextSize(1);
            tft.print(depature->target);

            tft.setTextColor(TFT_RED);
            tft.setCursor(tft.width() - 7 * 6 * 2, pos);
            if (strcmp("", depature->newPlatform) != 0) {
                tft.setTextColor(HIGHLIGHT_COLOR);
                tft.print(depature->newPlatform);
            } else {
                tft.print(depature->platform);
            }
#endif

            depature = depature->next;
        }
        nextCheck = millis() + 50000;
    }
}

time_t dst(time_t t) {
	if (month(t) > 3 && month(t) < 10) { //Apr-Sep
		return t + 3600;
	} else if (month(t) == 3 && day(t) - weekday(t) >= 24) { //Date at or after last sunday in March
		if (weekday(t) == 1) { //Sunday to switch to dst
			if (hour(t) >= 2) { //Time after 2AM
				return t + 3600;
			}
		} else { //Date after last sunday in March
			return t + 3600;
		}
	} else if (month(t) == 10 && day(t) - weekday(t) < 24) { //Date before last sunday in October
		return t + 3600;
	} else if (month(t) == 10 && day(t) - weekday(t) >= 24) { //Date at or after last sunday in March
		if (weekday(t) == 1) { //Sunday to switch back from dst
			if (hour(t) < 3) { //Time before 2AM without DST (3AM DST, which doesn't exist)
				return t + 3600;
			}
		}
	}
	return t;
}
