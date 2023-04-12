// Include necessary header files
#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"
#include "sys/alt_irq.h"
#include "priv/alt_legacy_irq.h"

// Define base addresses for peripherals
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SWITCH_BASE 0xFF200040
#define PUSH_BASE 0xFF200050
#define LEDR_BASE 0xFF200000
#define PRIVATE_TIMER_BASE 0xFFFEC600

// Create pointers to memory-mapped registers
volatile int *seg1_ptr = (int *)HEX3_HEX0_BASE;
volatile int *seg2_ptr = (int *)HEX5_HEX4_BASE;
volatile int *switch_ptr = (int *)SWITCH_BASE;
volatile int *push_ptr = (int *)PUSH_BASE;
volatile int *ledr_ptr = (int *)LEDR_BASE;

// Define hex codes for displaying numbers 0-9 on the 7-segment display
int hex[10] = {0b00111111, 0b0000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111100, 0b0000111, 0b01111111, 0b01101111};

// Define a9_private_timer struct to access the ARM private timer registers
typedef struct {
    int load;
    int counter;
    int control;
    int status;
} a9_private_timer;

// Create pointer to the ARM private timer
volatile a9_private_timer* const a9_ptr = (a9_private_timer*) PRIVATE_TIMER_BASE;

// Define a struct for managing parking spot information
struct ParkingSpot {
    int occupied;
    int time_remaining;
};

// Initialize an array of parking spots
struct ParkingSpot parking_spots[3] = {
    {0, 0},
    {0, 0},
    {0, 0}
};

// Function to display a timer value on the 7-segment display
void DisplayDigit(int value, int display) {
    // Initialize values to hold onto hundredths, tenths, and second values for timer
    int hundSec = value % 10;
    int sec = (value / 10) % 10;
    int tensSec = (value / 100) % 10;
    
    // Logic to chaneg the display values
    if (display == 1) {
        *seg2_ptr = (hex[tensSec] << 8) | hex[sec];
        *seg1_ptr = ((*seg1_ptr) & 0xFF00FFFF) | (hex[hundSec] << 16);
    } else {
        *seg1_ptr = (hex[hundSec] << 16) | (hex[sec] << 8) | hex[tensSec];
    }
}

// Function to read the current value of the switches
int ReadSwitch(void) {
    // get the value in the switch
    int swVal = *switch_ptr;
     // return the switch value
    return swVal;
}

// Function to display the number of vacant parking spots on the first 7-segment display
void DisplayNum(int value) {
     // creating variables for the timer
    int units = value;
    // setting the seven segment display addresses to the values in the integers
    *(seg1_ptr) = hex[units]<<24;
}

// Main function
int main(void) {
    // Initialize the private timer
    a9_ptr->control = 0b0000;
    a9_ptr->load = 0xFFFFFFFF;
    a9_ptr->control = 0b0101;

       // Initialize variables for the main loop
    int delay_count = 0;
    int DELAY_LENGTH = 20000;
    int swVal, pushVal, displayed_spot = 0;
    int prev_pushVal = 0;
    int prev_coin_pushVal[3] = {0, 0, 0};

    int last_tick = a9_ptr->counter;
    int ticks_per_second = 2000000000;
    int tick_count = 0;

    // Main loop
    while (1) {
        // Check parking spot occupancy based on switch values
        swVal = *switch_ptr;
        int switchVal = ReadSwitch();
        int vacant_spots = 0;
        for (int i = 0; i < 3; ++i) {
            parking_spots[i].occupied = (swVal & (1 << i)) != 0;
        }

        // Update elapsed time and timers
        int elapsed_ticks = last_tick - a9_ptr->counter;
        if (elapsed_ticks < 0) {
            elapsed_ticks += a9_ptr->load + 1;
        }
        if (elapsed_ticks >= ticks_per_second) {
            last_tick = a9_ptr->counter;
            tick_count++;

            if (tick_count >= ticks_per_second) {
                for (int i = 0; i < 3; i++) {
                    if (parking_spots[i].occupied && parking_spots[i].time_remaining > 0) {
                        parking_spots[i].time_remaining -= 1;
                    }
                }
                tick_count = 0;
            }
        }

        // Handle push buttons 1-3 (add coin on release)
        pushVal = *push_ptr;
        for (int i = 0; i < 3; ++i) {
            if ((prev_coin_pushVal[i] & (1 << i)) && !(pushVal & (1 << i))) {
                parking_spots[i].time_remaining += 5 * 100;
            }
            prev_coin_pushVal[i] = pushVal & (1 << i);
        }

        // Handle push button 4 (switch displayed timer on release)
        if ((prev_pushVal & 0x08) && !(pushVal & 0x08)) {
            displayed_spot = (displayed_spot + 1) % 3;
        }
        prev_pushVal = pushVal & 0x08;

        // Display the timer of the currently selected parking spot
        DisplayDigit(parking_spots[displayed_spot].time_remaining, 1);

        // Display occupied/vacant status on the first 3 7-Segment displays
        int occupied_display = 0;
        for (int i = 0; i < 3; i++) {
            if (parking_spots[i].occupied) {
                occupied_display |= (hex[1] << (i * 8));
            } else {
                occupied_display |= (hex[0] << (i * 8));
            }
        }
        *seg1_ptr = occupied_display;

                // Display the timer of the currently selected parking spot on 7-Segment displays 4-6
        int timer_display = 0;
        int value = parking_spots[displayed_spot].time_remaining;
        int hundSec = value % 10;
        int sec = (value / 100) % 10;
        int tensSec = (value / 1000) % 6;
        timer_display |= (hex[tensSec] << 8) | hex[sec] | (hex[hundSec] << 16);
        *seg2_ptr = timer_display;

        // Light up an LED corresponding to the timer being displayed
        *ledr_ptr = 1 << displayed_spot;

        // Read switch status and count the number of switches off (unoccupied spots)
        if (swVal & 0x08) {
            for (int i = 0; i < 3; i++) {
                if (!(switchVal & (1 << i))) {
                    vacant_spots++;
                }
            }
            DisplayNum(vacant_spots);
        }

        // Delay loop
        for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count);
    }

    // Exit the program (unreachable)
    return 0;
}

