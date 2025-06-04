#include "UCT.h"
#include "UCTNode.h"
#include <cstddef>
#include <cstring>
#include <memory.h>
#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#ifdef DEBUG
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...)
#endif

int UCT::M;
int UCT::N;
int UCT::noX;
int UCT::noY;
int UCT::currentTop[MAX_N];
BitBoard UCT::currentBitBoard[3];

UCT::UCT(int M, int N, int noX, int noY) {
    UCT::M = M;
    UCT::N = N;
    UCT::noX = noX;
    UCT::noY = noY;
}

// function UCTSEARCH(S_0)
void UCT::Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY) {
    fprintf(stderr, "-------\n");

    struct timeval begin, now;
    gettimeofday(&begin, NULL);
    long us = begin.tv_sec * 1000000 + begin.tv_usec;
    long cur_us = us;
    long searches = 0;

    int board[MAX_M][MAX_N];
    int moves = 0;
    for (int j = 0;j < M;j++) {
        for (int k = 0;k < N;k++) {
            board[j][k] = origBoard[j][k];
            if (board[j][k] != 0) {
                moves ++;
            }
        }
    }

    // give some time for memory reclaim
    int timeLimit = 2800000;

    BitBoard bitBoard[3];
    // construct bitboard for two players
    bitBoard[PLAYER_OTHER] = BitBoard(PLAYER_OTHER, board);
    bitBoard[PLAYER_ME] = BitBoard(PLAYER_ME, board);

    // new UCTNode might use these
    memcpy(currentTop, origTop, N * sizeof(int));
    currentBitBoard[PLAYER_OTHER] = bitBoard[PLAYER_OTHER];
    currentBitBoard[PLAYER_ME] = bitBoard[PLAYER_ME];
    assert(moves == currentBitBoard[PLAYER_OTHER].popcount() + currentBitBoard[PLAYER_ME].popcount());
    
    //以状态s_0创建根节点v_0;
    UCTNode *root = new UCTNode(PLAYER_OTHER, nullptr);

    //while 尚未用完计算时长 do:
    while (cur_us < us + timeLimit && root->cachedResult == -1) {
        memcpy(currentTop, origTop, N * sizeof(int));
        currentBitBoard[PLAYER_OTHER] = bitBoard[PLAYER_OTHER];
        currentBitBoard[PLAYER_ME] = bitBoard[PLAYER_ME];

        //v_l←TREEPOLICY(v_0);
        UCTNode *v1 = treePolicy(root);
        //∆←DEFAULTPOLICY(s(v_l));
        int delta = defaultPolicy(v1);
        //BACKUP(v_l,∆);
        v1->backup(delta);

        gettimeofday(&now, NULL);
        cur_us = now.tv_sec * 1000000 + now.tv_usec;
        searches++;
    }
    //end while
    //return a(BESTCHILD(v_0,0));

    int best = root->finalBestChild();
    fprintf(stderr, "board:\n");
    for (int j = 0;j < M;j++) {
        for (int k = 0;k < N;k++) {
            char ch = '\0';
            if (j == root->childX[best] && k == best) {
                ch = '!';
            } else if (j == noX && k == noY) {
                ch = 'z';
            } else if (origBoard[j][k] == 0) {
                ch = ' ';
            } else if (origBoard[j][k] == 1) {
                ch = 'x';
            } else if (origBoard[j][k] == 2) {
                ch = 'o';
            }
            fprintf(stderr, "%c", ch);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "children:\n");
    root->print(0);
    if (root->children[best]) {
        fprintf(stderr, "output: (%d, %d), score: %.2f, searches: %ld\n", root->childX[best], best, root->children[best]->Q, searches);
    } else {
        fprintf(stderr, "output: (%d, %d), searches: %ld\n", root->childX[best], best, searches);
    }

    placeX = root->childX[best];
    placeY = best;

    fprintf(stderr, "cache: %zu entries\n", nodeCache.size());

    // reclaim memory
    nodeCache.clear();
    allocator.clear();

    gettimeofday(&now, NULL);
    cur_us = now.tv_sec * 1000000 + now.tv_usec;
    fprintf(stderr, "time: %.2f s\n", (cur_us - us) / 1000000.0);
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
            node = node->bestChild();
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
int UCT::defaultPolicy(UCTNode *node) {
    int currentPlayer = 3 - node->player;
    bool isFirstStep = true;

    // use cached result
    if (node->cachedResult != -1) {
        return node->cachedResult;
    }

    if (currentBitBoard[node->player].win()) {
        // node player win
        node->cachedResult = 2;
        node->Q = 1;
        node->steps = 1;
        node->expandNum = 0;
        node->endNode = true;
        return 2;
    }

    while (1) {
        if (isTie(UCT::N, UCT::currentTop)) {
            // tie
            if (isFirstStep) {
                node->cachedResult = 1;
                node->Q = 0.5;
                node->steps = 1;
                node->expandNum = 0;
                node->endNode = true;
            }
            return 1;
        }

        // check early
        int deadPlace = 0;
        int deadX = 0, deadY = 0;
        for (int y = 0;y < N;y++) {
            if (currentTop[y] < 1) {
                continue;
            }

            int x = currentTop[y] - 1;
            if (currentBitBoard[currentPlayer].winIfSet(x, y)) {
                int result = currentPlayer == node->player ? 2 : 0;
                if (isFirstStep) {
                    node->cachedResult = result;
                    node->Q = (float)result / 2;
                    node->steps = 1;
                    node->expandNum = 0;
                    node->endNode = true;
                }
                return result;
            }

            if (currentBitBoard[3 - currentPlayer].winIfSet(x, y)) {
                deadPlace ++;
                deadX = x;
                deadY = y;
            }
        }

        if (deadPlace >= 2) {
            // always lose
            debug("early dead at %d %d\n", deadX, deadY);
            int result = currentPlayer == node->player ? 0 : 2;
            if (isFirstStep) {
                node->cachedResult = result;
                node->Q = (float)result / 2;
                node->steps = 2;
                node->expandNum = 0;
                node->endNode = true;
            }
            return result;
        }

        if (deadPlace == 1) {
            if (isFirstStep) {
                // no need to expand other children, only one column to place
                if (node->expandNum > 0) {
                    node->expandNum = 1;
                    node->expandNodes[0] = deadY;
                }
                isFirstStep = false;
            }
            currentTop[deadY] --;
            if (deadX - 1 == noX && deadY == noY) {
                currentTop[deadY]--;
            }
            currentBitBoard[currentPlayer].set(deadX, deadY);
            currentPlayer = 3 - currentPlayer;
            continue;
        }

        int y = rand() % N;
        while (currentTop[y] == 0) {
            y = rand() % N;
        }
        int x = --currentTop[y];

        // skip invalid
        if (x - 1 == noX && y == noY) {
            currentTop[y]--;
        }
        currentBitBoard[currentPlayer].set(x, y);

        currentPlayer = 3 - currentPlayer;
        isFirstStep = false;
    }
    assert(false);
}
