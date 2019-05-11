#include "UCT.h"
#include "UCTNode.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#ifdef DEBUG
#define debug(...) printf(##__VAARS__)
#else
#define debug(...)
#endif

int UCT::M;
int UCT::N;
int UCT::noX;
int UCT::noY;
int UCT::currentTop[MAX_N];
int * * UCT::currentBoard;

UCT::UCT(int M, int N, int noX, int noY) {
    UCT::M = M;
    UCT::N = N;
    UCT::noX = noX;
    UCT::noY = noY;
}

// function UCTSEARCH(S_0)
void UCT::Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY) {
    UCT::currentBoard = new int *[M];
    for (int i = 0;i < M;i++) {
        UCT::currentBoard[i] = new int[N];
    }
    memcpy(currentTop, origTop, N * sizeof(int));
    
    //以状态s_0创建根节点v_0;
    UCTNode *root = new UCTNode(-1, -1, PLAYER_OTHER, nullptr);

    struct timeval begin, now;
    gettimeofday(&begin, NULL);
    long us = begin.tv_sec * 1000000 + begin.tv_usec;
    long cur_us = us;
    //while 尚未用完计算时长 do:
    while (cur_us < us + 2700000) {
        for (int j = 0;j < M;j++) {
            for (int k = 0;k < N;k++) {
                currentBoard[j][k] = origBoard[j][k];
            }
        }
        memcpy(currentTop, origTop, N * sizeof(int));

        //v_l←TREEPOLICY(v_0);
        UCTNode *v1 = treePolicy(root);
        //∆←DEFAULTPOLICY(s(v_l));
        float delta = defaultPolicy(v1);
        //BACKUP(v_l,∆);
        v1->backup(delta);

        gettimeofday(&now, NULL);
        cur_us = now.tv_sec * 1000000 + now.tv_usec;
    }
    //end while
    //return a(BESTCHILD(v_0,0));

    printf("root win rate: %f/%d\n", root->Q, root->N);
    UCTNode *best = root->bestChild(0.0);
    printf("best win rate: %f/%d\n", best->Q, best->N);

    placeX = best->x;
    placeY = best->y;

    for (int i = 0;i < M;i++) {
        delete [] UCT::currentBoard[i];
    }
    delete [] UCT::currentBoard;
}

// function TreePolicy(v)
UCTNode *UCT::treePolicy(UCTNode *node) {
    // while 节点v不是终止节点 do:
    while (node->checkEnd() != 0) {
        //if 节点v是可扩展的 then:
        if (node->expandNum > 0) {
            //return EXPAND(v)
            return node->expandOne();
        } else {
            //else:
            //v← BESTCHILD(v,c)
            node = node->bestChild(2.33);
        }
    }
    //return v
    return node;
}

// function DEFAULTPOLICY(s)
//while s不是终止状态 do:
//以等概率选择行动a∈A(s)
//s←f(s,a)
//return 状态s的收益
float UCT::defaultPolicy(UCTNode *node) {
    int currentPlayer = node->player;
    debug("begin emulation:\n");
    for (int i = 0;i < M;i++) {
        for (int j = 0;j < N;j++) {
            debug("%d ", currentBoard[i][j]);
        }
        debug("\n");
    }
    while (1) {
        int y = rand() % N;
        while (currentTop[y] == 0) {
            y = rand() % N;
        }
        int x = --currentTop[y];

        if (x - 1 == noX && y == noY) {
            currentTop[y]--;
        }
        currentPlayer = 3 - currentPlayer;

        currentBoard[x][y] = currentPlayer;

        debug("one step:\n");
        for (int i = 0;i < M;i++) {
            for (int j = 0;j < N;j++) {
                debug("%d ", currentBoard[i][j]);
            }
            debug("\n");
        }

        if (currentPlayer == PLAYER_ME && machineWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
            return node->player == PLAYER_ME ? 1.0 : 0.0;
        } else if (currentPlayer == PLAYER_OTHER && userWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
            return node->player == PLAYER_OTHER ? 1.0 : 0.0;
        } else if (isTie(UCT::N, currentTop)) {
            return 0.5;
        }
    }
    return 0.0;
}