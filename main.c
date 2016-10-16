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
    struct ListNode *current = head, *t;
 

	for (count = 1; count< M && curr!= NULL; count++) { // Skip the safe ones
		current = current->next;
	}

	if (current == NULL) {
		return; // They are all safe.... For now
	}

    // The main loop that traverses through the whole list
    while (current != NULL) {
        // If we reached end of list, then return
        if (curr == NULL)
            return;
 
        // Start from next node and delete N nodes
        t = curr->next;
        for (count = 1; count<=N && t!= NULL; count++)
        {
            struct node *temp = t;
            t = t->next;
            free(temp);
        }
        curr->next = t; // Link the previous list with remaining nodes
 
        // Set current pointer for next iteration
        curr = t;
    }
}


// End Linked List implementation


// Initial Vars

unsigned char PlayerLives = 0;
ListNode * SnakeLinkedList = NULL;


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

	// Debugging Linked List
	int test = 0;

	while(test != 10) {
		Sprite firstNode = {test, 4, 4, 4};
		push(&SnakeLinkedList, firstNode);
		test++;
	}
	deleteTrailing(SnakeLinkedList);

	// End Debugging Linked List
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
