#ifndef UCT_H
#define UCT_H

#include "Judge.h"
#include <stdlib.h>

const int PLAYER_ME = 2;
const int PLAYER_OTHER = 1;

class UCTNode;

class UCT {
public:
    static int M, N;
    static int noX, noY;
    static int currentTop[MAX_N];
    static int currentBoard[MAX_M][MAX_N];

    UCT(int M, int N, int noX, int noY);

    // function UCTSEARCH(S_0)
    void Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY);

    // function TreePolicy(v)
    UCTNode *treePolicy(UCTNode *node);

    // function DEFAULTPOLICY(s)
    int defaultPolicy(UCTNode *node);
};

#endif
