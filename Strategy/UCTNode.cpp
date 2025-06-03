#include "UCTNode.h"
#include "UCT.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

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
        endNode = UCT::currentBitBoard[player].win() || expandNum == 0;
    }

    cachedResult = -1; // invalid
    steps = -1;
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
    UCT::currentBitBoard[3 - player].set(xx, yy);

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
    float logN2 = 2 * logf(N);
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q / 2.0f / children[i]->N + coef * sqrtf(logN2 / children[i]->N);
            if (num > max) {
                max = num;
                best = children[i];
                bestY = i;
            }
        }
    }

    // replay
    int bestX = --UCT::currentTop[bestY];
    assert(bestX == best->x && bestY == best->y);
    UCT::currentBitBoard[children[bestY]->player].set(bestX, bestY);
    if (UCT::currentTop[bestY] == UCT::noX + 1 && bestY == UCT::noY) {
        UCT::currentTop[bestY] --;
    }

    return best;
}

// function BESTCHILD
UCTNode *UCTNode::finalBestChild() {
    int minSteps = 1048576;
    UCTNode *best = nullptr;
    // any child always wins? select the child with least steps
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            if (children[i]->cachedResult == 2 && minSteps > children[i]->steps) {
                minSteps = children[i]->steps;
                best = children[i];
            }
        }
    }
    if (best) {
        return best;
    }

    // all child always loses? select the child with most steps
    bool allLose = true;
    int maxSteps = -1;
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            if (children[i]->cachedResult != 0) {
                allLose = false;
                break;
            } else if (children[i]->steps > maxSteps) {
                maxSteps = children[i]->steps;
                best = children[i];
            }
        }
    }
    if (allLose && best) {
        return best;
    }
    best = nullptr;

    float max = -1024768;
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q / 2.0f / children[i]->N;
            if (num > max) {
                max = num;
                best = children[i];
            }
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

void UCTNode::print(int depth, int tab) {
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            for (int i = 0;i < tab;i++) {
                fprintf(stderr, "    ");
            }
            fprintf(stderr, "(%d, %d): %.1f / %d = %.2f", children[i]->x, children[i]->y, children[i]->Q / 2.0, children[i]->N, children[i]->Q / 2.0 / children[i]->N);
            if (children[i]->cachedResult != -1) {
                fprintf(stderr, " %s in %d steps", 
                    (children[i]->cachedResult == 0 ? "lose" : (children[i]->cachedResult == 1 ? "tie" : "win")),
                    children[i]->steps);
            }
            fprintf(stderr, "\n");

            if (depth > 0) {
                children[i]->print(depth - 1, tab + 1);
            }
        }
    }
}

void UCTNode::propagateCachedResult() {
    if (parent) {
        // if this is a win node: parent must lose
        if (cachedResult == 2) {
            parent->cachedResult = 0;
            parent->steps = steps;
            parent->endNode = true;
            parent->Q = 0;
            parent->propagateCachedResult();
        } else if (parent->expandNum == 0) {
            // if all children has cached result equals to zero: parent must win
            bool allKnownZero = true;
            int maxSteps = -1;
            for (int i = 0;i < UCT::N;i++) {
                if (parent->children[i]) {
                    if (parent->children[i]->cachedResult != 0) {
                        allKnownZero = false;
                    } else {
                        maxSteps = std::max(maxSteps, parent->children[i]->steps);
                    }
                }
            }

            if (allKnownZero) {
                parent->cachedResult = 2;
                parent->steps = maxSteps + 1;
                parent->endNode = true;
                parent->Q = 2 * parent->N;
                parent->propagateCachedResult();
            }
        }
    }
}
