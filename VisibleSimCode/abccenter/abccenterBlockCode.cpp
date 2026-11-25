/**
 * @file abccenterBlockCode.cpp
 * Exercise 2: ABC center. A (red) -> B (yellow, farthest from A)
 *             -> C (blue, farthest from B) -> center of BC (brown).
 *
 * Each phase is a BFS flooding + a convergecast that finds the farthest module:
 *  - broadcast   (distance, phase): flooding wave. First reception = parent.
 *                A module that already joined this phase answers with a "nack".
 *  - acknowledge (maxDist, id): convergecast. A module waits for one answer per
 *                message it forwarded, keeps the farthest module (ties -> highest id),
 *                remembers the child leading to it (bestChild) and answers its parent.
 *                A nack is (-1,-1) and never wins.
 *  - select      (role, hops): sent down the bestChild path; the module reached when
 *                hops == 0 takes the role (B, C or center).
 *
 * Assumption: messages have the same size and are sent with a constant delay, so
 * the first wave to arrive follows a shortest path (hop distance = BFS distance).
 **/
#include "abccenterBlockCode.hpp"

// Global variables displayed by onInterfaceDraw (as requested in the subject)
BlinkyBlocksBlock *moduleA = nullptr;
BlinkyBlocksBlock *moduleB = nullptr;
BlinkyBlocksBlock *moduleC = nullptr;
BlinkyBlocksBlock *moduleCenter = nullptr;
int distAB, distBC, distBcenter, distCcenter;

typedef std::pair<int, int> IntPair;

abccenterBlockCode::abccenterBlockCode(BlinkyBlocksBlock *host) : BlinkyBlocksBlockCode(host), module(host) {
    if (not host) return;

    addMessageEventFunc2(BROADCAST_MSG_ID,
                         std::bind(&abccenterBlockCode::myBroadcastFunc, this,
                                   std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(ACKNOWLEDGE_MSG_ID,
                         std::bind(&abccenterBlockCode::myAcknowledgeFunc, this,
                                   std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(SELECT_MSG_ID,
                         std::bind(&abccenterBlockCode::mySelectFunc, this,
                                   std::placeholders::_1, std::placeholders::_2));
}

void abccenterBlockCode::startup() {
    console << "start " << getId() << "\n";
    if (isA) {
        module->setColor(RED);
        moduleA = module;
        beginPhase(1);
    }
}

// This module is the source of a BFS (A for phase 1, B for phase 2)
void abccenterBlockCode::beginPhase(int phase) {
    currentPhase = phase;
    distance = 0;
    parent = nullptr;
    bestChild = nullptr;
    bestDist = 0;
    bestId = getId();
    nbWaitedAnswers = sendMessageToAllNeighbors(
        "broadcast", new MessageOf<IntPair>(BROADCAST_MSG_ID, std::make_pair(1, phase)), 0, 0, 0);
    if (nbWaitedAnswers == 0) phaseFinished(); // isolated module
}

void abccenterBlockCode::myBroadcastFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    MessageOf<IntPair> *msg = static_cast<MessageOf<IntPair> *>(_msg.get());
    IntPair data = *msg->getData();
    int dist = data.first, phase = data.second;

    if (phase > currentPhase) { // first wave of this phase: join the tree
        currentPhase = phase;
        distance = dist;
        parent = sender;
        bestChild = nullptr;
        bestDist = dist;      // the farthest module of my subtree is me for now
        bestId = getId();
        nbWaitedAnswers = sendMessageToAllNeighbors(
            "broadcast", new MessageOf<IntPair>(BROADCAST_MSG_ID, std::make_pair(dist + 1, phase)),
            0, 0, 1, sender);
        if (nbWaitedAnswers == 0) { // leaf
            sendMessage("ack2parent",
                        new MessageOf<IntPair>(ACKNOWLEDGE_MSG_ID, std::make_pair(bestDist, bestId)),
                        parent, 0, 0);
        }
    } else { // already in the tree: nack
        sendMessage("nack",
                    new MessageOf<IntPair>(ACKNOWLEDGE_MSG_ID, std::make_pair(-1, -1)),
                    sender, 0, 0);
    }
}

void abccenterBlockCode::myAcknowledgeFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    MessageOf<IntPair> *msg = static_cast<MessageOf<IntPair> *>(_msg.get());
    IntPair data = *msg->getData();

    if (data.first > bestDist || (data.first == bestDist && data.first >= 0 && data.second > bestId)) {
        bestDist = data.first;
        bestId = data.second;
        bestChild = sender;
    }
    nbWaitedAnswers--;
    if (nbWaitedAnswers == 0) {
        if (parent != nullptr) {
            sendMessage("ack2parent",
                        new MessageOf<IntPair>(ACKNOWLEDGE_MSG_ID, std::make_pair(bestDist, bestId)),
                        parent, 0, 0);
        } else {
            phaseFinished(); // I am the source: the farthest module is known
        }
    }
}

// Executed by the source (A then B) when its convergecast is finished
void abccenterBlockCode::phaseFinished() {
    if (currentPhase == 1) {
        distAB = bestDist;
        console << "A knows B=" << bestId << " at " << bestDist << " hops\n";
        routeSelect(ROLE_B, bestDist);
    } else if (currentPhase == 2) {
        distBC = bestDist;
        distBcenter = bestDist / 2;
        distCcenter = bestDist - distBcenter;
        console << "B knows C=" << bestId << " at " << bestDist << " hops\n";
        // both messages follow the same path B -> C
        routeSelect(ROLE_CENTER, distBcenter);
        routeSelect(ROLE_C, bestDist);
    }
}

// hops == 0 : I am the target. Otherwise forward along the path to the farthest module.
void abccenterBlockCode::routeSelect(int role, int hops) {
    if (hops == 0) {
        applyRole(role);
    } else if (bestChild != nullptr) {
        sendMessage("select",
                    new MessageOf<IntPair>(SELECT_MSG_ID, std::make_pair(role, hops - 1)),
                    bestChild, 0, 0);
    }
}

void abccenterBlockCode::mySelectFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender) {
    MessageOf<IntPair> *msg = static_cast<MessageOf<IntPair> *>(_msg.get());
    IntPair data = *msg->getData();
    routeSelect(data.first, data.second);
}

void abccenterBlockCode::applyRole(int role) {
    switch (role) {
        case ROLE_B:
            module->setColor(Color(1.0f, 1.0f, 0.0f)); // yellow
            moduleB = module;
            beginPhase(2); // B starts the second BFS
            break;
        case ROLE_C:
            module->setColor(BLUE);
            moduleC = module;
            break;
        case ROLE_CENTER:
            module->setColor(Color(0.55f, 0.27f, 0.07f)); // brown
            moduleCenter = module;
            break;
    }
}

void abccenterBlockCode::parseUserBlockElements(TiXmlElement *config) {
    const char *attr = config->Attribute("isA");
    isA = (attr ? Simulator::extractBoolFromString(attr) : false);
    if (isA) {
        std::cout << getId() << " is A!" << std::endl;
    }
}

string abccenterBlockCode::onInterfaceDraw() {
    string str = "Press 'r' to run the simulation.";
    if (moduleA != nullptr) {
        str = "A= (" + moduleA->position.to_string() + ")";
    }
    if (moduleB != nullptr) {
        str += "\nB= (" + moduleB->position.to_string() + "), distAB=" + to_string(distAB);
    }
    if (moduleC != nullptr) {
        str += "\nC= (" + moduleC->position.to_string() + "), distBC=" + to_string(distBC);
    }
    if (moduleCenter != nullptr) {
        str += "\nCenter= (" + moduleCenter->position.to_string() + "), dist to B=" +
               to_string(distBcenter) + ", dist to C=" + to_string(distCcenter);
    }
    return str;
}
