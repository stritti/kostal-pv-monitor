/**
  Kostal Plenticore DC/DC converter Monitor:

*/
#include <Arduino.h>
#include <WiFi.h>

#include <ModbusIP_ESP8266.h>
#include <U8g2lib.h>

// settings managed through a private_config.ini file
#include "config.hpp"

WiFiClient theClient;  // Set up a client for the WiFi connection

IPAddress    remote;  // Address of Modbus Slave device
const String hostname = "Kostal Plenticore Monitor";
ModbusIP     mb;  //ModbusTCP object

// Display hardware I2C interface constructor
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(/* rotation=*/U8G2_R0, /* reset=*/U8X8_PIN_NONE);

const int32_t modbus_query_delay = 1500;  // Show result every n'th millisecond
uint32_t      modbus_query_last  = 0;

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

  while (millis() - modbus_query_last < modbus_query_delay) {
    if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
      mb.task();
      delay(10);
    }
  }

  if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
    uint16_t trans =
        mb.readHreg(remote, reg, value, numregs, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
    while (mb.isTransaction(trans)) {                                          // Check if transaction is active
      mb.task();
      delay(10);
    }
  } else {
    mb.connect(remote, KOSTAL_MODBUS_PORT);  // Try to connect if no connection
    Serial.println("Modbus connected.");
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

  while (millis() - modbus_query_last < modbus_query_delay) {
    if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
      mb.task();
      delay(10);
    }
  }

  if (mb.isConnected(remote)) {  // Check if connection to Modbus Slave is established
    uint16_t trans = mb.readHreg(remote, reg, &res, 1, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
    while (mb.isTransaction(trans)) {                                                // Check if transaction is active
      mb.task();
      delay(10);
    }
  } else {
    mb.connect(remote, KOSTAL_MODBUS_PORT);  // Try to connect if no connection
    Serial.println("Modbus connected.");
  }

  return res;
}

/**
 * @brief
 *
 * @param y
 * @param label
 * @param unit
 * @param value
 */
void writeToDisplay(u8g2_uint_t y, const char* label, const char* unit, float value) {  // write the text to the display
  char buffer[50];
  sprintf(buffer, "%s %4.0d%s", label, (int)value, unit);
  u8g2.setFont(u8g2_font_t0_17b_tr);
  //u8g2.setFont(u8g2_font_profont15_tr);  // choose a suitable font
  u8g2.setCursor(0, y * 16);
  u8g2.print(buffer);
}

/**
 * @brief
 *
 * @param soc
 */
void drawBattery(uint16_t soc) {
  // draw battery
  char buffer[50];
  u8g2.setFont(u8g2_font_battery19_tn);
  sprintf(buffer, "%d", (soc + 5) / 20);
  u8g2.drawStr(107, 22, buffer);
}

/**
 * @brief
 *
 */
void setup() {
  Serial.begin(74880);
  while (!Serial) {
  }

  Serial.println(F("-------------------------------------------"));
  Serial.println(F(" Kostal Plenticore DC/DC converter Monitor "));
  Serial.println(F("-------------------------------------------"));

  u8g2.begin();
  u8g2.clearBuffer();                 // clear the internal memory
  u8g2.setFont(u8g2_font_t0_17b_tr);  // choose a suitable font
  u8g2.setCursor(0, 17);
  u8g2.print(F("Kostal Plenticore"));
  u8g2.setCursor(0, 48);
  u8g2.print(F("Setup WiFi ..."));
  u8g2.sendBuffer();  // transfer internal memory to the display

  Serial.print(F("Connecting to WiFi SSID: "));
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str());  //define hostname

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(F(" ."));
  }
  Serial.println(F(""));

  IPAddress wIP = WiFi.localIP();
  Serial.printf("WiFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

  WiFi.hostByName(KOSTAL_MODBUS_HOSTNAME, remote);
  Serial.print(F("Connecting to Modbus Slave: "));
  Serial.print(KOSTAL_MODBUS_HOSTNAME);
  Serial.print(" IP address: ");
  Serial.println(remote);
  // Set up ModbusTCP client.
  mb.slave(KOSTAL_MODBUS_SLAVE_ID);

  u8g2.clearBuffer();                 // clear the internal memory
  u8g2.setFont(u8g2_font_t0_17b_tr);  // choose a suitable font
  u8g2.setCursor(2, 32);
  u8g2.print(F("Connected."));
  u8g2.sendBuffer();  // transfer internal memory to the display
}

const int32_t showDelay = 10000;  // Show result every n'th millisecond
uint32_t      showLast  = 0;

/**
 * @brief
 *
 */
void loop() {

  if (millis() - showLast > showDelay) {  // Display register value every n seconds (with default settings)
    showLast = millis();

    float    battery_temp           = get_float(214);
    uint16_t battery_soc            = get_uint16(514);
    float    ownConsumption_battery = get_float(106);
    float    ownConsumption_grid    = get_float(108);
    float    ownConsumption_pv      = get_float(116);
    float    ownConsumption_sum     = ownConsumption_battery + ownConsumption_grid + ownConsumption_pv;

    Serial.print(F("SoC: "));
    Serial.print(battery_soc);
    Serial.println(F(" %"));

    Serial.print(F("Battery Temp: "));
    Serial.print(battery_temp);
    Serial.println(F("°C"));

    Serial.print(F("Total DC power: "));
    Serial.print(get_float(100));
    Serial.println(F(" W"));

    Serial.print(F("Home own consumption from battery: "));
    Serial.print(ownConsumption_battery);
    Serial.println(F(" W"));

    Serial.print(F("Home own consumption from grid: "));
    Serial.print(ownConsumption_grid);
    Serial.println(F(" W"));

    Serial.print(F("Home own consumption from PV: "));
    Serial.print(ownConsumption_pv);
    Serial.println(F(" W"));

    Serial.print(F("Total home own consumption: "));
    Serial.print(ownConsumption_sum);
    Serial.println(F(" W"));

    Serial.print(F("Power limit from EVU: "));
    Serial.print(get_uint16(122));
    Serial.println(F(" %"));

    Serial.println(F(" "));

    u8g2.clearBuffer();  // clear the internal memory

    u8g2.setFont(u8g2_font_t0_17b_tr);
    char buffer_soc[50];
    sprintf(buffer_soc, "SoC   %3.0d%%", battery_soc);
    u8g2.setCursor(0, 15);
    u8g2.print(buffer_soc);

    writeToDisplay(2, "PV  ", "W", ownConsumption_pv);
    writeToDisplay(3, "Akku", "W", ownConsumption_battery);
    writeToDisplay(4, "Netz", "W", ownConsumption_grid);

    drawBattery(battery_soc);

    // draw smiley
    u8g2.setFont(u8g2_font_emoticons21_tr);
    if (ownConsumption_grid < 50) {
      u8g2.drawGlyph(100, 64, 0x0021); /* hex 21 smily man */
    } else if (ownConsumption_pv > ownConsumption_battery) {
      u8g2.drawGlyph(100, 64, 0x0036); /* hex 36 sunglass smily man */
    } else {
      u8g2.drawGlyph(100, 64, 0x0026); /* hex 26 sad man */
    }

    u8g2.sendBuffer();  // transfer internal memory to the display
  }
}