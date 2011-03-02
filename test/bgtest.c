/* bgtest: unit tests for parts of blockgame */

#include <stdio.h>

#include <inttypes.h>

#include "bggame.h"

#define PASS 0
#define FAIL 1

#define ASSERT_GAME(Test, Game)                                  \
    if (!(Test)) {                                               \
        printf("Assertion failed, line %d:\n", __LINE__);        \
        print_game(Game);                                        \
        return FAIL;                                             \
    }

#define TEST(Test)                              \
    printf(#Test " ... ");                      \
    if (Test()==PASS) {                         \
        printf("PASS\n");                       \
    } else {                                    \
        printf("FAIL\n");                       \
        pass = FAIL;                            \
    }

void print_game(game_t);
int valid_move_test_SIMPLE();
int valid_move_test_BITTEST();

int main() {
    int pass = PASS;
    printf("Beginning tests...\n");
    TEST(valid_move_test_SIMPLE);
    TEST(valid_move_test_BITTEST);
    printf("Tests finished: %s\n", pass == PASS ? "PASS" : "FAIL");
    return pass;
}

// TESTS

int valid_move_test_SIMPLE() {
    game_t game = {.width=3,
                   .height=3,
                   .variety=9,
                   .board={ {'a', 'b', 'c'},
                            {'a', 'd', 'e'},
                            {'f', 'a', 'z'} }
    };

    //SIMPLE has a valid move (move 'a' in lower middle to the left)
    ASSERT_GAME(bggame_valid_move_exists(game), game);
    return PASS;
}

int valid_move_test_BITTEST() {
    game_t game = {.width=3,
                   .height=3,
                   .variety=9,
                   .board={ {'a', 'b', 'c'},
                            {'a', 'd', 'e'},
                            {'f', 'q', 'z'} }
    };

    // BITTEST has no valid move ('a' is now 'q')
    // this test captures a bug that bggame_match has:
    // comparing only four bits of the character, not five
    ASSERT_GAME(!bggame_valid_move_exists(game), game);
    return PASS;
}

// UTILS

void print_game(game_t game) {
    int r, c;
    printf("%d x %d (%d)\n", game.width, game.height, game.variety);
    for (r = 0; r < game.height; r++) {
        printf("   ");
        for (c = 0; c < game.width; c++)
            putchar(game.board[r][c]);
        putchar('\n');
    }
}
