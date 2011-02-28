// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

#include <stdlib.h>
#include <inttypes.h>

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "lcd.h" //add nerdkits-provided library

#include "nkbuttons.h"
#include "nkrand.h"
#include "nklcd.h"
#include "nktimer.h"
#include "nksleep.h"

#include "bggame.h"
#include "bgmenu.h"
#include "bghighscore.h"

int main() {
    struct game game;
    // idle/sleep timer
    uint8_t idle = 0;

    // the playing board
    game.width = MAX_WIDTH;
    game.height = MAX_HEIGHT;
    game.variety = 5;

    nklcd_init();
    nkbuttons_init();
    nktimer_init(60);
    nkrand_init();
    srand(nkrand_seed());
    bghighscore_init();
    sei(); //enable interrupts

    while(1) {
        if (bgmenu_display(&game)) {
            idle = 0;
            bggame_play(&game);
            bggame_over(game.score);
            bghighscore_maybe(game.score);
        } else if(++idle > 4) {
            // go to sleep after cycling menu<->highscore
            // without a game several times
            idle = 0;
            nksleep_standby();
        }
        bghighscore_screen();
    }

    return 0;
}
