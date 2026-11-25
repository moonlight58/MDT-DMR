/*
 * BB1-c : display the neighborhood as a color sequence, combined with a sound.
 * For each direction (EAST, NORTH, WEST, SOUTH, BOTTOM, TOP) the block shows
 * the direction's color + plays its note if a neighbor is connected there,
 * otherwise it stays black/silent. A white flash marks the end of the sequence.
 *
 * Note: no HAL_Delay() here -> messages can still be processed (non-blocking).
 */
#include <user_code.h>
#include <layer3_generic.h>
#include <speaker.h>
#include <abstraction.h>

// Port numbers follow this order (hardware constraint): use direction ids, not raw ports
enum direction { NORTH, BOTTOM, WEST, EAST, SOUTH, TOP };

uint8_t currentPort = 0;
uint32_t time = 0, treatmentTime = 0;

// Sequence of directions shown, with their color
static const uint8_t seqDir[6]   = { EAST, NORTH,  WEST,   SOUTH, BOTTOM, TOP };
static const uint8_t seqColor[6] = { RED,  ORANGE, YELLOW, GREEN, CYAN,   BLUE };

void BBinit() {
    setColor(WHITE);
    treatmentTime = HAL_GetTick() + 1000;
}

static void playNote(uint8_t step) {
    switch (step) {
        case 0: setHWBUZZER(DO,  200); break;
        case 1: setHWBUZZER(RE,  200); break;
        case 2: setHWBUZZER(MI,  200); break;
        case 3: setHWBUZZER(FA,  200); break;
        case 4: setHWBUZZER(SOL, 200); break;
        case 5: setHWBUZZER(LA,  200); break;
    }
}

void BBloop() {
    time = HAL_GetTick();
    if (time > treatmentTime) {
        treatmentTime = time + 400;
        if (currentPort < 6) {
            if (is_connected(seqDir[currentPort])) {
                setColor(seqColor[currentPort]);
                playNote(currentPort);
            } else {
                setColor(BLACK);
            }
            currentPort++;
        } else {
            setColor(WHITE);      // end of sequence marker
            currentPort = 0;
        }
    }
}

uint8_t process_standard_packet(L3_packet *packet) {
    return 0;
}
