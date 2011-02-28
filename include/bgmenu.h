#ifndef __BGMENU_H__
#define __BGMENU_H__

void bgmenu_forb(int row, uint8_t focus);
void bgmenu_focus(int row);
void bgmenu_blur(int row);
void bgmenu_write_prompt(int row, int val);
uint8_t bgmenu_display(game_t *game);
#endif
