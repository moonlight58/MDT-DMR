/*
 * BB1-a : blink at 2 Hz, alternating two colors (id%18) and (17-id%18)
 * Drop-in replacement for src/user_code.c
 */
#include <user_code.h>
#include <layer3_generic.h>
#include <speaker.h>
#include <abstraction.h>

uint32_t time = 0, treatmentTime = 0;
uint8_t phase = 0;

void BBinit() {
    setColor(getId() % 18);
    treatmentTime = HAL_GetTick();
}

void BBloop() {
    time = HAL_GetTick();
    // 2 Hz = 2 full cycles per second -> change state every 250 ms
    if (time > treatmentTime) {
        treatmentTime = time + 250;
        phase = !phase;
        setColor(phase ? getId() % 18 : 17 - getId() % 18);
    }
}

uint8_t process_standard_packet(L3_packet *packet) {
    return 0;
}
