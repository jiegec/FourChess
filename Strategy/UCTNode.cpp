#include "UCTNode.h"
#include "UCT.h"
#include <math.h>
#include <memory>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

BumpAllocator allocator;
std::unordered_map<std::pair<BitBoard, BitBoard>, UCTNode*> nodeCache;

UCTNode::UCTNode(int player, UCTNode *parent) {
    this->player = player;
    this->parent = parent;
    endNode = -1;

    Q = 0.0;
    selfN = 0;
    selfQ = 0;

    expandNum = 0;
    for (int i = 0;i < UCT::N;i++) {
        if (UCT::currentTop[i]) {
            expandNodes[expandNum++] = i;
        }
        children[i] = nullptr;
        childVisit[i] = 0;
        childX[i] = -1;
    }
    sumChildVisit = 0;

    // check if this is an end node?
    // somebody wins, or no moves are possible
    endNode = UCT::currentBitBoard[player].win() || expandNum == 0;

    cachedResult = -1; // invalid
    steps = -1;
}

UCTNode::~UCTNode() {
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

    // find in cache
    auto it = nodeCache.find(
        {UCT::currentBitBoard[PLAYER_OTHER], UCT::currentBitBoard[PLAYER_ME]}
    );
    if (it == nodeCache.end()) {
        UCTNode *buffer = allocator.allocate();
        UCTNode *node = new (buffer) UCTNode(3 - player, this);
        children[yy] = node;
        nodeCache.insert({
            {UCT::currentBitBoard[PLAYER_OTHER], UCT::currentBitBoard[PLAYER_ME]},
            node
        });
    } else {
        // reuse
        children[yy] = it->second;
        // redirect parent
        children[yy]->parent = this;
    }
    childX[yy] = xx;
    childVisit[yy] += 1;
    sumChildVisit++;

    // remove expand
    expandNum --;
    int temp = expandNodes[expandNum];
    expandNodes[expandNum] = expandNodes[child];
    expandNodes[child] = temp;

    return children[yy];
}

// function BESTCHILD
UCTNode *UCTNode::bestChild() {
    float max = -1024768;
    UCTNode *best = nullptr;
    int bestY = -1;

    // count all child visits
    int N = selfN + sumChildVisit;
    float logN2 = 2 * logf(N);
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q + sqrtf(logN2 / childVisit[i]);
            if (num > max || best == nullptr) {
                max = num;
                best = children[i];
                bestY = i;
            }
        }
    }

    // replay
    childVisit[bestY]++;
    sumChildVisit++;
    int bestX = --UCT::currentTop[bestY];
    assert(bestX == childX[bestY]);
    UCT::currentBitBoard[children[bestY]->player].set(bestX, bestY);
    if (UCT::currentTop[bestY] == UCT::noX + 1 && bestY == UCT::noY) {
        UCT::currentTop[bestY] --;
    }

    // remember to redirect parent pointer
    best->parent = this;

    return best;
}

// function BESTCHILD
int UCTNode::finalBestChild() {
    int minSteps = 1048576;
    int best = -1;
    // any child always wins? select the child with least steps
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            if (children[i]->cachedResult == 2 && minSteps > children[i]->steps) {
                minSteps = children[i]->steps;
                best = i;
            }
        }
    }
    if (best != -1) {
        return best;
    }

    // all child always loses? select the child with most steps
    best = -1;
    bool allLose = true;
    int maxSteps = -1;
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            if (children[i]->cachedResult != 0) {
                allLose = false;
                break;
            } else if (children[i]->steps > maxSteps) {
                maxSteps = children[i]->steps;
                best = i;
            }
        }
    }
    if (allLose && best != -1) {
        return best;
    }

    float max = -1024768;
    best = -1;
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            float num = children[i]->Q;
            if (num > max) {
                max = num;
                best = i;
            }
        }
    }

    return best;
}

// function Backup
void UCTNode::backup(int delta) {
    // propagate cached result
    if (cachedResult != -1) {
        UCTNode *cur = this;
        while (cur) {
            if (cur->parent && cur->cachedResult != -1) {
                if (cur->cachedResult == 2) {
                    // cur always win: parent always lose
                    cur->parent->cachedResult = 0;
                    cur->parent->steps = cur->steps;
                    cur->parent->endNode = true;
                    cur->parent->selfQ = 0.0;
                    cur->parent->Q = 0.0;
                    cur = cur->parent;
                    continue;
                } else if (cur->cachedResult == 0 && cur->parent->expandNum == 0) {
                    // cur always lose: see if all children of parent always lose
                    bool allKnownZero = true;
                    int8_t maxSteps = -1;
                    for (int i = 0;i < UCT::N;i++) {
                        if (cur->parent->children[i]) {
                            if (cur->parent->children[i]->cachedResult != 0) {
                                allKnownZero = false;
                            } else {
                                maxSteps = std::max(maxSteps, cur->parent->children[i]->steps);
                            }
                        }
                    }

                    if (allKnownZero) {
                        cur->parent->cachedResult = 2;
                        cur->parent->steps = maxSteps + 1;
                        cur->parent->endNode = true;
                        cur->parent->Q = 1.0;
                        cur->parent->selfQ = cur->parent->selfN;
                        cur = cur->parent;
                        continue;
                    }
                }
            }

            if (cur->cachedResult == -1) {
                // fixup score
                // weighted sum of children scores, weighted by edge visit count
                float weightedSum = 0;
                for (int i = 0;i < UCT::N;i++) {
                    if (cur->children[i]) {
                        weightedSum += cur->childVisit[i] * (1.0f - cur->children[i]->Q);
                    }
                }

                weightedSum += cur->selfQ * cur->selfN;
                cur->Q = weightedSum / (cur->sumChildVisit + cur->selfN);
            }

            cur = cur->parent;
        }
    } else {
        // defaultPolicy invocation of this node
        selfQ = (selfQ * selfN + (float)delta / 2.0f) / (selfN + 1);
        selfN += 1;

        UCTNode *cur = this;
        while (cur) {
            // weighted sum of children scores, weighted by edge visit count
            float weightedSum = 0;
            for (int i = 0;i < UCT::N;i++) {
                if (cur->children[i]) {
                    weightedSum += cur->childVisit[i] * (1.0f - cur->children[i]->Q);
                }
            }

            weightedSum += cur->selfQ * cur->selfN;
            cur->Q = weightedSum / (cur->sumChildVisit + cur->selfN);
            cur = cur->parent;
        }
    }
}

void UCTNode::print(int depth, int tab) {
    for (int i = 0;i < UCT::N;i++) {
        if (children[i]) {
            for (int i = 0;i < tab;i++) {
                fprintf(stderr, "    ");
            }
            fprintf(stderr, "(%d, %d): %.2f of %d visits", childX[i], i, children[i]->Q, childVisit[i]);
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
