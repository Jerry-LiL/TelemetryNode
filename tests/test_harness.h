#ifndef TELEMETRY_TEST_HARNESS_H
#define TELEMETRY_TEST_HARNESS_H

#include <stdio.h>

static int g_test_fails;

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_test_fails++; \
        } \
    } while (0)

#endif
