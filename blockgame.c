// blockgame
// for NerdKits with ATmega168

#define F_CPU 14745600

#include <stdio.h>
#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/delay.h"
#include "../libnerdkits/lcd.h"
#include "../libnerdkits/uart.h"

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC5)

#define B_LEFT_INT   (1<<PCINT8)
#define B_DOWN_INT   (1<<PCINT9)
#define B_UP_INT     (1<<PCINT10)
#define B_RIGHT_INT  (1<<PCINT11)
#define B_SELECT_INT (1<<PCINT13)

// last button state
volatile uint8_t button_states = 0;

// what buttons were freshly pushed
volatile uint8_t buttons_pushed = 0;

// wheter or not the animate timer has clicked
volatile int animate = 0;

ISR(PCINT1_vect) {
    uint8_t fresh_buttons = PINC & (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
    
    if (!buttons_pushed) {
        // only allow button presses when asked
        // and require a button to be released before activating again
        buttons_pushed = (button_states ^ fresh_buttons) & fresh_buttons;
        button_states = fresh_buttons;
    } else {
        // only *clear* states,
        // do not set them until pushed has been cleared
        button_states &= fresh_buttons;
    }
}

ISR(TIMER0_COMPA_vect) {
    // time to cycle animations
    animate = 1;
}

// initialize the board
void boot_board(char board[3][10]) {
    int c;
    for (c = 0; c < 10; c+=3) {
        board[0][c] = 'a';
        board[0][c+1] = 'b';
        board[0][c+2] = 'c';
        board[1][c] = 'c';
        board[1][c+1] = 'a';
        board[1][c+2] = 'b';
        board[2][c] = 'b';
        board[2][c+1] = 'c';
        board[2][c+2] = 'a';
    }
}

// get the input pins setup at boot
void boot_pins() {
    // Set the 6 pins to input mode - four directions + select
    DDRC &= ~(B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
	  
    // turn on the internal resistors for the pins
    PORTC |= (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);

    // enable bin change mask registers
    PCMSK1 |= (B_LEFT_INT|B_DOWN_INT|B_UP_INT|B_RIGHT_INT|B_SELECT_INT);

    // enable Pin Change Interrupt 1
    PCICR |= (1<<PCIE1);
}

// configure the animation timer at boot
void boot_timer() {
  // Clear Timer on Compare Match of OCRA
  TCCR0A |= (1<<WGM01);

  // system clock is ~1.47Mhz
  // 1470000/1024 = 1435
  // 1435/30 = 212 (30Hz being target frame rate)
  // choose clock source as system/prescaler1024
  TCCR0B |= (1<<CS02) | (1<<CS00);

  // choose the value for Output Compare A
  OCR0A = 212;
  
  // endable Timer Output Compare Match A Interrupt 0
  TIMSK0 |= (1<<OCIE0A);
}

// handle any directional button pushes
void move_cursor(int* row, int* column) {
    if (buttons_pushed & B_UP)
        (*row)--;
    if (buttons_pushed & B_DOWN)
        (*row)++;
    if (buttons_pushed & B_LEFT)
        (*column)--;
    if (buttons_pushed & B_RIGHT)
        (*column)++;

    if (*row > 2) row = 0;
    if (*column > 9) column = 0;
}

// handle a select button push
void do_select(char board[3][10], int row, int column) {
    if (buttons_pushed & B_SELECT) {
        if (board[row][column] >= 'a')
            board[row][column] = board[row][column]-('a'-'A');
        else
            board[row][column] = board[row][column]+('a'-'A');
    }
}

void write_row(char row[10]) {
    int c;

    for (c = 0; c < 10; c++)
        lcd_write_data(row[c]);
}

void write_board(char board[3][10]) {
    lcd_home();
    write_row(board[0]);
    lcd_line_two();
    write_row(board[1]);
    lcd_line_three();
    write_row(board[2]);
}

void show_button(uint8_t button, char label) {
    lcd_write_data((button_states & button) ? label : ' ');
    lcd_write_data(' ');
}

void show_buttons() {
    lcd_line_four();
    show_button(B_LEFT,   'L');
    show_button(B_DOWN,   'D');
    show_button(B_UP,     'U');
    show_button(B_RIGHT,  'R');
    show_button(B_SELECT, 'S');
}

int main() {

    // row and column of the cursor
    int row = 0, column = 0;
    char board[3][10];
    boot_board(board);

    // get the LCD setup at boot -- must be done in main?!
    // -- or was that an issue with opening the FILE stream?
    lcd_init();
    lcd_home();

    boot_pins();
    boot_timer();

    sei(); //enable interrupts
    animate = 1; // draw the board the first time
    while(1) {
        if(buttons_pushed) {
            move_cursor(&row, &column);
            do_select(board, row, column);
            animate = 1;
            buttons_pushed = 0;
        }
        
        if (animate) {
            write_board(board);
            animate = 0;
        }

        show_buttons(); // debug
    }
    return 0;
}
