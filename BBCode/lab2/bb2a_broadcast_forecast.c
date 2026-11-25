/*
 * BB2-a : Broadcast followed by a Forecast (acknowledgement convergecast)
 *
 * Broadcast : leader (ID 1) floods DISTANCE_MSG; each block keeps the FIRST sender as parent,
 *             stores its distance, and forwards to all other neighbors.
 * Forecast  : every DISTANCE_MSG sent gets exactly ONE ACK_MSG back:
 *               - a block that was already reached answers immediately,
 *               - a block with children answers its parent once all children answered,
 *               - a leaf (nothing to forward) answers immediately.
 *             When the leader has received all its ACKs, EVERY block has the message.
 * Side effect: parent links form a spanning tree rooted at the leader.
 *
 * Colors: waiting = WHITE, reached = distance%16, leader RED then blinking GREEN when done.
 */
#include <stdio.h>
#include <string.h>
#include <user_code.h>
#include <layer3_generic.h>
#include <speaker.h>
#include <abstraction.h>

#define DISTANCE_MSG 1
#define ACK_MSG      2
#define NO_PARENT    255
#define LEADER_ID    1

uint8_t  visited = 0;
uint8_t  parent = NO_PARENT;
uint8_t  distance = 255;
uint8_t  nbWaitedAnswers = 0;
uint8_t  finished = 0;          // leader only: all acks received
uint8_t  reported = 0;
uint8_t  blinkState = 0;
uint32_t timeTreatment = 0;

static void sendAck(uint8_t port) {
    uint8_t data[2] = { ACK_MSG, 0 };
    sendMessage(port, data, 2, 1);
}

// Forward DISTANCE_MSG to every connected port except 'except'. Returns number of messages sent.
static uint8_t floodDistance(uint8_t except) {
    uint8_t n = 0;
    uint8_t data[2] = { DISTANCE_MSG, (uint8_t)(distance + 1) };
    for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
        if (p != except && is_connected(p)) {
            sendMessage(p, data, 2, 1);
            n++;
        }
    }
    return n;
}

void BBinit() {
    timeTreatment = HAL_GetTick();
    setColor(WHITE);
    if (getId() == LEADER_ID) {
        visited = 1;
        distance = 0;
        setColor(RED);
        nbWaitedAnswers = floodDistance(NO_PARENT);
        if (nbWaitedAnswers == 0) finished = 1;   // alone: nothing to wait for
    }
}

void BBloop() {
    uint32_t time = HAL_GetTick();
    if (time > timeTreatment) {
        timeTreatment = time + 500;
        if (finished) {                           // leader signals the end of the forecast
            blinkState = !blinkState;
            setColor(blinkState ? GREEN : BLACK);
            if (!reported) {
                reported = 1;
                char msg[20];
                sprintf(msg, "ALL ACKED");
                send_debug_message((uint8_t*)msg, strlen(msg));
            }
        }
    }
}

static uint8_t onDistance(L3_packet *packet) {
    if (!visited) {
        visited = 1;
        parent = packet->io_port;
        distance = packet->packet_content[1];
        setColor(distance % 16);
        nbWaitedAnswers = floodDistance(parent);
        if (nbWaitedAnswers == 0) sendAck(parent);       // leaf
    } else {
        sendAck(packet->io_port);                        // already reached: answer right away
    }
    return 1;
}

static uint8_t onAck(L3_packet *packet) {
    if (nbWaitedAnswers > 0) nbWaitedAnswers--;
    if (nbWaitedAnswers == 0) {
        if (getId() == LEADER_ID) finished = 1;
        else if (parent != NO_PARENT) sendAck(parent);
    }
    return 1;
}

uint8_t process_standard_packet(L3_packet *packet) {
    switch (packet->packet_content[0]) {
        case DISTANCE_MSG: return onDistance(packet);
        case ACK_MSG:      return onAck(packet);
    }
    return 0;
}
