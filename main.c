#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "cpu_speed.h"
#include "graphics.h"
#include "lcd.h"
#include "sprite.h"


// Initial Vars
unsigned char PlayerLives = 0;

struct SPRITE {
	unsigned char x;
	unsigned char y;
};

void initial_screen() {
	
	clear_screen();
	
	draw_string(LCD_X/2-30, LCD_Y/3, "Cian O\'Leary");
	draw_string(LCD_X/2-20, LCD_Y/1.5, "n9727442");

	show_screen();
	
}

void initial_setup() {
	set_clock_speed(CPU_8MHz);

	DDRB = 0b01111100;
	DDRF = 0b10011111;
	DDRD = 0b11111100;

	lcd_init(LCD_DEFAULT_CONTRAST);
}


void update(){
	clear_screen();

	show_screen();
}

int main(void) {
	initial_setup();
	initial_screen();
	_delay_ms(2000);
	
	while (PlayerLives > 0) {
		update();
	}
	clear_screen();	
	draw_string(LCD_X/2-20, LCD_Y/3, "GAME OVER");
	show_screen();
	_delay_ms(2000);
}
