/*
 * BB2-b : Flooding Coordinates (short project)
 *
 * Leader (ID 1) = (0,0,0). In the leader's frame:
 *   NORTH = (+1,0,0)  SOUTH = (-1,0,0)
 *   EAST  = (0,+1,0)  WEST  = (0,-1,0)
 *   TOP   = (0,0,+1)  BOTTOM = (0,0,-1)
 *
 * PROBLEM: a block can be rotated (multiples of 90 deg around the vertical axis) relative
 * to the leader, so its local NORTH is not the leader's NORTH.
 * SOLUTION: every block keeps a rotation `rot` (0..3) = its rotation relative to the leader frame.
 *   - a message carries: coordinates of the receiver, the GLOBAL direction g used by the
 *     sender to reach it, and the sender's rotation;
 *   - the receiver knows the local port q the message came in on. Physically, q faces the
 *     opposite of g, hence:  rot = (idx(g) + 2 - idx(q)) mod 4   (idx: N=0,E=1,S=2,W=3)
 *   - vertical links (TOP/BOTTOM) carry no horizontal information: rotation is inherited
 *     (assumes blocks are stacked aligned; LIMITATION to mention in the presentation).
 *   - to forward on local port p, the block converts it to the global direction with its rot.
 *
 * Each block shows its coordinates in the console (needs the spanning tree from the USB block)
 * and takes a color depending on x+y+z. The FIRST coordinates received are kept.
 */
#include <stdio.h>
#include <string.h>
#include <user_code.h>
#include <layer3_generic.h>
#include <speaker.h>
#include <abstraction.h>

enum direction { NORTH, BOTTOM, WEST, EAST, SOUTH, TOP };   // = port order

#define COORD_MSG 1
#define LEADER_ID 1

// Offsets indexed by (global) direction id: NORTH, BOTTOM, WEST, EAST, SOUTH, TOP
static const int8_t offX[6] = {  1,  0,  0, 0, -1, 0 };
static const int8_t offY[6] = {  0,  0, -1, 1,  0, 0 };
static const int8_t offZ[6] = {  0, -1,  0, 0,  0, 1 };
// Horizontal index on the clockwise cycle N,E,S,W (-1 for TOP/BOTTOM)
static const int8_t  hIndex[6] = { 0, -1, 3, 1, 2, -1 };
static const uint8_t hDir[4]   = { NORTH, EAST, SOUTH, WEST };

int8_t  px = 0, py = 0, pz = 0;
uint8_t rot = 0;
uint8_t hasCoord = 0;
uint8_t reported = 0;
uint32_t timeTreatment = 0;

// local direction -> global direction (leader frame)
static uint8_t toGlobal(uint8_t d) {
    if (hIndex[d] < 0) return d;
    return hDir[(hIndex[d] + rot) % 4];
}

// Send the coordinate of each neighbor (computed from mine) on every port except 'except'
static void propagate(uint8_t except) {
    for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
        if (p != except && is_connected(p)) {
            uint8_t g = toGlobal(p);
            uint8_t data[6];
            data[0] = COORD_MSG;
            data[1] = (uint8_t)(px + offX[g]);
            data[2] = (uint8_t)(py + offY[g]);
            data[3] = (uint8_t)(pz + offZ[g]);
            data[4] = g;
            data[5] = rot;
            sendMessage(p, data, 6, 1);
        }
    }
}

static void showColor() {
    int s = (px + py + pz) % 16;
    if (s < 0) s += 16;
    setColor(getId() == LEADER_ID ? RED : (uint8_t)s);
}

void BBinit() {
    timeTreatment = HAL_GetTick() + 2000;     // leave time for the spanning tree/debug route
    setColor(WHITE);
    if (getId() == LEADER_ID) {
        hasCoord = 1;
        px = py = pz = 0;
        rot = 0;
        showColor();
        propagate(255);
    }
}

void BBloop() {
    uint32_t time = HAL_GetTick();
    if (hasCoord && !reported && time > timeTreatment) {
        reported = 1;
        char msg[32];
        sprintf(msg, "ID:%d (%d,%d,%d) rot:%d", (int)getId(), px, py, pz, rot);
        send_debug_message((uint8_t*)msg, strlen(msg));
    }
}

static uint8_t onCoord(L3_packet *packet) {
    if (hasCoord) return 1;                       // first coordinates win
    uint8_t q = packet->io_port;                  // local direction the message came from
    uint8_t g = packet->packet_content[4];        // global direction used by the sender
    uint8_t senderRot = packet->packet_content[5];

    px = (int8_t)packet->packet_content[1];
    py = (int8_t)packet->packet_content[2];
    pz = (int8_t)packet->packet_content[3];

    if (hIndex[q] < 0 || hIndex[g] < 0) rot = senderRot;            // vertical link: inherit
    else rot = (uint8_t)((hIndex[g] + 2 + 4 - hIndex[q]) % 4);      // horizontal link

    hasCoord = 1;
    showColor();
    propagate(q);
    return 1;
}

uint8_t process_standard_packet(L3_packet *packet) {
    switch (packet->packet_content[0]) {
        case COORD_MSG: return onCoord(packet);
    }
    return 0;
}
