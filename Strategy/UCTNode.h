#ifndef UCTNODE_H
#define UCTNODE_H

#include "UCT.h"

class UCTNode {
    friend class UCT;
    int player; // 2=me,1=other
    int x, y;

    UCTNode *parent;
    UCTNode *children [MAX_N];

    int expandNum;
    int expandNodes[MAX_N]; // next step which column

    bool endNode; // whether it is an end node, i.e. no more steps are available
    int cachedResult; // skip defaultPolicy if already determined, -1 for invalid

    int Q; // 0 for lose, 1 for tie, 2 for win; divide by 2 to get the real score
    int N;
public:
    UCTNode(int x, int y, int player, UCTNode *parent);

    ~UCTNode();

    int checkEnd();

    void print(int depth, int tab = 0);

    // function Expand(v)
    UCTNode *expandOne();

    // function BestChild(v,c)
    UCTNode *bestChild(float coef);
    // for select the children of root node
    UCTNode *finalBestChild();

    // propagate cached result to parent node if possible
    void propagateCachedResult();

    // function Backup
    void backup(int delta);
};

#endif