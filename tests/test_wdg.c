#include "test_harness.h"
#include "wdg.h"

int main(void)
{
    uint32_t kicks[TN_TASK_COUNT];
    uint32_t stale;

    kicks[TN_TASK_ENV] = 1000;
    kicks[TN_TASK_IMU] = 1000;
    kicks[TN_TASK_TELEM] = 1000;
    kicks[TN_TASK_SUPERVISOR] = 0;

    stale = wdg_stale_mask(kicks, 1300, 400, WDG_SUPERVISED_MASK);
    EXPECT(stale == 0u);

    stale = wdg_stale_mask(kicks, 1500, 400, WDG_SUPERVISED_MASK);
    EXPECT(stale == ((1u << TN_TASK_ENV) | (1u << TN_TASK_IMU) | (1u << TN_TASK_TELEM)));

    kicks[TN_TASK_IMU] = 1400;
    stale = wdg_stale_mask(kicks, 1500, 400, WDG_SUPERVISED_MASK);
    EXPECT((stale & (1u << TN_TASK_IMU)) == 0u);
    EXPECT((stale & (1u << TN_TASK_ENV)) != 0u);
    EXPECT((stale & (1u << TN_TASK_SUPERVISOR)) == 0u);

    /* Wrap of the millisecond clock. */
    kicks[TN_TASK_ENV] = 0xFFFFFFF0u;
    kicks[TN_TASK_IMU] = 0xFFFFFFF0u;
    kicks[TN_TASK_TELEM] = 0xFFFFFFF0u;
    stale = wdg_stale_mask(kicks, 0x10u, 400, WDG_SUPERVISED_MASK);
    EXPECT(stale == 0u);

    EXPECT(wdg_stale_mask(NULL, 0, 10, WDG_SUPERVISED_MASK) == WDG_SUPERVISED_MASK);
    return g_test_fails == 0 ? 0 : 1;
}
