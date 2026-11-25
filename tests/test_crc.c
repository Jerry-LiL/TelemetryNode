#include "crc16.h"
#include "test_harness.h"

#include <string.h>

int main(void)
{
    const uint8_t digits[] = "123456789";
    uint8_t empty = 0;

    EXPECT(crc16_ccitt(digits, 9) == 0x29B1u);
    EXPECT(crc16_ccitt(&empty, 0) == 0xFFFFu);
    EXPECT(crc16_ccitt(NULL, 4) == 0xFFFFu);
    EXPECT(crc16_ccitt(digits, 1) != crc16_ccitt(digits, 2));
    return g_test_fails == 0 ? 0 : 1;
}
