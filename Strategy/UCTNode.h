#ifndef UCTNODE_H
#define UCTNODE_H

#include "UCT.h"
#include <cassert>
#include <memory>
#include <unordered_map>

class UCTNode {
    friend class UCT;
    UCTNode *parent; // parent node in the current search path
    UCTNode *children [MAX_N];
    float Q; // win rate
    float selfQ; // win rate for self visit in defaultPolicy

    int childVisit [MAX_N]; // visit count for each child
    int sumChildVisit;
    int player; // 2=me,1=other
    int selfN; // self visit time in defaultPolicy

    int8_t childX [MAX_N]; // x for each child
    int8_t expandNum;
    int8_t expandNodes[MAX_N]; // next step which column

    int8_t endNode; // whether it is an end node, i.e. no more steps are available
    int8_t cachedResult; // skip defaultPolicy if already determined, -1 for invalid
    int8_t steps; // maximum steps from the current player (including current one) to achieve the cached result
public:
    UCTNode() {}
    UCTNode(int player, UCTNode *parent);

    ~UCTNode();

    int checkEnd();

    void print(int depth, int tab = 0);

    // function Expand(v)
    UCTNode *expandOne();

    // function BestChild(v,c)
    UCTNode *bestChild();
    // for select the children of root node
    int finalBestChild();

    // function Backup
    void backup(int delta);
};

class BumpAllocator {
public:
    UCTNode data[900 * 1024 * 1024 / sizeof(struct UCTNode)];
    size_t index;

    BumpAllocator() {
        index = 0;
    }

    UCTNode *allocate() {
        assert(index < sizeof(data) / sizeof(data[0]));
        return &data[index++];
    }

    void clear() {
        index = 0;
    }
};

extern BumpAllocator allocator;

// hash map: (bitboardOther, bitboardMe) -> state
extern std::unordered_map<std::pair<BitBoard, BitBoard>, UCTNode *> nodeCache;

#endif