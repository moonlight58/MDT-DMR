/**
 * @file myAppCode.cpp
 * Exercise 1: color gradient flooded from a leader; tapping a block
 * makes it the new leader and restarts the gradient from it.
 *
 * Messages:
 *  - broadcast   (pair<int,int> = (distance, round)): flooding wave. A module
 *                accepts it if the round is newer than the one it knows, or
 *                the same round with a shorter distance. The sender becomes
 *                its parent, and the wave is forwarded to all other neighbors.
 *  - acknowledge (no data): sent to the parent once every message forwarded
 *                has been answered (convergecast). When it reaches the
 *                leader, the wave is finished.
 **/

#include "exo1BlockCode.hpp"
#include <cmath>

Exo1BlockCode::Exo1BlockCode(BlinkyBlocksBlock *host) : BlinkyBlocksBlockCode(host), module(host) {
    // @warning Do not remove block below, as a blockcode with a NULL host might be created
    //  for command line parsing
    if (not host) return;

    addMessageEventFunc2(BROADCAST_MSG_ID,
                         std::bind(&Exo1BlockCode::myBroadcastFunc, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(ACKNOWLEDGE_MSG_ID,
                         std::bind(&Exo1BlockCode::myAcknowledgeFunc, this,
                                   std::placeholders::_1, std::placeholders::_2));
}

// Simple hue ramp: neighbours at distance d and d+1 get different colors
Color Exo1BlockCode::gradientColor(int d) const {
    float h = std::fmod(d * 0.07f, 1.0f) * 6.0f;
    int i = (int)h;
    float f = h - i;
    float r = 0, g = 0, b = 0;
    switch (i % 6) {
        case 0: r = 1;     g = f;     b = 0;     break;
        case 1: r = 1 - f; g = 1;     b = 0;     break;
        case 2: r = 0;     g = 1;     b = f;     break;
        case 3: r = 0;     g = 1 - f; b = 1;     break;
        case 4: r = f;     g = 0;     b = 1;     break;
        default: r = 1;    g = 0;     b = 1 - f; break;
    }
    return Color(r, g, b);
}

// This module starts a new wave, with a round number newer than any it has seen
void Exo1BlockCode::startFlooding() {
    isLeader = true;
    module->setColor(RED);
    currentRound++;
    distance = 0;
    parent = nullptr;
    nbWaitedAnswers = sendMessageToAllNeighbors(
        "broadcast",
        new MessageOf<std::pair<int, int>>(BROADCAST_MSG_ID, std::make_pair(1, currentRound)),
        0, 0, 0);
    console << "leader " << getId() << " starts round " << currentRound << "\n";
}

void Exo1BlockCode::startup() {
    console << "start " << getId() << "\n";
    if (isLeader) {
        startFlooding();
    }
}

void Exo1BlockCode::myBroadcastFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    MessageOf<std::pair<int, int>> *msg = static_cast<MessageOf<std::pair<int, int>> *>(_msg.get());
    std::pair<int, int> data = *msg->getData();
    int dist = data.first;
    int round = data.second;

    bool newer = (round > currentRound) || (round == currentRound && dist < distance);
    if (isLeader && round == currentRound) newer = false; // never override my own wave

    if (newer) {
        if (round > currentRound && isLeader) isLeader = false; // a newer leader exists
        currentRound = round;
        distance = dist;
        parent = sender;
        module->setColor(gradientColor(distance));

        nbWaitedAnswers = sendMessageToAllNeighbors(
            "broadcast",
            new MessageOf<std::pair<int, int>>(BROADCAST_MSG_ID, std::make_pair(dist + 1, round)),
            0, 0, 1, sender);

        if (nbWaitedAnswers == 0) { // leaf of the spanning tree
            sendMessage("ack2parent", new Message(ACKNOWLEDGE_MSG_ID), parent, 0, 0);
        }
    } else {
        // useless wave: answer at once so that the sender can finish counting
        sendMessage("ack2sender", new Message(ACKNOWLEDGE_MSG_ID), sender, 0, 0);
    }
}

void Exo1BlockCode::myAcknowledgeFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    if (nbWaitedAnswers <= 0) return; // late ack from an older round: ignore
    nbWaitedAnswers--;
    if (nbWaitedAnswers == 0) {
        if (parent != nullptr) {
            sendMessage("ack2parent", new Message(ACKNOWLEDGE_MSG_ID), parent, 0, 0);
        } else if (isLeader) {
            console << "round " << currentRound << " finished (all modules reached)\n";
        }
    }
}

void Exo1BlockCode::parseUserBlockElements(TiXmlElement *config) {
    const char *attr = config->Attribute("leader");
    isLeader = (attr ? Simulator::extractBoolFromString(attr) : false);
    if (isLeader) {
        std::cout << getId() << " is leader!" << std::endl;
    }
}

void Exo1BlockCode::onUserKeyPressed(unsigned char c, int x, int y) {
    // not used in this exercise
}

// Run the app with "-s inf", then Ctrl + right click on a block -> Tap
void Exo1BlockCode::onTap(int face) {
    console << "tapped: " << getId() << " becomes the leader\n";
    startFlooding();
}
