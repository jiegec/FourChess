#include "UCTNode.h"
#include "UCT.h"
#include <math.h>

UCTNode::UCTNode(int x, int y, int player, UCTNode *parent) {
    this->x = x;
    this->y = y;
    this->player = player;
    this->parent = parent;
    endNode = -1;

    Q = 0.0;
    N = 0;

    expandNum = 0;
    for (int i = 0;i < UCT::N;i++) {
        if (UCT::currentTop[i]) {
            expandNodes[expandNum++] = i;
        }
        children[i] = nullptr;
    }
}

UCTNode::~UCTNode() {
    for (int i = 0;i < UCT::N;i++) {
        delete children[i];
    }
}

int UCTNode::checkEnd() {
    if (x == -1 || y == -1) {
        // root node
        return -1;
    }
    if (endNode == -1) {
        // check
        endNode = userWin(x, y, UCT::M, UCT::N, UCT::currentBoard) || 
            machineWin(x, y, UCT::M, UCT::N, UCT::currentBoard) || expandNum == 0;
        return endNode;
    } else {
        return endNode;
    }
}

// function EXPAND
UCTNode *UCTNode::expandOne() {
    int child = rand() % expandNum;
    int yy = expandNodes[child];
    int xx = --UCT::currentTop[yy];
    UCT::currentBoard[xx][yy] = player;

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
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q / children[i]->N + coef * sqrt(2 * log(N) / children[i]->N);
            if (num > max) {
                max = num;
                best = children[i];
            }
        }
    }

    return best;
}

// function Backup
void UCTNode::backup(float delta) {
    UCTNode *cur = this;
    while (cur) {
        cur->N += 1;
        cur->Q += delta;
        delta = 1 - delta;
        cur = cur->parent;
    }
}