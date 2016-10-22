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
// #include "usb_serial.h"

#define DebugMode 0
#define InitialSnakeLength 2

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
				1 // We don't need any bitmaps
			);
		}
	}
}

// Lets implement a linked list for my sanity reasons
// In the real world I would build this into a library or more likely use an existing library 
// and import it but this is university
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

void draw_centred(unsigned char y, char* string) { // Thanks random Week tutorial work
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

// Debug Helpers - Disabled for final submission

void SendDebug(char* string) {
	// Disabled for Submittion
	// if (DebugMode == 1) {
	// 	// Send all of the characters in the string
	// 	unsigned char char_count = 0;
	// 	while (*string != '\0') {
	// 		usb_serial_putchar(*string);
	// 		string++;
	// 		char_count++;
	// 	}

	// 	// Go to a new line (force this to be the start of the line)
	// 	usb_serial_putchar('\r');
	// 	usb_serial_putchar('\n');
	// }
}

void EnterBreakpoint(unsigned long line_num) {
	// Disabled for Submition
	// if (DebugMode == 1) {
	// 	int16_t curr_char;
	// 	char buff[85];
	// 	sprintf(buff, "Entered breakpoint @ line %lu. Press b to continue...", line_num);
	// 	SendDebug(buff);
	// 	do {
	// 		curr_char = usb_serial_getchar();
	// 	} while (curr_char != 'b');
	// }
}

// End Debug Helpers

// Initial Vars

int PlayerLives = 5;
int PlayerScore = 0;
bool wallsVisible = false;
Sprite WallsArray[3];

// Initial Snake Vars
ListNode * SnakeLinkedList = NULL; // Linked List containing the snake
enum Directions SnakeDirection;

Sprite FoodPellet;

void drawInitialScreen() {
	clear_screen();
	
	draw_centred(LCD_Y/3, "Cian O\'Leary");
	draw_centred(LCD_Y/1.5, "n9727442");

	show_screen();
}

void initialiseSnake() {
	// IniitialSnakeLength
	int SnakeCurrentLength = 0;
	SnakeLinkedList = NULL;

	while(SnakeCurrentLength <= (1 + InitialSnakeLength)) {
		Sprite firstNode = {SnakeCurrentLength*3 + 30, 15, 3, 3};
		push(&SnakeLinkedList, firstNode);
		SnakeCurrentLength++;
	}
}

void initialiseWalls() {
	Sprite Wall1 = {10, 0, 1, 11};
	Sprite Wall2 = {30, 22, 1, 16};
	Sprite Wall3 = {60, 0, 1, 22};

	WallsArray[0] = Wall1;
	WallsArray[1] = Wall2;
	WallsArray[2] = Wall3;
}

void DrawHUD() {
	draw_string(0, 0, "L: ");
	draw_char(10, 0, (char)PlayerLives + '0');
    draw_string(30, 0, "S: ");

	char buff[6];
	// 2.777 hours of continuous play under perfect conditions where the food is always in the way next cycle
	sprintf(buff, "%d", PlayerScore);
	draw_string(40, 0, buff);
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

void DrawWalls() {
	int i;   
	int arraySize = sizeof(WallsArray)/sizeof(Sprite); /* arraySize <-- 1<-- 4/4 */
	for (i = 0; i < arraySize; i++) {  
		draw_sprite(&WallsArray[i]);
	}
}

void SnakeLoseLife() {
	PlayerLives = PlayerLives - 1;
	SendDebug("Snake Lost a life");

	trimList(SnakeLinkedList, 0);
	initialiseSnake();

	clear_screen();
	DrawHUD();
	show_screen();

	_delay_ms(500);
	SnakeDirection = IDLE;
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

bool collidesWithWall(Sprite TestSprite) {
	int i;   
	int arraySize = sizeof(WallsArray)/sizeof(Sprite); /* arraySize <-- 1<-- 4/4 */
	for (i = 0; i < arraySize; i++) {  
		if (hasCollided(WallsArray[i], TestSprite)) {
			return true;
		}
	}
	return false;
}

void generateFood() {
	Sprite ValidTempFood = {(rand()%80), ((rand()%30)+6), 3, 3}; 
	
	while (collidesWithSnake(SnakeLinkedList, ValidTempFood) || collidesWithWall(ValidTempFood)){
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

	Sprite SnakeHead = SnakeLinkedList->val;

	if (SnakeDirection == RIGHT) {
		SnakeHead.x = SnakeHead.x + 3;
		if(SnakeHead.x >= 84) {
			SnakeHead.x = 0;
		}
	} else if (SnakeDirection == LEFT) {
		SnakeHead.x = SnakeHead.x - 3;
		if(SnakeHead.x < 0) {
			SnakeHead.x = 84;
		}
	} else if (SnakeDirection == UP) {
		SnakeHead.y = SnakeHead.y - 3;
		if(SnakeHead.y < 0) {
			SnakeHead.y = 40;
		}
	} else if (SnakeDirection == DOWN) {
		SnakeHead.y = SnakeHead.y + 3;
		if(SnakeHead.y >= 38) {
			SnakeHead.y = 0;
		}
	}

	push(&SnakeLinkedList, SnakeHead);

	if(collidesWithSnake(SnakeLinkedList->next, SnakeLinkedList->val)) {
		SnakeLoseLife();
		return;
	}

	if(wallsVisible && collidesWithWall(SnakeLinkedList->val)) {
		SnakeLoseLife();
		return;
	}

	if(hasCollided(FoodPellet, SnakeLinkedList->val)) {
		if (wallsVisible == false) {
			PlayerScore = PlayerScore + 1;
		} else {
			PlayerScore = PlayerScore + 2;
		}
		generateFood();
	} else {
		deleteTrailing(SnakeLinkedList);
	}
}

// initialize adc
void adc_init() {
    // AREF = AVcc
    ADMUX = (1<<REFS0);
 
    // ADC Enable and pre-scaler of 128
    // 8000000/128 = 62500
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with '7' will always keep the value
    // of 'ch' between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing
 
    // start single conversion
    // write '1' to ADSC
    ADCSRA |= (1<<ADSC);
 
    // wait for conversion to complete
    // ADSC becomes '0' again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));
 
    return (ADC);
}

void initial_setup() {
	set_clock_speed(CPU_8MHz);

	DDRB = 0b01111100;
	DDRF = 0b10011111;
	DDRD = 0b11111100;

	lcd_init(LCD_DEFAULT_CONTRAST);
	adc_init();

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
	if ((PINF >> 6) & 1){
		if (wallsVisible) {
			wallsVisible = false;
			SendDebug("Walls hidden");
		}
	}

	// SW3
	if ((PINF >> 5) & 1){
		if (wallsVisible == false) {
			wallsVisible = true;
			SendDebug("Walls Show");
		}
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
	if (wallsVisible) {
		DrawWalls();
	}
	show_screen();
}

int main(void) {
	initial_setup();
	// Disabled for Submittion
	// if(DebugMode == 1) {
	// 	usb_init();
	// 	draw_centred(17, "Waiting for");
	// 	draw_centred(24, "debugger...");
	// 	show_screen();
	// 	while(!usb_configured() || !usb_serial_get_control());
	// }
	initialiseWalls();
	drawInitialScreen();
	_delay_ms(2000);

	while (PlayerLives > 0) {
		_delay_ms(50);
		if (adc_read(0) > 700) {
			_delay_ms(70);
		} else if (adc_read(0) > 550) {
			_delay_ms(55);
		} else if (adc_read(0) > 400) {
			_delay_ms(35);
		} else if (adc_read(0) > 200) {
			_delay_ms(20);
		} else {
			_delay_ms(5);
		}

		update();
	}

	clear_screen();	
	draw_centred(LCD_Y/3, "GAME OVER");
	show_screen();
	_delay_ms(2000);
}
