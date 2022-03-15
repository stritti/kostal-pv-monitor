
const uint8_t KOSTAL_MODBUS_SLAVE_ID = 71;    // slave id of the Modbus TCP server
const int32_t MODBUS_QUERY_DELAY     = 5000;  // Show result every n'th millisecond (1500)

const uint16_t KOSTAL_MODBUS_REG_OWN_CONSUMPTION_BATTERY    = 0x6A;  // own consumption from battery
const uint16_t KOSTAL_MODBUS_REG_OWN_CONSUMPTION_GRID       = 0x6C;  // own consumption from grid
const uint16_t KOSTAL_MODBUS_REG_OWN_CONSUMPTION_PV         = 0x74;  // own consumption from PV
const uint16_t KOSTAL_MODBUS_REG_TOTAL_HOME_CONSUMPTION     = 0x76;  // float | Wh | total home consumption
const uint16_t KOSTAL_MODBUS_REG_TOTAL_HOME_CONSUMTION_RATE = 0x7C;  // float | % | Total home consumption rate

const uint16_t KOSTAL_MODBUS_REG_POWER_DC1 = 0x104;  // DC1 power
const uint16_t KOSTAL_MODBUS_REG_POWER_DC2 = 0x10E;  // DC2 power

const uint16_t KOSTAL_MODBUS_REG_BATTERY_SOC          = 0x202;  // U16 | % | battery state of charge
const uint16_t KOSTAL_MODBUS_REG_BATTERY_TEMP         = 0xD6;   // float | °C | battery temperature
const uint16_t KOSTAL_MODBUS_REG_BATTERY_POWER_CHARGE = 0x246;  // S16 | W | Actual battery charge/discharge power

const uint16_t KOSTAL_MODBUS_REG_TOTAL_DC_POWER = 0x42A;  // R32 | W | total DC power
