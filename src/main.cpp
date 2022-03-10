/**
  Kostal Plenticore DC/DC converter Monitor:
*/

#if !( defined(ESP32) )
#error This code is intended to run on the ESP32 platform!
#endif


#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <WiFiSettings.h>

#include <ESPAsyncWebServer.h>

#include <ModbusIP_ESP8266.h>
#include "kostal_modbus.h"


#if ( defined(TTGO_EINK) )
#include <GxEPD.h>
#include <GxGDE0213B1/GxGDE0213B1.cpp> // 2.13" b/w

#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>


const String  hostname                  = "pv-monitor";
const int32_t DATA_UPDATE_DELAY_SECONDS = 60;  // Show result every second

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
RTC_DATA_ATTR int wakeupCount = 0; // RTC counter variable
#define MODEM_POWER_ON 23

#define LED_BUILTIN 2 // built-in LED on TTGO-T5

#else
#include <U8g2lib.h>
#endif

WiFiClient    theClient;  // Set up a client for the WiFi connection

IPAddress remote;  // Address of Modbus Slave device

#if ( defined(TTGO_EINK) )
// e-ink display of TTGO T5

// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, SS, 17, 16); // arbitrary selection of rst=17, busy=16
// GxGDEP015OC1(GxIO& io, uint8_t rst = D4, uint8_t busy = D2);
GxEPD_Class display(io, 16, 4); // arbitrary selection of (16), 4
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

typedef enum
{
  RIGHT_ALIGNMENT = 0,
  LEFT_ALIGNMENT,
  CENTER_ALIGNMENT,
} Text_alignment;
#else
// Display hardware I2C interface constructor for 0,96" OLED display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(/* rotation=*/U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#endif

AsyncWebServer server(80);

ModbusIP      mb;                             //ModbusTCP object
String        kostal_hostname;         // hostname of the Modbus TCP server
uint16_t      kostal_modbus_port;             // port of the Modbus TCP server
uint32_t      modbus_query_last      = 0;

/**
 * @brief reconstruct the float from 2 unsigned integers
 *
 */
float f_2uint_float(unsigned int uint1, unsigned int uint2) {
  union f_2uint {
    float    f;
    uint16_t i[2];
  };

  union f_2uint f_number;
  f_number.i[0] = uint1;
  f_number.i[1] = uint2;

  return f_number.f;
}

/**
 * @brief
 *
 * @param event
 * @param transactionId
 * @param data
 * @return true
 * @return false
 */
bool cb(Modbus::ResultCode event, uint16_t transactionId, void* data) {  // Callback to monitor errors
  if (event != Modbus::EX_SUCCESS) {
    Serial.print("Request result: 0x");
    Serial.println(event, HEX);
  }
  return true;
}

/**
 * @brief Get the float object
 *
 * @param reg
 * @return float
 */
float get_float(uint16_t reg) {  // get the float from the Modbus register
  uint16_t numregs = 2;
  uint16_t value[numregs];

  while (millis() - modbus_query_last < MODBUS_QUERY_DELAY) {
    if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
      mb.task();
      delay(10);
    }
  }

  if (!mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
    mb.connect(remote, kostal_modbus_port);  // Try to connect if no connection
  }
  uint16_t trans = mb.readHreg(remote, reg, value, numregs, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
  while (mb.isTransaction(trans)) {                                          // Check if transaction is active
    mb.task();
    delay(10);
  }
  float float_reconstructed = f_2uint_float(value[0], value[1]);
  return float_reconstructed;
}

/**
 * @brief Get the uint16 object
 *
 * @param reg
 * @return uint16_t
 */
uint16_t get_uint16(uint16_t reg) {  // get the int16 from the Modbus register
  uint16_t res;

  while (millis() - modbus_query_last < MODBUS_QUERY_DELAY) {
    if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
      mb.task();
      delay(10);
    }
  }

  if (!mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
    mb.connect(remote, kostal_modbus_port);  // Try to connect if no connection
  }

  uint16_t trans = mb.readHreg(remote, reg, &res, 1, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
  while (mb.isTransaction(trans)) {                                                // Check if transaction is active
    mb.task();
    delay(10);
  }

  return res;
}

/**
 * @brief write the text to the display
 *
 * @param y
 * @param label
 * @param unit
 * @param value
 */
void writeToDisplay(uint16_t y, const char* label, const char* unit, float value) {
  char buffer[50];
  if (value < 1000 && value > -1000) {
    sprintf(buffer, "%s %4.0d%s", label, (int)value, unit);
  } else {
    sprintf(buffer, "%s %2.2dk%s", label, (int)value / 1000, unit);
  }
  Serial.println(buffer);

#if ( defined(TTGO_EINK) )
  display.setTextColor(GxEPD_BLACK);
  //display.setFont(&FreeSans9pt7b);
  display.setFont(&FreeMono12pt7b);
  display.setTextSize(1);

  display.setCursor(0, y*20);
  display.print(buffer);


#else
  u8g2.setFont(u8g2_font_t0_17b_tr);
  u8g2.setCursor(0, y * 16);
  u8g2.print(buffer);
#endif
}

/**
 * @brief write the text to the display
 *
 * @param y
 * @param label
 * @param unit
 * @param value
 */
void writeToDisplay(uint16_t y, const char* label, const char* unit, uint16_t value) {
  char buffer[50];
  sprintf(buffer, "%s %4.0d%s", label, value, unit);
  Serial.println(buffer);
#if ( defined(TTGO_EINK) )
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMono12pt7b);
  display.setTextSize(1);

  display.setCursor(0, y*20);
  display.print(buffer);
#else
  u8g2.setFont(u8g2_font_t0_17b_tr);
  //u8g2.setFont(u8g2_font_profont15_tr);  // choose a suitable font
  u8g2.setCursor(0, y * 16);
  u8g2.print(buffer);
#endif
}

/**
 * @brief
 *
 * @param soc
 */
void drawBattery(uint16_t soc) {
  // draw battery
  char buffer[50];
  sprintf(buffer, "%d", (soc + 5) / 20);
#if ( defined(TTGO_EINK) )
  display.setTextColor(GxEPD_BLACK);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_battery19_tn);
  u8g2_for_adafruit_gfx.drawStr(170, 42, buffer);

#else
  u8g2.setFont(u8g2_font_battery19_tn);
  u8g2.drawStr(107, 22, buffer);
#endif
}

/**
 * @brief draw smiley
 *
 */
void drawSmiley(float own_consumption_grid, float own_consumption_pv, float own_consumption_batt) {
  uint8_t smiley;
  if (own_consumption_grid > (own_consumption_pv + own_consumption_batt)) {
    smiley = 0x0026; /* hex 26 sad man */
  } else if (own_consumption_pv > (own_consumption_batt + own_consumption_grid)) {
    smiley = 0x0036; /* hex 36 sunglass smily man */
  } else {
    smiley = 0x0021; /* hex 21 smily man */
  }
#if ( defined(TTGO_EINK) )
  u8g2_for_adafruit_gfx.setFontMode(0);
  u8g2_for_adafruit_gfx.setForegroundColor(0);
  u8g2_for_adafruit_gfx.setBackgroundColor(1);
  // u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_emoticons21_tr);
  u8g2_for_adafruit_gfx.drawGlyph(170, 76, smiley);
#else
  u8g2.setFont(u8g2_font_emoticons21_tr);
  u8g2.drawGlyph(100, 64, smiley);
#endif
}
void writeOwnConsumption () {
  writeToDisplay(1, "Kostal Monitor", "",(uint16_t) 0);

  uint16_t battery_soc = get_uint16(KOSTAL_MODBUS_REG_BATTERY_SOC);
  float own_consumption_grid = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_GRID);
  float own_consumption_pv = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_PV);
  float own_consumption_batt = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_BATTERY);

  drawBattery(battery_soc);
  drawSmiley(own_consumption_grid, own_consumption_pv, own_consumption_batt);

  writeToDisplay(2, "SOC: ", "%", battery_soc);
  writeToDisplay(3, "PV:  ", "W", own_consumption_pv); // own consumption PV
  writeToDisplay(4, "Akku:", "W", own_consumption_batt); // own consumption battery
  writeToDisplay(5, "Netz:", "W", own_consumption_grid); // own consumption grid

  display.update();
}

/**
 * @brief
 *
 */
void setup() {
  Serial.begin(SERIAL_SPEED);
  while (!Serial) {
  }

  Serial.println(F("-------------------------------------------"));
  Serial.println(F(" Kostal Plenticore DC/DC converter Monitor "));
  Serial.println(F("-------------------------------------------"));

  SPIFFS.begin(true);  // Will format on the first run after failing to mount

#if ( defined(TTGO_EINK) )
  display.init(SERIAL_SPEED);
  u8g2_for_adafruit_gfx.begin(display);
  display.setRotation(3);
  // display.eraseDisplay();

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);

  display.setCursor(0, 17);
  display.print("Kostal Monitor");
  display.update();
#else
  u8g2.begin();
  u8g2.clearBuffer();                 // clear the internal memory
  u8g2.setFont(u8g2_font_t0_15b_tr);  // choose a suitable font
  u8g2.setCursor(0, 17);
  u8g2.print(F("Kostal Monitor"));
  u8g2.sendBuffer();  // transfer internal memory to the display
#endif

  WiFiSettings.onPortal = []() {
#if ( defined(TTGO_EINK) )

#else
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_t0_15b_tr);  // choose a suitable font
    u8g2.setCursor(0, 14);
    u8g2.println("Setup Device");
    u8g2.setCursor(0, 28);
    u8g2.println("---------------");
    u8g2.setCursor(0, 42);
    u8g2.println(F("Connect to WiFi:"));
    u8g2.setCursor(8, 56);
    u8g2.println(hostname);
    u8g2.sendBuffer();  // transfer internal memory to the display
#endif
  };

  // Set custom callback functions
  WiFiSettings.onSuccess = []() {
    IPAddress wIP = WiFi.localIP();
    Serial.printf("WiFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

    //WiFi.mode(WIFI_STA);
    //WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    //WiFi.setHostname(hostname.c_str());  //define hostname

    WiFi.hostByName(kostal_hostname.c_str(), remote);
    Serial.print(F("Connecting to Modbus Slave: "));
    Serial.print(kostal_hostname);
    Serial.print(" IP address: ");
    Serial.println(remote);
    // Set up ModbusTCP client.
    mb.slave(KOSTAL_MODBUS_SLAVE_ID);

#if ( defined(TTGO_EINK) )

#else
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_t0_15b_tr);  // choose a suitable font
    u8g2.setCursor(0, 16);
    u8g2.print(F("Connected to:"));
    u8g2.setCursor(0, 32);
    u8g2.print(kostal_hostname);
    u8g2.setCursor(0, 48);
    u8g2.print(F("Loading data ..."));
    u8g2.sendBuffer();  // transfer internal memory to the display
#endif
  };

  WiFiSettings.onFailure = []() {
#if ( defined(TTGO_EINK) )

#else
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_t0_15b_tr);  // choose a suitable font
    u8g2.setCursor(2, 32);
    u8g2.print(F("WiFi connection failed."));
    u8g2.sendBuffer();  // transfer internal memory to the display
#endif
  };

  WiFiSettings.hostname = hostname.c_str();
  // Define custom settings saved by WifiSettings
  // These will return the default if nothing was set before
  kostal_hostname = WiFiSettings.string("kostal_hostname", "hostname", "KOSTAL Hostname");
  kostal_modbus_port     = WiFiSettings.integer("kostal_modbus_port", 1, 65535, 1502, "KOSTAL Modbus Port");

  // Connect to WiFi with a timeout of 30 seconds
  // Launches the portal if the connection failed
  WiFiSettings.connect(true, 30);

  // TODO: Show Warning if level X is reached
  Serial.print(F("Battery Temp: "));
  Serial.print(get_float(214));
  Serial.println(F("\u00B0C"));

#if ( defined(TTGO_EINK) )
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.print("Boot Count : ");
    Serial.println(wakeupCount);
  }

  // Set up the display
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);

  writeOwnConsumption();


  Serial.printf("Going to sleep now for %d sec.\n", (DATA_UPDATE_DELAY_SECONDS));
  esp_sleep_enable_timer_wakeup(DATA_UPDATE_DELAY_SECONDS * uS_TO_S_FACTOR);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, LOW);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  pinMode(LED_BUILTIN, OUTPUT);

  esp_deep_sleep_start();
  Serial.println("This will never be printed");

#endif
}

uint32_t showLast       = 0;
bool     showConsumtion = true;

/**
 * @brief
 *
 */
void loop() {
#if ( !defined(TTGO_EINK) )
  if (millis() - showLast >
      (DATA_UPDATE_DELAY_SECONDS * 1000)) {  // Display register value every n seconds (with default settings)
    showLast = millis();

    Serial.print(F("Total DC power: "));
    Serial.print(get_float(100));
    Serial.println(F(" W"));
    Serial.println(F(" "));

    u8g2.clearBuffer();  // clear the internal memory

    if (showConsumtion == true) {
      writeToDisplay(1, "SOC ", "%", get_uint16(514)); // battery SoC
      writeToDisplay(2, "PV  ", "W", get_float(116)); // own consumption PV
      writeToDisplay(3, "Akku", "W", get_float(106)); // own consumption battery
      writeToDisplay(4, "Netz", "W", get_float(108)); // own consumption grid
    } else {
      writeToDisplay(2, "PVgen", "W", get_float(224) + get_float(234));
      writeToDisplay(3, "react", "W", get_float(254));
      writeToDisplay(4, "appar", "W", get_float(256));
    }

    drawBattery(battery_soc);

    // draw smiley
    u8g2.setFont(u8g2_font_emoticons21_tr);
    if (ownConsumption_grid > (ownConsumption_pv + ownConsumption_battery)) {
      u8g2.drawGlyph(100, 64, 0x0026); /* hex 26 sad man */
    } else if (ownConsumption_pv > (ownConsumption_battery + ownConsumption_grid)) {
      u8g2.drawGlyph(100, 64, 0x0036); /* hex 36 sunglass smily man */
    } else {
      u8g2.drawGlyph(100, 64, 0x0021); /* hex 21 smily man */
    }

    u8g2.sendBuffer();  // transfer internal memory to the display

    showConsumtion = !showConsumtion;
  }
#endif
}
