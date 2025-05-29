#include "UCT.h"
#include "UCTNode.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

int UCT::M;
int UCT::N;
int UCT::noX;
int UCT::noY;
int UCT::currentTop[MAX_N];
int UCT::currentBoard[MAX_M][MAX_N];

UCT::UCT(int M, int N, int noX, int noY) {
    UCT::M = M;
    UCT::N = N;
    UCT::noX = noX;
    UCT::noY = noY;
}

// function UCTSEARCH(S_0)
void UCT::Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY) {
    memcpy(currentTop, origTop, N * sizeof(int));
    
    //以状态s_0创建根节点v_0;
    UCTNode *root = new UCTNode(-1, -1, PLAYER_OTHER, nullptr);

    struct timeval begin, now;
    gettimeofday(&begin, NULL);
    long us = begin.tv_sec * 1000000 + begin.tv_usec;
    long cur_us = us;
    long searches = 0;
    //while 尚未用完计算时长 do:
    while (cur_us < us + 2900000) {
        for (int j = 0;j < M;j++) {
            for (int k = 0;k < N;k++) {
                currentBoard[j][k] = origBoard[j][k];
            }
        }
        memcpy(currentTop, origTop, N * sizeof(int));

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

    UCTNode *best = root->bestChild(0.0);
    fprintf(stderr, "board:\n");
    for (int j = 0;j < M;j++) {
        for (int k = 0;k < N;k++) {
            char ch = '\0';
            if (j == best->x && k == best->y) {
                ch = '!';
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
    root->print();
    fprintf(stderr, "win rate: %.2f, searches: %ld\n", best->Q / 2.0 / best->N, searches);

    placeX = best->x;
    placeY = best->y;

    delete root;
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
            node = node->bestChild(1.0);
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
    int winnerPlayer = 0;
    debug("begin emulation:\n");
    for (int i = 0;i < M;i++) {
        for (int j = 0;j < N;j++) {
            debug("%d ", currentBoard[i][j]);
        }
        debug("\n");
    }

    if (node->player == PLAYER_ME && machineWin(node->x, node->y, UCT::M, UCT::N, UCT::currentBoard)) {
        winnerPlayer = PLAYER_ME;
        goto end;
    } else if (node->player == PLAYER_OTHER && userWin(node->x, node->y, UCT::M, UCT::N, UCT::currentBoard)) {
        winnerPlayer = PLAYER_OTHER;
        goto end;
    }
    while (1) {
        if (isTie(UCT::N, UCT::currentTop)) {
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
            currentBoard[x][y] = currentPlayer;
            if (currentPlayer == PLAYER_ME && machineWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
                winnerPlayer = PLAYER_ME;
                goto end;
            } else if (currentPlayer == PLAYER_OTHER && userWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
                winnerPlayer = PLAYER_OTHER;
                goto end;
            }

            currentBoard[x][y] = 3 - currentPlayer;
            if (currentPlayer == PLAYER_ME && userWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
                deadPlace ++;
                deadX = x;
                deadY = y;
            } else if (currentPlayer == PLAYER_OTHER && machineWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) {
                deadPlace ++;
                deadX = x;
                deadY = y;
            }
            currentBoard[x][y] = 0;
        }

        debug("one step:\n");
        for (int i = 0;i < M;i++) {
            for (int j = 0;j < N;j++) {
                debug("%d ", currentBoard[i][j]);
            }
            debug("\n");
        }
        if (deadPlace >= 2) {
            // always lose
            debug("early dead at %d %d\n", deadX, deadY);
            winnerPlayer = 3 - currentPlayer;
            goto end;
        }

        if (deadPlace == 1) {
            currentTop[deadY] --;
            if (deadX - 1 == noX && deadY == noY) {
                currentTop[deadY]--;
            }
            currentBoard[deadX][deadY] = currentPlayer;
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
        currentBoard[x][y] = currentPlayer;

        // check if opponent can win right after we place this
        if (x > 0 && x - 1 != noX) {
            currentBoard[x-1][y] = 3 - currentPlayer;
            if (currentPlayer == PLAYER_ME && userWin(x-1, y, UCT::M, UCT::N, UCT::currentBoard)) {
                winnerPlayer = PLAYER_OTHER;
                goto end;
            } else if (currentPlayer == PLAYER_OTHER && machineWin(x-1, y, UCT::M, UCT::N, UCT::currentBoard)) {
                winnerPlayer = PLAYER_ME;
                goto end;
            }
            currentBoard[x-1][y] = 0;
        }

        currentPlayer = 3 - currentPlayer;
    }
end:
    return winnerPlayer == node->player ? 2 : 0;
}
