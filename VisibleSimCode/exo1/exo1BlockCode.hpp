#ifndef Exo1BlockCode_H_
#define Exo1BlockCode_H_

#include "robots/blinkyBlocks/blinkyBlocksSimulator.h"
#include "robots/blinkyBlocks/blinkyBlocksBlockCode.h"
#include "robots/blinkyBlocks/blinkyBlocksBlock.h"

// Message identifiers (the generator creates these: keep ITS values if different)
static const int BROADCAST_MSG_ID   = 1001; // pair<int,int> = (distance, round)
static const int ACKNOWLEDGE_MSG_ID = 1002; // no data

using namespace BlinkyBlocks;

class Exo1BlockCode : public BlinkyBlocksBlockCode {
private:
    BlinkyBlocksBlock *module = nullptr;

    bool isLeader = false;                  // set from the XML (leader="true") or by a tap
    int currentRound = 0;                   // number of the last flooding wave seen
    int distance = 0;                       // hops to the current leader
    int nbWaitedAnswers = 0;                // acknowledgments still expected
    P2PNetworkInterface *parent = nullptr;  // neighbor towards the leader (spanning tree)

    void startFlooding();                   // (re)start a wave from this module
    Color gradientColor(int d) const;       // color depending on the distance

public:
    Exo1BlockCode(BlinkyBlocksBlock *host);
    ~Exo1BlockCode() {}

    void startup() override;

    /** Handler of message `broadcast` : flooding wave from the leader */
    void myBroadcastFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    /** Handler of message `acknowledge` : convergecast towards the leader */
    void myAcknowledgeFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);

    void parseUserBlockElements(TiXmlElement *config) override;
    void onUserKeyPressed(unsigned char c, int x, int y) override;
    /** A tapped module becomes the new leader */
    void onTap(int face) override;

    static BlockCode *buildNewBlockCode(BuildingBlock *host) {
        return (new Exo1BlockCode((BlinkyBlocksBlock *)host));
    }
};

#endif /* myAppCode_H_ */
