/*
 * BB1-b : send a debug message ("ID:x") to the computer console every 5 s
 * Before running: build the spanning tree from the USB block (blinkyApploaderCLI -t ...)
 * so messages can travel to the root block and then to the terminal.
 */
#include <stdio.h>
#include <string.h>
#include <user_code.h>
#include <layer3_generic.h>
#include <speaker.h>
#include <abstraction.h>

uint32_t time = 0, treatmentTime = 0;

void BBinit() {
    setColor(getId() % 17);
    treatmentTime = HAL_GetTick();
}

void BBloop() {
    time = HAL_GetTick();
    if (time > treatmentTime) {
        treatmentTime = time + 5000;
        char msg[20];
        sprintf(msg, "ID:%d", (int)getId());
        send_debug_message((uint8_t*)msg, strlen(msg));
    }
}

uint8_t process_standard_packet(L3_packet *packet) {
    return 0;
}
