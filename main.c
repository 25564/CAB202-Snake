#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cpu_speed.h"
#include "graphics.h"
#include "lcd.h"
#include "usb_serial.h"

#define DebugMode 1
#define InitialSnakeLength 20

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

void draw_sprite(Sprite* sprite ) { 
	unsigned char dx, dy;
	for (dy = 0; dy<sprite->height; dy++) {
		for (dx = 0; dx<sprite->width; dx++) {
			set_pixel(
				(unsigned char) sprite->x+dx,
				(unsigned char) sprite->y+dy + 9,
				1
			);
		}
	}
}

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

// Helper Functions

void draw_centred(unsigned char y, char* string) {
    // Draw a string centred in the LCD when you don't know the string length
    unsigned char l = 0, i = 0;
    while (string[i] != '\0') {
        l++;
        i++;
    }
    char x = 42-(l*5/2);
    draw_string((x > 0) ? x : 0, y, string);
}

bool hasCollided(Sprite sprite1, Sprite sprite2) {

	if (sprite2.x >= (sprite1.x + sprite1.width)) {
		return false;
	}

	if (sprite1.x >= (sprite2.x + sprite2.width)) {
		return false;
	}

	if (sprite2.y >= (sprite1.y + sprite1.height)) {
		return false;
	}

	if (sprite1.y >= (sprite2.y + sprite2.height)) {
		return false;
	}

	return true;
}

// Debug Helpers

void SendDebug(char* string) {
	if (DebugMode == 1) {
		// Send all of the characters in the string
		unsigned char char_count = 0;
		while (*string != '\0') {
			usb_serial_putchar(*string);
			string++;
			char_count++;
		}

		// Go to a new line (force this to be the start of the line)
		usb_serial_putchar('\r');
		usb_serial_putchar('\n');
	}
}

void EnterBreakpoint(unsigned long line_num) {
	if (DebugMode == 1) {
		int16_t curr_char;
		char buff[85];
		sprintf(buff, "Entered breakpoint @ line %lu. Press b to continue...", line_num);
		SendDebug(buff);
		do {
			curr_char = usb_serial_getchar();
		} while (curr_char != 'b');
	}
}

// End Debug Helpers

// Initial Vars

int PlayerLives = 5;
int PlayerScore = 0;

// Initial Snake Vars
ListNode * SnakeLinkedList = NULL;
enum Directions SnakeDirection;

Sprite FoodPellet;

void initial_screen() {
	clear_screen();
	
	draw_centred(LCD_Y/3, "Cian O\'Leary");
	draw_centred(LCD_Y/1.5, "n9727442");

	show_screen();
}

void initialiseSnake() {
	// IniitialSnakeLength
	int SnakeCurrentLength = 0;
	SnakeLinkedList = NULL;

	while(SnakeCurrentLength != InitialSnakeLength) {
		Sprite firstNode = {SnakeCurrentLength*3, 0, 3, 3};
		push(&SnakeLinkedList, firstNode);
		SnakeCurrentLength++;
	}
}

void DrawHUD() {
	draw_string(30, 0, "L: ");
	draw_char(40, 0, (char)PlayerLives + '0');
    draw_string(0, 0, "S: ");
	draw_char(10, 0, (char)PlayerScore + '0');

	// For Debugging Direction
	draw_string(60, 0, "D: ");
	draw_char(70, 0, (char)SnakeDirection + '0');
	// End Debugging Direction
}

void DrawSnake() {
	ListNode *IterationTemp = SnakeLinkedList;
	ListNode *IterationTempnext = IterationTemp->next;

	while(IterationTempnext->next != NULL) {
		IterationTemp=IterationTempnext;
		IterationTempnext=IterationTemp->next;
		draw_sprite(&IterationTemp->val);
	}
}

void SnakeLoseLife() {
	PlayerLives = PlayerLives - 1;
	SendDebug("Snake Lost a life");
	SnakeDirection = IDLE;

	trimList(SnakeLinkedList, 0);
	initialiseSnake();

	clear_screen();
	DrawHUD();
	show_screen();

	_delay_ms(500);
}

bool collidesWithSnake(ListNode * head, Sprite TestCollision) {
	ListNode *IterationTemp = SnakeLinkedList;
	ListNode *IterationTempnext = IterationTemp->next;

	while(IterationTempnext->next != NULL) {
		IterationTemp=IterationTempnext;
		IterationTempnext=IterationTemp->next;
		if(hasCollided(IterationTemp->val, TestCollision)) {
			return true;
		}
	}
	return false;
}

void generateFood() {
	Sprite ValidTempFood = {(rand()%80), ((rand()%30)+6), 3, 3}; 
	
	while (collidesWithSnake(SnakeLinkedList, ValidTempFood)){
		Sprite TempFood = {(rand()%80), ((rand()%30)+6), 3, 3};
		ValidTempFood = TempFood;
	}

	char buff[20];
	sprintf(buff, "Pellet at: %8.2f, %8.2f", ValidTempFood.x, ValidTempFood.y);

	SendDebug(buff);
	FoodPellet = ValidTempFood;
}

void MoveSnake() {
	if(SnakeDirection == IDLE) { // Don't move the snake
		return;
	}

	deleteTrailing(SnakeLinkedList);
	Sprite SnakeHead = SnakeLinkedList->val;

	if (SnakeDirection == RIGHT) {
		SnakeHead.x = SnakeHead.x + 3;
	} else if (SnakeDirection == LEFT) {
		SnakeHead.x = SnakeHead.x - 3;
	} else if (SnakeDirection == UP) {
		SnakeHead.y = SnakeHead.y - 3;
	} else if (SnakeDirection == DOWN) {
		SnakeHead.y = SnakeHead.y + 3;
	}

	push(&SnakeLinkedList, SnakeHead);

	if(collidesWithSnake(SnakeLinkedList->next, SnakeLinkedList->val)) {
		SnakeLoseLife();
	}

	if(hasCollided(FoodPellet, SnakeLinkedList->val)) {
		PlayerScore++;
		generateFood();
	}
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
	
	initialiseSnake();
	generateFood();
}

// Interrupt that
// Checks for user input
ISR(TIMER0_OVF_vect) {
	// SW2
	if ((PINF >> 6) & 1){ // TODO For walls
	}

	// SW3
	if ((PINF >> 5) & 1){ // TODO For walls
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
	MoveSnake();
	DrawSnake();
	draw_sprite(&FoodPellet);
	show_screen();
}

int main(void) {
	initial_setup();
	if(DebugMode == 1) {
		usb_init();
		draw_centred(17, "Waiting for");
		draw_centred(24, "debugger...");
		show_screen();
		while(!usb_configured() || !usb_serial_get_control());
	}
	initial_screen();
	_delay_ms(2000);

	while (PlayerLives > 0) {
		_delay_ms(100);
		update();
	}

	clear_screen();	
	draw_centred(LCD_Y/3, "GAME OVER");
	show_screen();
	_delay_ms(2000);
}
