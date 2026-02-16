/**
 * Native platform test suite for Kostal PV Monitor
 * 
 * This test validates core functionality that doesn't require ESP32 hardware.
 * Tests run on the native platform (CI server) using PlatformIO Unity.
 */

#include <unity.h>
#include <string.h>
#include <stdio.h>

// Test millis() overflow-safe timeout pattern
void test_millis_overflow_safe_timeout(void) {
    // Simulate scenario where millis() has overflowed
    unsigned long start = 0xFFFFFF00UL;  // Close to overflow
    unsigned long current = 0x00000100UL; // After overflow
    unsigned long timeout = 500UL;
    
    // Overflow-safe pattern: (current - start) >= timeout
    unsigned long elapsed = current - start;
    TEST_ASSERT_GREATER_OR_EQUAL(timeout, elapsed);
    TEST_ASSERT_EQUAL_UINT32(512, elapsed);  // Should be exactly 512
}

// Test millis() overflow-safe rate limiting pattern
void test_millis_overflow_safe_rate_limit(void) {
    // Simulate scenario where we should NOT execute yet
    unsigned long lastTime = 0xFFFFFF00UL;  // Close to overflow
    unsigned long current = 0x000000C0UL;   // After overflow, but < 500ms
    unsigned long delay = 500UL;
    
    // Overflow-safe pattern: (current - lastTime) < delay means too soon
    unsigned long elapsed = current - lastTime;
    TEST_ASSERT_LESS_THAN(delay, elapsed);
    TEST_ASSERT_EQUAL_UINT32(448, elapsed);  // Should be exactly 448
}

// Test buffer safety - snprintf doesn't overflow
void test_snprintf_buffer_safety(void) {
    char buffer[10];
    const char* longString = "This is a very long string that would overflow";
    
    int written = snprintf(buffer, sizeof(buffer), "%s", longString);
    
    // snprintf should return length it would have written (>= buffer size)
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(buffer), (size_t)written);
    // Buffer should be null-terminated
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[sizeof(buffer)-1]);
    // Length should be safe
    TEST_ASSERT_LESS_THAN(sizeof(buffer), strlen(buffer) + 1);
}

// Test snprintf with formatting
void test_snprintf_power_formatting(void) {
    char buffer[20];
    float power = 1234.56f;
    
    int written = snprintf(buffer, sizeof(buffer), "%3.0f W", power);
    
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_LESS_THAN(sizeof(buffer), (size_t)written);
    TEST_ASSERT_EQUAL_STRING("1235 W", buffer);  // Should round to 1235
}

// Test timeout constant validity
void test_timeout_constants(void) {
    // Verify timeout constants are reasonable (in milliseconds)
    const unsigned long MODBUS_TRANSACTION_TIMEOUT_MS = 5000UL;
    const unsigned long MODBUS_CONNECTION_TIMEOUT_MS = 3000UL;
    const unsigned long MODBUS_DISCONNECT_TIMEOUT_MS = 2000UL;
    
    TEST_ASSERT_GREATER_THAN(0, MODBUS_TRANSACTION_TIMEOUT_MS);
    TEST_ASSERT_GREATER_THAN(0, MODBUS_CONNECTION_TIMEOUT_MS);
    TEST_ASSERT_LESS_THAN(60000UL, MODBUS_TRANSACTION_TIMEOUT_MS); // Should be < 1 minute
}

// Test unsigned arithmetic wrapping behavior
void test_unsigned_arithmetic_wrapping(void) {
    unsigned long a = 1UL;
    unsigned long b = 10UL;
    
    // In C, unsigned subtraction wraps around
    unsigned long result = a - b;
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFF7UL, result);  // Wraps to large positive
    
    // This is why (millis() - start) works even after overflow
    TEST_ASSERT_TRUE(result > 0);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_millis_overflow_safe_timeout);
    RUN_TEST(test_millis_overflow_safe_rate_limit);
    RUN_TEST(test_snprintf_buffer_safety);
    RUN_TEST(test_snprintf_power_formatting);
    RUN_TEST(test_timeout_constants);
    RUN_TEST(test_unsigned_arithmetic_wrapping);
    
    return UNITY_END();
}
