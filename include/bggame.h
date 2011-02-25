#ifndef __BGGAME_H__
#define __BGGAME_H__

// maximum size of the game board
#define MAX_WIDTH 20
#define MAX_HEIGHT 4

struct game {
    // size of the board
    int width, height;
    // number of unique piece types
    int variety;
    // board state (with enough room for the larges board)
    char board[MAX_HEIGHT][MAX_WIDTH];
};

#endif
