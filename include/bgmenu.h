#ifndef __BGMENU_H__
#define __BGMENU_H__

void bgmenu_forb(int row, uint8_t focus);
void bgmenu_focus(int row);
void bgmenu_blur(int row);
void bgmenu_write_prompt(int row, int val);
int bgmenu_previous_prompt(int current);
int bgmenu_next_prompt(int current);
int bgmenu_refocus(int blur, int focus);
int bgmenu_field_and_limits(int prompt, game_t *game,
                            int **field, int *min, int *max);
void bgmenu_increase_prompt(int prompt, game_t *game);
void bgmenu_decrease_prompt(int prompt, game_t *game);
uint8_t bgmenu_display(game_t *game);

#endif
