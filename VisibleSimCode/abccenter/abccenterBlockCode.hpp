#ifndef abccenterBlockCode_H_
#define abccenterBlockCode_H_

#include "robots/blinkyBlocks/blinkyBlocksSimulator.h"
#include "robots/blinkyBlocks/blinkyBlocksBlockCode.h"
#include "robots/blinkyBlocks/blinkyBlocksBlock.h"

// Message identifiers (keep the values produced by the generator if different)
static const int BROADCAST_MSG_ID   = 1001; // pair<int,int> = (distance, phase)
static const int ACKNOWLEDGE_MSG_ID = 1002; // pair<int,int> = (max distance, id of the farthest module)
static const int SELECT_MSG_ID      = 1003; // pair<int,int> = (role, hops left)

static const int ROLE_B      = 1;
static const int ROLE_C      = 2;
static const int ROLE_CENTER = 3;

using namespace BlinkyBlocks;

class abccenterBlockCode : public BlinkyBlocksBlockCode {
private:
    BlinkyBlocksBlock *module = nullptr;

    bool isA = false;                        // from the XML (isA="true")
    int currentPhase = 0;                    // 1: BFS from A, 2: BFS from B
    int distance = 0;                        // hops to the source of the current phase
    int nbWaitedAnswers = 0;
    P2PNetworkInterface *parent = nullptr;   // towards the source of the phase
    P2PNetworkInterface *bestChild = nullptr;// towards the farthest module of my subtree
    int bestDist = 0;                        // distance of that farthest module
    int bestId = 0;                          // its id

    void beginPhase(int phase);              // this module becomes the source of a BFS
    void phaseFinished();                    // called on the source when the convergecast ends
    void routeSelect(int role, int hops);    // send a role down the path to the farthest module
    void applyRole(int role);

public:
    abccenterBlockCode(BlinkyBlocksBlock *host);
    ~abccenterBlockCode() {}

    void startup() override;

    void myBroadcastFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    void myAcknowledgeFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);
    void mySelectFunc(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);

    void parseUserBlockElements(TiXmlElement *config) override;
    string onInterfaceDraw() override;

    static BlockCode *buildNewBlockCode(BuildingBlock *host) {
        return (new abccenterBlockCode((BlinkyBlocksBlock *)host));
    }
};

#endif /* abccenterBlockCode_H_ */
