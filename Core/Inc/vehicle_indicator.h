#ifndef VEHICLE_INDICATOR_H
#define VEHICLE_INDICATOR_H
/* User authorized provisional active-low mapping; physical polarity unverified. */
#define VEHICLE_LED_CONFIGURED 1
#define VEHICLE_LED_ACTIVE_HIGH 0
#define VEHICLE_LED_TEST_LOW 0
#if VEHICLE_LED_ACTIVE_HIGH != 0 && VEHICLE_LED_ACTIVE_HIGH != 1
#error Invalid PE3 polarity
#endif
#endif
