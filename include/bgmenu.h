#ifndef __BGMENU_H__
#define __BGMENU_H__

void bgmenu_forb(int8_t row, uint8_t focus);
void bgmenu_focus(int8_t row);
void bgmenu_blur(int8_t row);
void bgmenu_write_prompt(int8_t row, int8_t val);
int8_t bgmenu_previous_prompt(int8_t current);
int8_t bgmenu_next_prompt(int8_t current);
int8_t bgmenu_refocus(int8_t blur, int8_t focus);
int8_t bgmenu_field_and_limits(int8_t prompt, game_t *game,
                               int8_t **field, int8_t *min, int8_t *max);
void bgmenu_increase_prompt(int8_t prompt, game_t *game);
void bgmenu_decrease_prompt(int8_t prompt, game_t *game);
uint8_t bgmenu_display(game_t *game);

#endif
