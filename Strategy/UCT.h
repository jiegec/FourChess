#ifndef UCT_H
#define UCT_H

#include "Judge.h"
#include <cassert>
#include <cstdint>
#include <stdlib.h>
#include <stdint.h>
#include <string>

const int PLAYER_ME = 2;
const int PLAYER_OTHER = 1;

class UCTNode;
class BitBoard;

class UCT {
public:
    static int M, N;
    static int noX, noY;
    static int currentTop[MAX_N];
    // each player has its bit board
    static BitBoard currentBitBoard[3];

    UCT(int M, int N, int noX, int noY);

    // function UCTSEARCH(S_0)
    void Search(const int * const * origBoard, const int * origTop, int &placeX, int &placeY);

    // function TreePolicy(v)
    UCTNode *treePolicy(UCTNode *node);

    // function DEFAULTPOLICY(s)
    int defaultPolicy(UCTNode *node);
};

struct RShift3 {
    BitBoard operator ()(BitBoard self, int amount);
};
struct RShift2 {
    BitBoard operator ()(BitBoard self, int amount);
};

// https://github.com/denkspuren/BitboardC4/blob/master/BitboardDesign.md
class BitBoard {
public:
    // at most (MAX_M+1)*MAX_N=156 bits, less than 192 bits
    // stores a matrix of (M+1) rows, N cols
    // when N=M=6:
    //         6 13 20 27 34 41 48
    //       +---------------------+
    // row 0 | 5 12 19 26 33 40 47 |
    //       | 4 11 18 25 32 39 46 |
    //       | 3 10 17 24 31 38 45 |
    //       | 2  9 16 23 30 37 44 |
    //       | 1  8 15 22 29 36 43 |
    // row 5 | 0  7 14 21 28 35 42 |
    //       +---------------------+
    //     col 0  1  2  3  4  5  6
    uint64_t data[3];

    BitBoard() {
        data[0] = data[1] = data[2] = 0;
    }

    BitBoard(const BitBoard &other) {
        data[0] = other.data[0];
        data[1] = other.data[1];
        data[2] = other.data[2];
    }

    BitBoard(int player, int board[MAX_M][MAX_N]) {
        data[0] = data[1] = data[2] = 0;

        for (int j = 0;j < UCT::M;j++) {
            for (int k = 0;k < UCT::N;k++) {
                if (board[j][k] == player) {
                    int bit = k * (UCT::M + 1) + UCT::M - j - 1;
                    data[bit / 64] |= (uint64_t)1 << (bit % 64);
                }
            }
        }
    }

    template <typename Shifter>
    bool winInner(Shifter rshift) const{
        // learned from https://github.com/qu1j0t3/fhourstones/blob/bf0e70ed9fe8128eeea8539f17dd41826f2cc6b6/Game.c#L108
        // A & (A >> S) & (A >> (2 * S)) & (A >> (3 * S))
        // becomes:
        // temp = A & (A >> S)
        // temp & (temp >> (2 * S))

        // row: shift by M+1
        BitBoard temp = (*this) & (rshift(*this, UCT::M + 1));
        if (temp & (rshift(temp, 2 * (UCT::M + 1)))) {
            return true;
        }

        // col: shift by 1
        temp = (*this) & (rshift(*this, (1)));
        if (temp & (rshift(temp, (2 * (1))))) {
            return true;
        }

        // diag \: shift by M
        temp = (*this) & (rshift(*this, (UCT::M)));
        if (temp & (rshift(temp, (2 * (UCT::M))))) {
            return true;
        }

        // diag /: shift by M+2
        temp = (*this) & (rshift(*this, (UCT::M + 2)));
        if (temp & (rshift(temp, (2 * (UCT::M + 2))))) {
            return true;
        }

        return false;
    }


    inline bool win() const {
        if ((UCT::M + 1) * UCT::N > 128) {
            return winInner(RShift3{});
        } else {
            return winInner(RShift2{});
        }
    }

    inline void set(int j, int k) {
        int bit = k * (UCT::M + 1) + UCT::M - j - 1;
        // assert((data[bit / 64] & ((uint64_t)1 << (bit % 64))) == 0);
        data[bit / 64] |= (uint64_t)1 << (bit % 64);
    }

    inline bool winIfSet(int j, int k) const {
        BitBoard temp = *this;
        temp.set(j, k);
        return temp.win();
    }

    void unset(int j, int k) {
        int bit = k * (UCT::M + 1) + UCT::M - j - 1;
        // assert((data[bit / 64] & ((uint64_t)1 << (bit % 64))) != 0);
        data[bit / 64] &= ~((uint64_t)1 << (bit % 64));
    }

    bool operator == (const BitBoard &other) const {
        return data[0] == other.data[0] && data[1] == other.data[1] && data[2] == other.data[2];
    }


    inline BitBoard operator & (const BitBoard &other) const {
        BitBoard res;
        res.data[0] = data[0] & other.data[0];
        res.data[1] = data[1] & other.data[1];
        res.data[2] = data[2] & other.data[2];
        return res;
    }

    void operator = (const BitBoard &other) {
        data[0] = other.data[0];
        data[1] = other.data[1];
        data[2] = other.data[2];
    }

    inline operator bool () const {
        return data[0] || data[1] || data[2];
    }

    std::string print() const {
        std::string result;
        for (int j = 0;j < UCT::M;j++) {
            for (int k = 0;k < UCT::N;k++) {
                int bit = k * (UCT::M + 1) + UCT::M - j - 1;
                if (data[bit / 64] & ((uint64_t)1 << (bit % 64))) {
                    result += '1';
                } else {
                    result += ' ';
                }
            }
            result += '\n';
        }
        return result;
    }
};

template<>
struct std::hash<std::pair<BitBoard, BitBoard>>
{
    std::size_t operator()(const std::pair<BitBoard, BitBoard>& board) const noexcept
    {
        std::size_t h1 = std::hash<uint64_t>{}(board.first.data[0]);
        std::size_t h2 = std::hash<uint64_t>{}(board.first.data[1]);
        std::size_t h3 = std::hash<uint64_t>{}(board.first.data[2]);
        std::size_t h4 = std::hash<uint64_t>{}(board.second.data[0]);
        std::size_t h5 = std::hash<uint64_t>{}(board.second.data[1]);
        std::size_t h6 = std::hash<uint64_t>{}(board.second.data[2]);
        return h1 ^ (h2 << 3) ^ (h3 << 5) ^ (h4 << 7) ^ (h5 << 11) ^ (h6 << 13);
    }
};

inline BitBoard RShift3::operator ()(BitBoard self, int amount) {
    BitBoard res;
    // assert(0 < amount && amount < 64);
    res.data[0] = (self.data[0] >> amount) | (self.data[1] << (64 - amount));
    res.data[1] = (self.data[1] >> amount) | (self.data[2] << (64 - amount));
    res.data[2] = self.data[2] >> amount;
    return res;
}

inline BitBoard RShift2::operator ()(BitBoard self, int amount) {
    BitBoard res;
    // assert(0 < amount && amount < 64);
    res.data[0] = (self.data[0] >> amount) | (self.data[1] << (64 - amount));
    res.data[1] = self.data[1] >> amount;
    return res;
}


#endif
