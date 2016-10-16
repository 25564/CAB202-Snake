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

enum Directions // Snake Direction
{
    IDLE,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// The standard Sprite was too bloated. Not even sorry
typedef struct sprite {
	float x, y;				// Position of top-left sprite corner
	unsigned char width, height;	// Pixel width and height of sprite
} Sprite;

// Lets implement a linked list for my sanity reasons
typedef struct node {
    Sprite val;
    struct node * next;
} ListNode;

void push(ListNode ** head, Sprite val) { // Add to the front of list
    ListNode * NewNode;
    NewNode = malloc(sizeof(ListNode));

    NewNode->val = val;
    NewNode->next = *head;
    *head = NewNode;
}

void deleteTrailing(ListNode * head) { // Delete Last
	ListNode *temp = head;
	ListNode *t = temp->next;

	while(temp->next != NULL) {
		t=temp;
		temp=temp->next;
	}
	free(t->next);
	t->next=NULL; 
}

void trimList(ListNode * head, int Start){ // Delete all after a certain point
    ListNode *current = head;
    ListNode *next = head->next;
 	int count = 0;

	for (count = 1; count < Start && current != NULL; count++) { // Skip the safe ones
		current = current->next;
	}

	if (current == NULL) {
		return; // They are all safe.... For now
	}

    // The main loop that traverses through the rest of the list
    while (current != NULL) {
        if (current == NULL) // We done boys
            return;
 
        next = current->next; // Save next then delete the current
        free(current);
        current->next = NULL;
        current = next;
    }
}


// End Linked List implementation


// Initial Vars

int PlayerLives = 5;
int PlayerScore = 0;

// Initial Snake Vars
ListNode * SnakeLinkedList = NULL;
unsigned char SnakeLength = 1;
enum Directions SnakeDirection;


// Helper Drawing Functions

void DrawHUD() {
	draw_string(30, 0, "L: ");
	draw_char(40, 0, (char)PlayerLives + '0');
    draw_string(0, 0, "S: ");
	draw_char(10, 0, (char)PlayerScore + '0');
	draw_string(60, 0, "D: ");
	draw_char(70, 0, (char)SnakeDirection + '0');
}


// Snake Controls 
void SnakeLoseLife() {
	PlayerLives = PlayerLives - 1;
	SnakeDirection = IDLE;
}



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

    // Configure timer
    TCCR0B &= ~((1<<WGM02));

    //Prescaler = 256
    //Timer Speed = 128 microseconds
    //Timer Overflow Speed = 8160 micro seconds
    TCCR0B |= (1<<CS02);
    TCCR0B &= ~((1<<CS01));
    TIMSK0 |= (1 << TOIE0);

	sei(); // Globally enable interrupts
	
	// Debugging Linked List
	int test = 0;

	while(test != 10) {
		Sprite firstNode = {test, 4, 4, 4};
		push(&SnakeLinkedList, firstNode);
		test++;
	}
	deleteTrailing(SnakeLinkedList);
	trimList(SnakeLinkedList, 5);
	// End Debugging Linked List
}

// Interrupt that
// Checks for user input
ISR(TIMER0_OVF_vect) {
	// SW2
	if ((PINF >> 6) & 1){
	}

	// SW3
	if ((PINF >> 5) & 1){
	}

	// Left
	if ((PINB >> 1) & 1) {
		if (SnakeDirection == RIGHT){
			SnakeLoseLife();
			return;
		}
		SnakeDirection = LEFT;
	}

	// Right
	if (PIND & 1) {
		if (SnakeDirection == LEFT){
			SnakeLoseLife();
			return;
		}
		SnakeDirection = RIGHT;
	}

	// Down
	if ((PINB >> 7) & 1) {
		if (SnakeDirection == UP){
			SnakeLoseLife();
			return;
		}
		SnakeDirection = DOWN;
	}

	// Up
	if ((PIND >> 1) & 1){
		if (SnakeDirection == DOWN){
			SnakeLoseLife();
			return;
		}
		SnakeDirection = UP;
	}
}

void update(){
	clear_screen();
	DrawHUD();
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

	// Debugging Linked List
    ListNode * current = SnakeLinkedList;

    while (current != NULL) {
    	clear_screen();
    	draw_char(LCD_X/2-20, LCD_Y/3, (char)(current->val.x+'0'));
        current = current->next;
        show_screen();
    	_delay_ms(2000);
    }

    // End Debugging Linked List
}
