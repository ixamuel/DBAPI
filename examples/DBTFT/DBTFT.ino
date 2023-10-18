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

void setup() {
	Serial.begin(115200);
	tft.init();
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	tft.fillScreen(TFT_BLACK); // Use TFT_BLACK as the background color
	tft.setTextColor(TFT_WHITE); // Use TFT_WHITE as the foreground color
	tft.setRotation(1);
	tft.println("Verbinde...");
 	Serial.print("Verbinde...");
	while (WiFi.status() != WL_CONNECTED) {
		tft.write('.');
    	Serial.write('.');
		delay(500);
	}
	db.setAGFXOutput(true);
	Serial.println();
	fromStation = db.getStation(fromStationName);
	yield();
	timeClient.begin();
	timeClient.setTimeOffset(3600); // CET

	// Draw static content
	tft.fillScreen(TFT_BLACK); // Use TFT_BLACK as the background color
	tft.setTextColor(TFT_WHITE); // Use TFT_WHITE as the foreground color
	tft.setTextSize(2);
	//tft.setCursor((tft.width() - (strlen(fromStationName) * 6 - 1) * 2) / 2, 2);
	tft.setCursor(11 * 6 + 6, 2);
	tft.println(fromStationName);
	tft.fillRect(0, 18, tft.width(), 18, TFT_RED); // Use TFT_RED as the highlight color
	tft.setTextColor(TFT_BLACK); // Use TFT_BLACK as the text color on the highlighted area
	tft.setCursor(2, 20);
	tft.println("Zeit");
#ifdef WIDE_MODE
	tft.setCursor(11 * 6 + 6, 20);
#else
	tft.setCursor(9 * 6 + 6, 20);
#endif
	tft.println("Nach");
	tft.setCursor(tft.width() - 7 * 6 * 2, 20);
	tft.println("Gleis");
}

uint32_t scroll;
void loop() {
	if (nextTime < millis()) {
		timeClient.update();
		time_t tnow = timeClient.getEpochTime();
		time_t tdst = dst(tnow);
		if (old_time / 60 != tdst / 60) {
			tft.setTextColor(FOREGROUND_COLOR);
			tft.setTextSize(2);
			tft.setCursor(2, 2);
			tft.fillRect(2, 2, 5 * 6 * 2 , 16, BACKGROUND_COLOR);
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
			tft.fillRect(0, pos - 1, 11 * 6 + 4 , 17, FOREGROUND_COLOR);
#else
			tft.fillRect(0, pos - 1, 9 * 6 + 4 , 17, FOREGROUND_COLOR);
#endif
			tft.setTextColor(BACKGROUND_COLOR);
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
			tft.setTextColor(FOREGROUND_COLOR);
#ifdef WIDE_MODE
			tft.fillRect(11 * 6 + 4, pos - 1, tft.width(), 17, BACKGROUND_COLOR);
			printScroll(depature->target, 11 * 6 + 6, pos, true, strcmp("cancel", depature->textdelay) == 0);
#else
			tft.fillRect(9 * 6 + 4, pos - 1, tft.width(), 17, BACKGROUND_COLOR);
			tft.setCursor(9 * 6 + 6, pos);
			tft.setTextSize(1);
			tft.print(depature->target);
#endif

#ifndef WIDE_MODE
			tft.setTextColor(HIGHLIGHT_COLOR);
			tft.setCursor(9 * 6 + 6, pos + 8);
			if (strcmp("cancel", depature->textdelay) == 0) {
				tft.print("Fahrt f\x84llt aus");
				tft.drawFastHLine(9 * 6 + 6, pos + 3, strlen(depature->target) * 6 - 1, FOREGROUND_COLOR);
			} else if (strcmp("0", depature->textdelay) != 0 && strcmp("-", depature->textdelay) != 0) {
				tft.print("ca. ");
				tft.print(depature->delay);
				tft.print(" Minuten sp\x84ter");
			}
#endif
			tft.setTextColor(FOREGROUND_COLOR);
			tft.setTextSize(2);
			tft.setCursor(tft.width() - 7 * 6 * 2, pos);
			if (strcmp("", depature->newPlatform) != 0) {
				tft.setTextColor(HIGHLIGHT_COLOR);
				tft.print(depature->newPlatform);
			} else {
				tft.print(depature->platform);
			}
			depature = depature->next;
		}
		nextCheck = millis() + 50000;
	}
#ifdef WIDE_MODE
	if (nextScroll < millis()) {
		depature = da;
		uint16_t pos = 21;
	    while (depature != NULL) {
			pos += 18;
			printScroll(depature->target, 11 * 6 + 6, pos, false, strcmp("cancel", depature->textdelay) == 0);
			depature = depature->next;
		}
		scroll++;
		nextScroll = millis() + SCROLL_INTERVAL;
	}
#endif
}

void printScroll(String text, uint16_t x, uint16_t y, bool force, bool cancelled) {
	tft.setTextColor(FOREGROUND_COLOR);
	tft.setTextSize(2);
	if (text.length() > SCROLL_CHARS || force) {
		uint32_t p = scroll;
		int16_t ts = text.length() - SCROLL_CHARS;
		if (ts < 0) {
			ts = 0;
		}
		uint32_t to_scroll = ts;
		p %= to_scroll * 2 + SCROLL_STALL * 2;
		if (p <= to_scroll) {
			if (!force && p == 0) return; // do not update 0 position
		} else if (p <= to_scroll + SCROLL_STALL) {
			p = to_scroll;
			if (!force) return; // do not update on stall, if no update is forced
		} else if (p <= to_scroll * 2 + SCROLL_STALL) {
			p = SCROLL_STALL + to_scroll * 2 - p;
		} else {
			p = 0;
			if (!force) return; // do not update on stall, if no update is forced
		}
		tft.fillRect(x - 2, y - 1, SCROLL_CHARS * 6 * 2, 17, BACKGROUND_COLOR);
		tft.setCursor(x, y);
		text = text.substring(p, p + SCROLL_CHARS);
		tft.print(text);
		if (cancelled) {
			uint8_t len = min((uint8_t) text.length(), (uint8_t) SCROLL_CHARS);
			tft.drawFastHLine(x, y + 6, (len * 6 - 1) * 2, FOREGROUND_COLOR);
			tft.drawFastHLine(x, y + 7, (len * 6 - 1) * 2, FOREGROUND_COLOR);
		}
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
