/**
  Kostal Plenticore DC/DC converter Monitor:
*/

#if !(defined(ESP32))
#error This code is intended to run on the ESP32 platform!
#endif

#define TTGO_T5_1_2 1  // see defines in board_def.h
//#define TTGO_T5_2_3 1  // see defines in board_def.h
#include "board_def.h"

#include "ntp_localtime.h"
#include "kostal_modbus.h"
#include "u8g2_display.h"

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <WiFiSettings.h>

const String  hostname                  = "kostal-monitor";
const int32_t DATA_UPDATE_DELAY_SECONDS = 180;  // Show result every second

#define uS_TO_S_FACTOR 1000000        // Conversion factor for micro seconds to seconds
RTC_DATA_ATTR int wakeUpCounter = 0;  // RTC counter variable
#define MODEM_POWER_ON 23

#define LED_BUILTIN 2  // built-in LED on TTGO-T5

// Modbus transaction timeout in milliseconds
const unsigned long MODBUS_TRANSACTION_TIMEOUT_MS = 5000;

// Modbus connection settings
const unsigned long MODBUS_CONNECTION_TIMEOUT_MS = 3000;  // Timeout for establishing connection
const unsigned long MODBUS_DISCONNECT_TIMEOUT_MS = 2000;  // Timeout for disconnect loop
const unsigned long MODBUS_CONNECTION_SETTLE_MS = 500;    // Wait after connect before first transaction
const int MODBUS_MAX_RETRIES = 3;                         // Maximum connection retry attempts

// Time range for display updates (7 AM to 11 PM)
const int DISPLAY_UPDATE_START_HOUR = 7;
const int DISPLAY_UPDATE_END_HOUR = 23;

WiFiClient theClient;  // Set up a client for the WiFi connection
IPAddress  remote;     // Address of Modbus Slave device

ModbusIP mb;                  //ModbusTCP object
String   kostal_hostname;     // hostname of the Modbus TCP server
uint16_t kostal_modbus_port;  // port of the Modbus TCP server
uint32_t modbus_query_last = 0;
bool     modbus_connection_stable = false;  // Track if connection is stable

/**
 * @brief Reconstruct a float value from two unsigned 16-bit integers.
 * 
 * Used to convert Modbus register pairs into floating point values
 * according to IEEE 754 format.
 * 
 * @param uint1 First 16-bit word (lower bytes)
 * @param uint2 Second 16-bit word (upper bytes)
 * @return float Reconstructed floating point value
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
 * @brief Callback for Modbus TCP connection events.
 * 
 * Handles connection errors and timeouts by disconnecting and dropping transactions.
 * 
 * @param event Result code from Modbus operation
 * @param transactionId Transaction identifier
 * @param data Additional data from the transaction
 * @return true Always returns true to continue operation
 */
bool cb(Modbus::ResultCode event, uint16_t transactionId, void* data) {  // Callback to monitor errors

  if (event == Modbus::EX_TIMEOUT) {  // If Transaction timeout took place
    Serial.println("Modbus timeout - disconnecting");
    mb.disconnect(remote);            // Close connection to slave and
    mb.dropTransactions();            // Cancel all waiting transactions
    modbus_connection_stable = false; // Mark connection as unstable
  }
  if (event != Modbus::EX_SUCCESS) {
    Serial.print("Request result: 0x");
    Serial.println(event, HEX);
  }
  return true;
}

/**
 * @brief Establish a stable Modbus TCP connection with retry logic.
 * 
 * Attempts to connect to the Modbus server with multiple retries and
 * waits for connection to stabilize before returning.
 * 
 * @return true if connection established successfully, false otherwise
 */
bool establishModbusConnection() {
  if (remote == INADDR_NONE) {
    Serial.println("Error: Invalid remote IP address for Modbus connection");
    return false;
  }
  
  // If already connected and stable, verify it's still alive
  if (mb.isConnected(remote) && modbus_connection_stable) {
    mb.task();  // Process any pending tasks
    return true;
  }
  
  // Disconnect if previously connected but not stable
  if (mb.isConnected(remote)) {
    mb.disconnect(remote);
    delay(100);
  }
  
  // Try to establish connection with retries
  for (int attempt = 0; attempt < MODBUS_MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      Serial.printf("Modbus connection retry %d/%d\n", attempt + 1, MODBUS_MAX_RETRIES);
      // Exponential backoff: 500ms, 1000ms, 2000ms
      unsigned long backoffDelay = 500 * (1 << attempt);  // Exponential: 500 * 2^attempt
      delay(backoffDelay);
    }
    
    mb.connect(remote, kostal_modbus_port);
    
    // Wait for connection to establish with timeout
    unsigned long connectStart = millis();
    while (!mb.isConnected(remote)) {
      if (millis() - connectStart >= MODBUS_CONNECTION_TIMEOUT_MS) {
        break;  // Timeout
      }
      mb.task();
      yield();  // Allow other tasks to run
      delay(10);
    }
    
    if (mb.isConnected(remote)) {
      Serial.println("Modbus connected, waiting for connection to settle...");
      // Give connection time to stabilize with non-blocking approach
      unsigned long settleStart = millis();
      while (millis() - settleStart < MODBUS_CONNECTION_SETTLE_MS) {
        mb.task();
        yield();
        delay(10);
      }
      
      // Verify still connected after settle time
      if (mb.isConnected(remote)) {
        modbus_connection_stable = true;
        Serial.println("Modbus connection stable");
        return true;
      }
    }
  }
  
  Serial.println("Failed to establish stable Modbus connection");
  modbus_connection_stable = false;
  return false;
}

/**
 * @brief Read a float value from Modbus register.
 * 
 * Reads two consecutive holding registers and combines them into a float.
 * Implements rate limiting, connection validation, and timeout protection.
 * 
 * @param reg Modbus register address to read from
 * @return float The float value read from the registers, or 0.0f on error
 */
float get_float(uint16_t reg) {  // get the float from the Modbus register
  uint16_t numregs = 2;
  uint16_t value[numregs] = {0};  // Initialize to prevent use of uninitialized memory

  // Rate limiting - only applies if connected
  if (mb.isConnected(remote)) {
    while (millis() - modbus_query_last < MODBUS_QUERY_DELAY) {
      mb.task();
      delay(10);
    }
  }

  // Ensure we have a stable connection
  if (!establishModbusConnection()) {
    Serial.printf("Error: Cannot read register 0x%X - no Modbus connection\n", reg);
    return 0.0f;
  }
  
  uint16_t trans = mb.readHreg(remote, reg, value, numregs, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
  
  // Add timeout protection for transaction wait loop
  unsigned long transactionStart = millis();
  
  while (mb.isTransaction(trans)) {  // Check if transaction is active
    if (millis() - transactionStart > MODBUS_TRANSACTION_TIMEOUT_MS) {
      Serial.printf("Error: Modbus transaction timeout for register 0x%X\n", reg);
      modbus_connection_stable = false;  // Mark connection as unstable
      return 0.0f;  // Return safe default on timeout
    }
    mb.task();
    delay(10);
  }
  
  modbus_query_last = millis();  // Update timestamp for rate limiting
  
  float float_reconstructed = f_2uint_float(value[0], value[1]);
  return float_reconstructed;
}

/**
 * @brief Read a uint16 value from Modbus register.
 * 
 * Reads a single holding register as an unsigned 16-bit integer.
 * Implements rate limiting, connection validation, and timeout protection.
 * 
 * @param reg Modbus register address to read from
 * @return uint16_t The value read from the register, or 0 on error
 */
uint16_t get_uint16(uint16_t reg) {  // get the int16 from the Modbus register
  uint16_t res = 0;  // Initialize to prevent use of uninitialized memory

  // Rate limiting - only applies if connected
  if (mb.isConnected(remote)) {
    while (millis() - modbus_query_last < MODBUS_QUERY_DELAY) {
      mb.task();
      delay(10);
    }
  }

  // Ensure we have a stable connection
  if (!establishModbusConnection()) {
    Serial.printf("Error: Cannot read register 0x%X - no Modbus connection\n", reg);
    return 0;
  }

  uint16_t trans = mb.readHreg(remote, reg, &res, 1, cb, KOSTAL_MODBUS_SLAVE_ID);  // Initiate Read Hreg from Modbus Server
  
  // Add timeout protection for transaction wait loop
  unsigned long transactionStart = millis();
  
  while (mb.isTransaction(trans)) {  // Check if transaction is active
    if (millis() - transactionStart > MODBUS_TRANSACTION_TIMEOUT_MS) {
      Serial.printf("Error: Modbus transaction timeout for register 0x%X\n", reg);
      modbus_connection_stable = false;  // Mark connection as unstable
      return 0;  // Return safe default on timeout
    }
    mb.task();
    delay(10);
  }

  modbus_query_last = millis();  // Update timestamp for rate limiting

  return res;
}

/**
 * @brief Format power value as human-readable string.
 * 
 * Formats power values with appropriate units:
 * - Values < 1W: "  0 W"
 * - Values < 1000W: "XXX W"
 * - Values >= 1000W: "X.X kW"
 * 
 * @param value Power value in watts (negative values are converted to positive)
 * @return String Formatted power string
 */
String getPowerString(float value) {
  char buffer[50];

  if (value < 0) {
    value *= -1;  //remove the minus sign
  }
  if (value < 1) {
    snprintf(buffer, sizeof(buffer), "  0 W");
  } else if (value < 1000) {
    snprintf(buffer, sizeof(buffer), "%3d W", (int)value);
  } else {
    snprintf(buffer, sizeof(buffer), "%2.1f kW", value / 1000);
  }
  return String(buffer);
}

/**
 * @brief Draw battery icon with state of charge indicator.
 * 
 * Displays a battery icon on the e-paper display with a fill level
 * corresponding to the battery state of charge (0-100%).
 * 
 * @param percent Battery state of charge (0-100%), values are clamped to this range
 * @param y Vertical position on display
 */
void drawBattery(uint16_t percent, uint16_t y) {
  char buffer[50];
  // Clamp percent to valid range (0-100)
  if (percent > 100) {
    percent = 100;
  }
  
  // Calculate battery level (0-5 range for display)
  uint8_t batteryLevel = (percent + 5) / 20;
  snprintf(buffer, sizeof(buffer), "%d", batteryLevel);
  
  u8g2_for_adafruit_gfx.setFontMode(0);
  u8g2_for_adafruit_gfx.setForegroundColor(0);
  u8g2_for_adafruit_gfx.setBackgroundColor(1);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_battery19_tn);
  u8g2_for_adafruit_gfx.drawStr(12, y, buffer);
}

/**
 * @brief Draw smiley face based on primary energy source.
 * 
 * Displays different emoticons based on where the house is getting most of its power:
 * - Grid (highest consumption): Sad face
 * - PV (highest consumption): Cool sunglasses face
 * - Battery (highest consumption): Happy face
 * 
 * @param own_consumption_grid Power consumption from grid in watts
 * @param own_consumption_pv Power consumption from PV in watts
 * @param own_consumption_batt Power consumption from battery in watts
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

  u8g2_for_adafruit_gfx.setFontMode(0);
  u8g2_for_adafruit_gfx.setForegroundColor(0);
  u8g2_for_adafruit_gfx.setBackgroundColor(1);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_emoticons21_tr);
  u8g2_for_adafruit_gfx.drawGlyph(display.width() / 2 - 11, 75, smiley);
}

/**
 * @brief Query Modbus and display power consumption data.
 * 
 * Reads all power consumption data from the Kostal inverter via Modbus TCP
 * and displays it on the e-paper screen with icons, values, and flow arrows.
 * Shows PV production, battery status, grid consumption, and house consumption.
 */
void writeOwnConsumption() {

  // get data from modbus
  uint16_t battery_soc           = get_uint16(KOSTAL_MODBUS_REG_BATTERY_SOC);
  float    own_consumption_grid  = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_GRID);
  float    own_consumption_pv    = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_PV);
  float    own_consumption_batt  = get_float(KOSTAL_MODBUS_REG_OWN_CONSUMPTION_BATTERY);
  float    power_dc1             = get_float(KOSTAL_MODBUS_REG_POWER_DC1);
  float    power_dc2             = get_float(KOSTAL_MODBUS_REG_POWER_DC2);
  float    home_consumption_rate = get_float(KOSTAL_MODBUS_REG_TOTAL_HOME_CONSUMTION_RATE);
  Serial.printf("Eigenverbrauch: %.1f %%\n", home_consumption_rate);
  float home_consumption = get_float(KOSTAL_MODBUS_REG_TOTAL_HOME_CONSUMPTION);
  Serial.printf("Hausverbrauch: %.1f Wh\n", home_consumption);

  float   power_wallbox = 0;  // TODO
  int16_t power_battery = (int16_t)get_uint16(KOSTAL_MODBUS_REG_BATTERY_POWER_CHARGE);
  float   power_house   = own_consumption_grid + own_consumption_pv + own_consumption_batt;
  float   power_pv      = power_dc1 + power_dc2;
  float   power_grid    = power_pv + power_battery + power_wallbox - power_house;

  display.setFont(&FreeSans9pt7b);
  displayText("Kostal Monitor", 18, GxEPD_ALIGN_CENTER);

  // draw icons
  u8g2_for_adafruit_gfx.setFontMode(0);
  u8g2_for_adafruit_gfx.setForegroundColor(0);
  u8g2_for_adafruit_gfx.setBackgroundColor(1);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_streamline_ecology_t);

  u8g2_for_adafruit_gfx.drawGlyph(4, 50, 0x003E); /* hex 3E solar panel */
  drawBattery(battery_soc, 115);

  u8g2_for_adafruit_gfx.setFont(u8g2_font_streamline_interface_essential_wifi_t);
  u8g2_for_adafruit_gfx.drawGlyph(display.width() - 23, 50, 0x0031); /* hex 30 Grid */

  u8g2_for_adafruit_gfx.setFont(u8g2_font_streamline_ecology_t);
  u8g2_for_adafruit_gfx.drawGlyph(display.width() - 23, 87, 0x0034); /* hex 34 e-car */

  u8g2_for_adafruit_gfx.setFont(u8g2_font_streamline_interface_essential_home_menu_t);
  u8g2_for_adafruit_gfx.drawGlyph(display.width() - 23, 122, 0x0030); /* hex 30 house */

  int16_t offset = display.width() / 2 + 38;
  displayText(getPowerString(power_pv).c_str(), 50, GxEPD_ALIGN_RIGHT, offset);        // PV power production
  displayText(getPowerString(power_battery).c_str(), 102, GxEPD_ALIGN_RIGHT, offset);  // battery power production

  char buffer_soc[10];
  snprintf(buffer_soc, sizeof(buffer_soc), "%3d %%", battery_soc);
  displayText(buffer_soc, 120, GxEPD_ALIGN_RIGHT, offset);  // battery SoC

  displayText(getPowerString(power_grid).c_str(), 50, GxEPD_ALIGN_RIGHT, 28);     // grid consumption grid
  displayText(getPowerString(power_wallbox).c_str(), 85, GxEPD_ALIGN_RIGHT, 28);  // wallbox consumption grid
  displayText(getPowerString(power_house).c_str(), 120, GxEPD_ALIGN_RIGHT, 28);

  /*
  char buffer_hcr[10];
  snprintf(buffer_hcr, sizeof(buffer_hcr), "%3d %%", (int)home_consumption_rate);
  displayText(buffer_hcr, 120, GxEPD_ALIGN_CENTER);
  */

  int8_t arrow_offset_left  = 38;
  int8_t arrow_offset_right = 21;
  int8_t display_center     = display.width() / 2;

  // draw centered inverter with smiley in center
  drawSmiley(own_consumption_grid, own_consumption_pv, own_consumption_batt);
  display.drawRect(display_center - 22, 40, 43, 65, GxEPD_BLACK);
  display.drawLine(display_center - 17, 40, display_center - 15, 105, GxEPD_BLACK);
  display.drawLine(display_center + 16, 40, display_center + 14, 105, GxEPD_BLACK);

  // draw arrows
  u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_t_86);
  if (power_pv > 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center - arrow_offset_left, 55, 0x2b0a); /* ↘ */
  } else if (power_pv < 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center - arrow_offset_left, 55, 0x2b09); /* ↖ */
  }

  if (power_battery > 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center - arrow_offset_left, 105, 0x2b08); /* ↗ */
  } else if (power_battery < 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center - arrow_offset_left, 105, 0x2b0b); /* ↙ */
  }

  if (power_grid < 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center + arrow_offset_right, 55, 0x2b0b); /* ↙ */
  } else if (power_grid > 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center + arrow_offset_right, 55, 0x2b08); /* ↗ */
  }

  if (power_wallbox > 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center + arrow_offset_right, 85, 0x2b6c); /* → */
  } else if (power_wallbox < 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center + arrow_offset_right, 85, 0x2b69); /* ← */
  }

  if (power_house > 0) {
    u8g2_for_adafruit_gfx.drawGlyph(display_center + arrow_offset_right, 105, 0x2b0a); /* ↘ */
  }

  display.setFont(&FreeMono9pt7b);
  displayText(getCurrentTime().c_str(), 18, GxEPD_ALIGN_RIGHT);

  display.update();
}

void showSetupScreen() {
  Serial.println("setup device");

  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeSans9pt7b);
  displayText("Kostal Monitor", 18, GxEPD_ALIGN_LEFT);
  displayText("*** Setup ***", 50, GxEPD_ALIGN_CENTER);
  displayText("Connect to WiFi & add data:", 80, GxEPD_ALIGN_CENTER);
  displayText(hostname.c_str(), 110, GxEPD_ALIGN_CENTER);
  display.update();
}

void showWiFiConnectionFailedScreen() {
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeSans9pt7b);
  displayText("Kostal Monitor", 18, GxEPD_ALIGN_LEFT);
  displayText("*** Error ***", 60, GxEPD_ALIGN_CENTER);
  displayText("WiFi connection failed", 90, GxEPD_ALIGN_LEFT);
  display.update();
}

void showModbusConnectionFailedScreen() {
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeSans9pt7b);
  displayText("Kostal Monitor", 18, GxEPD_ALIGN_LEFT);
  displayText("*** Error ***", 60, GxEPD_ALIGN_CENTER);
  displayText("Modbus connection failed", 90, GxEPD_ALIGN_LEFT);
  displayText("Check hostname/IP", 110, GxEPD_ALIGN_LEFT);
  display.update();
}

void showWiFiConnectedScreen() {

  WiFi.waitForConnectResult();

  IPAddress wIP = WiFi.localIP();
  Serial.printf("WiFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

  Serial.printf("Resolving hostname: %s\n", kostal_hostname.c_str());
  
  // Validate hostname before attempting resolution
  if (kostal_hostname.length() == 0) {
    Serial.println("Error: Kostal hostname is empty");
    showModbusConnectionFailedScreen();
    return;
  }
  
  WiFi.hostByName(kostal_hostname.c_str(), remote);

  if (remote != INADDR_NONE) {
    Serial.printf("Resolved to IP: %u.%u.%u.%u\n", remote[0], remote[1], remote[2], remote[3]);
    Serial.println("Modbus connection will be established on first read");
  } else {
    Serial.printf("Could not resolve hostname: %s\n", kostal_hostname.c_str());
    Serial.println("Please check network connection and hostname configuration");
    showModbusConnectionFailedScreen();
  }
}

/**
 * @brief Print the reason the ESP32 woke up from deep sleep.
 * 
 * Diagnostic function to help debug wake-up issues and power management.
 * Logs the wake-up cause to serial console and resets counter on first boot.
 */
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    wakeUpCounter = 0;
    break;
  }
}

/**
 * @brief Setup function - initializes hardware and connects to services.
 * 
 * Performs system initialization including:
 * - Serial communication setup
 * - Display initialization
 * - WiFi connection and configuration
 * - NTP time synchronization
 * - Modbus TCP connection
 * - Data query and display update
 * - Deep sleep configuration
 */
void setup() {
  Serial.begin(SERIAL_SPEED);
  
  // Wait for serial with timeout to prevent hanging on battery-powered operation
  unsigned long serialStart = millis();
  const unsigned long SERIAL_TIMEOUT_MS = 3000;  // 3 second timeout
  while (!Serial && (millis() - serialStart < SERIAL_TIMEOUT_MS)) {
    delay(10);
  }

  Serial.println(F(".-----------------------------------------------."));
  Serial.println(F("|  Kostal Plenticore DC/DC converter Monitor     |"));
  Serial.println(F("|  https://stritti.github.io/kostal-pv-monitor/  |"));
  Serial.println(F(" ------------------------------------------------"));
  Serial.println("  wake up counter " + String(++wakeUpCounter));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  SPIFFS.begin(true);  // Will format on the first run after failing to mount

  // Set up the display
  display.init(SERIAL_SPEED);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);
  u8g2_for_adafruit_gfx.begin(display);

  if (ESP_SLEEP_WAKEUP_TIMER != esp_sleep_get_wakeup_cause()) {
    display.fillScreen(GxEPD_WHITE);
  }
  WiFiSettings.hostname  = hostname.c_str();
  WiFiSettings.onPortal  = []() { showSetupScreen(); };
  WiFiSettings.onSuccess = []() { showWiFiConnectedScreen(); };
  WiFiSettings.onFailure = []() { showWiFiConnectionFailedScreen(); };

  // Define custom settings saved by WifiSettings
  // These will return the default if nothing was set before
  kostal_hostname    = WiFiSettings.string("kostal_hostname", "hostname", "KOSTAL Hostname");
  kostal_modbus_port = WiFiSettings.integer("kostal_modbus_port", 1, 65535, 1502, "KOSTAL Modbus Port");

  // Connect to WiFi with a timeout of 30 seconds
  // Launches the portal if the connection failed
  WiFiSettings.connect(true, 30);

  // Initialize a NTPClient to get time
  timeClient.begin();

  // Set up ModbusTCP connection
  mb.client();

  // TODO: Show Warning if level X is reached
  Serial.print(F("Battery Temp: "));
  Serial.print(get_float(214));
  Serial.println(F("\u00B0C"));

  int hour = getHourOfDay();
  Serial.printf("Current hour: %d\n", hour);
  if( hour >= DISPLAY_UPDATE_START_HOUR && hour <= DISPLAY_UPDATE_END_HOUR ) {
    writeOwnConsumption();
  } else {
    display.fillScreen(GxEPD_WHITE);
    displayText("sleeping until 7.00 ...", 60, GxEPD_ALIGN_CENTER);
    display.update();
  }


  // Disconnect Modbus with timeout protection
  Serial.println("Disconnecting Modbus...");
  mb.disconnect(remote);            // Close connection and
  mb.dropTransactions();            // Cancel all waiting transactions
  
  unsigned long disconnectStart = millis();
  while (mb.isConnected(remote)) {
    if (millis() - disconnectStart >= MODBUS_DISCONNECT_TIMEOUT_MS) {
      break;  // Timeout
    }
    mb.task();
    yield();
    delay(10);
  }
  
  if (mb.isConnected(remote)) {
    Serial.println("Warning: Modbus disconnect timeout - forcing cleanup");
  } else {
    Serial.println("Modbus disconnected successfully");
  }

  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off

  Serial.printf("Going to sleep now for %d sec.\n", (DATA_UPDATE_DELAY_SECONDS));
  esp_sleep_enable_timer_wakeup(DATA_UPDATE_DELAY_SECONDS * uS_TO_S_FACTOR);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, LOW);
  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  pinMode(LED_BUILTIN, OUTPUT);

  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

/**
 * @brief Main loop - not used as device operates in deep sleep mode.
 * 
 * This function is required by Arduino but never executes because the device
 * enters deep sleep at the end of setup() and wakes up to restart setup().
 */
void loop() {}
