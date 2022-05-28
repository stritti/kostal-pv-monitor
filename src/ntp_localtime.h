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
  // update time
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  time_t t = currentTZ.toLocal(timeClient.getEpochTime());
  char   buf[10];
  sprintf(buf, "%.2d:%.2d", hour(t), minute(t));

  return String(buf);
}

static int getHourOfDay() {
  // update time
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  time_t t = currentTZ.toLocal(timeClient.getEpochTime());
  return hour(t);
}
