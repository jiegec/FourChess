#ifndef UCT_H
#define UCT_H

#include "Judge.h"
#include <stdlib.h>

const int MAX_M = 12;
const int MAX_N = 12;

const int PLAYER_ME = 2;
const int PLAYER_OTHER = 1;

class UCTNode;

class UCT {
public:
    static int M, N;
    static int noX, noY;
    static int currentTop[MAX_N];
    static int **currentBoard;

    UCT(int M, int N, int noX, int noY);

    // function UCTSEARCH(S_0)
    void Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY);

    // function TreePolicy(v)
    UCTNode *treePolicy(UCTNode *node);

    // function DEFAULTPOLICY(s)
    float defaultPolicy(UCTNode *node);
};

#endif