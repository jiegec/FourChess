#include "UCTNode.h"
#include "UCT.h"
#include <math.h>
#include <stdio.h>

UCTNode::UCTNode(int x, int y, int player, UCTNode *parent) {
    this->x = x;
    this->y = y;
    this->player = player;
    this->parent = parent;
    endNode = -1;

    Q = 0;
    N = 0;

    expandNum = 0;
    for (int i = 0;i < UCT::N;i++) {
        if (UCT::currentTop[i]) {
            expandNodes[expandNum++] = i;
        }
        children[i] = nullptr;
    }

    // check if this is an end node?
    if (x == -1 || y == -1) {
        // root node
        endNode = false;
    } else {
        // somebody wins, or no moves are possible
        endNode = (player == PLAYER_OTHER && userWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) || 
            (player == PLAYER_ME && machineWin(x, y, UCT::M, UCT::N, UCT::currentBoard)) ||
            expandNum == 0;
    }
}

UCTNode::~UCTNode() {
    for (int i = 0;i < UCT::N;i++) {
        delete children[i];
        children[i] = nullptr;
    }
}

int UCTNode::checkEnd() {
    return endNode ? 0 : 1;
}

// function EXPAND
UCTNode *UCTNode::expandOne() {
    int child = rand() % expandNum;
    int yy = expandNodes[child];
    int xx = --UCT::currentTop[yy];
    UCT::currentBoard[xx][yy] = 3 - player;

    // skip invalid
    if (UCT::currentTop[yy] == UCT::noX + 1 && yy == UCT::noY) {
        UCT::currentTop[yy] --;
    }

    children[yy] = new UCTNode(xx, yy, 3 - player, this);

    // remove expand
    expandNum --;
    int temp = expandNodes[expandNum];
    expandNodes[expandNum] = expandNodes[child];
    expandNodes[child] = temp;

    return children[yy];
}

// function BESTCHILD
UCTNode *UCTNode::bestChild(float coef) {
    float max = -1024768;
    UCTNode *best = nullptr;
    int bestY = 0;
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q / 2.0 / children[i]->N + coef * sqrt(2 * log(N) / children[i]->N);
            if (num > max) {
                max = num;
                best = children[i];
                bestY = i;
            }
        }
    }

    if (coef != 0.0) {
        // replay
        UCT::currentBoard[--UCT::currentTop[bestY]][bestY] = children[bestY]->player;
        if (UCT::currentTop[bestY] == UCT::noX + 1 && bestY == UCT::noY) {
            UCT::currentTop[bestY] --;
        }
    }

    return best;
}

// function Backup
void UCTNode::backup(int delta) {
    UCTNode *cur = this;
    while (cur) {
        cur->N += 1;
        cur->Q += delta;
        delta = 2 - delta;
        cur = cur->parent;
    }
}

void UCTNode::print() {
    fprintf(stderr, "children\n");
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            fprintf(stderr, "(%d, %d): %.1f / %d = %.2f\n", children[i]->x, children[i]->y, children[i]->Q / 2.0, children[i]->N, children[i]->Q / 2.0 / children[i]->N);
        }
    }
}
