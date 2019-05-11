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

    int endNode; // -1 means unknown, 0/1 means end/non-end

    float Q;
    int N;
public:
    UCTNode(int x, int y, int player, UCTNode *parent);

    ~UCTNode();

    int checkEnd();

    void print();

    // function Expand(v)
    UCTNode *expandOne();

    // function BestChild(v,c)
    UCTNode *bestChild(float coef);

    // function Backup
    void backup(float delta);
};

#endif