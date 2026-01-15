#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>

// Define NTP Client to get time
WiFiUDP   ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");

// see: https://github.com/JChristensen/Timezone/blob/master/examples/WorldClock/WorldClock.ino
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};  // Central European Summer Time
TimeChangeRule CET  = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone       CE(CEST, CET);

// UTC
TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};  // UTC
Timezone       UTC(utcRule);

Timezone currentTZ = CE;

static String getCurrentTime() {
  // update time with timeout protection
  int retries = 0;
  const int MAX_RETRIES = 5;
  
  while (!timeClient.update() && retries < MAX_RETRIES) {
    timeClient.forceUpdate();
    retries++;
    delay(100);  // Small delay between retries
  }
  
  if (retries >= MAX_RETRIES) {
    Serial.println("Warning: NTP update failed after retries");
  }
  
  time_t t = currentTZ.toLocal(timeClient.getEpochTime());
  char   buf[10];
  snprintf(buf, sizeof(buf), "%.2d:%.2d", hour(t), minute(t));

  return String(buf);
}

static int getHourOfDay() {
  // update time with timeout protection
  int retries = 0;
  const int MAX_RETRIES = 5;
  
  while (!timeClient.update() && retries < MAX_RETRIES) {
    timeClient.forceUpdate();
    retries++;
    delay(100);  // Small delay between retries
  }
  
  if (retries >= MAX_RETRIES) {
    Serial.println("Warning: NTP update failed after retries in getHourOfDay");
  }
  
  time_t t = currentTZ.toLocal(timeClient.getEpochTime());
  return hour(t);
}
