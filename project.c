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
    //Two values, one for occupied/open (0 means open) and one for time remaining (starts at 0)
    {0, 0},
    {0, 0},
    {0, 0}
};

// Function to display a timer value on the 7-segment display
void DisplayDigit(int value, int display) {
    // Calculate the hundredths value by getting the remainder of value divided by 10
    int hundSec = value % 10;
    // Calculate the tenths value by dividing value by 10 and then getting the remainder when divided by 10 again
    int sec = (value / 10) % 10;
    // Calculate the seconds value by dividing value by 100 and then getting the remainder when divided by 10
    int tensSec = (value / 100) % 10;
    
    // Check if the display value is 1
    if (display == 1) {
        // Set the first two digits of seg2_ptr to the tensSec and sec values
        *seg2_ptr = (hex[tensSec] << 8) | hex[sec];
        // Set the third digit of seg1_ptr to the hundSec value, preserving the other digits
        *seg1_ptr = ((*seg1_ptr) & 0xFF00FFFF) | (hex[hundSec] << 16);
    } else {
        // Set the first three digits of seg1_ptr to the hundSec, sec, and tensSec values
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

        /*This code snippet checks parking spot occupancy based on the values of the switches and updates 
        the elapsed time and timers. The code loops through the parking spots, calculates the elapsed ticks 
        since the last update, and updates the time remaining for each occupied parking spot.*/

        /* !-------------------------------------------------------------------------------------------------------!*/
        // Check parking spot occupancy based on switch values
        swVal = *switch_ptr; // Read the value of the switches
        int switchVal = ReadSwitch(); // Call ReadSwitch function to read switch values
        int vacant_spots = 0; // Initialize a variable to count vacant spots
        // Loop through the parking spots to determine occupancy
        for (int i = 0; i < 3; ++i) {
            // Set the occupied status of each parking spot based on the corresponding switch value
            parking_spots[i].occupied = (swVal & (1 << i)) != 0;
        }

        // Update elapsed time and timers
        int elapsed_ticks = last_tick - a9_ptr->counter; // Calculate the elapsed ticks since the last update
        // Check if elapsed_ticks is negative (counter wrapped around)
        if (elapsed_ticks < 0) {
            // Correct the elapsed_ticks value by adding the maximum timer value + 1
            elapsed_ticks += a9_ptr->load + 1;
        }
        // Check if elapsed_ticks is greater or equal to ticks_per_second
        if (elapsed_ticks >= ticks_per_second) {
            // Update last_tick with the current counter value
            last_tick = a9_ptr->counter;
            // Increment tick_count by 1
            tick_count++;

            // Check if tick_count is greater or equal to ticks_per_second
            if (tick_count >= ticks_per_second) {
                // Loop through the parking spots to update time_remaining
                for (int i = 0; i < 3; i++) {
                    // Check if the parking spot is occupied and time_remaining is greater than 0
                    if (parking_spots[i].occupied && parking_spots[i].time_remaining > 0) {
                        // Decrement time_remaining of the parking spot by 1
                        parking_spots[i].time_remaining -= 1;
                    }
                }
                // Reset tick_count to 0
                tick_count = 0;
            }
        }
        /* ^-------------------------------------------------------------------------------------------------------^*/

        /* This code snippet handles the push buttons to add coins to parking spots and switch the displayed timer. 
        It also updates the 7-segment display with the occupied/vacant status of the parking spots.*/

        /* !-------------------------------------------------------------------------------------------------------!*/
        // Handle push buttons 1-3 (add coin on release)
        pushVal = *push_ptr; // Read the value of the push buttons
        // Loop through the first 3 push buttons
        for (int i = 0; i < 3; ++i) {
            // Check if the push button was previously pressed and is now released
            if ((prev_coin_pushVal[i] & (1 << i)) && !(pushVal & (1 << i))) {
                // Add time to the corresponding parking spot when a coin is inserted
                parking_spots[i].time_remaining += 5 * 100;
            }
            // Update the previous state of the push button
            prev_coin_pushVal[i] = pushVal & (1 << i);
        }

        // Handle push button 4 (switch displayed timer on release)
        // Check if push button 4 was previously pressed and is now released
        if ((prev_pushVal & 0x08) && !(pushVal & 0x08)) {
            // Cycle through the parking spots to display
            displayed_spot = (displayed_spot + 1) % 3;
        }
        // Update the previous state of push button 4
        prev_pushVal = pushVal & 0x08;

        // Display the timer of the currently selected parking spot
        DisplayDigit(parking_spots[displayed_spot].time_remaining, 1);

        // Display occupied/vacant status on the first 3 7-Segment displays
        int occupied_display = 0; // Initialize a variable to hold the display value
        // Loop through the parking spots to generate the display value
        for (int i = 0; i < 3; i++) {
            // Check if the parking spot is occupied
            if (parking_spots[i].occupied) {
                // Set the corresponding 7-segment display value to 1 (occupied)
                occupied_display |= (hex[1] << (i * 8));
            } else {
                // Set the corresponding 7-segment display value to 0 (vacant)
                occupied_display |= (hex[0] << (i * 8));
            }
        }
        // Update the 7-segment display with the occupied/vacant status
        *seg1_ptr = occupied_display;
        /* ^-------------------------------------------------------------------------------------------------------^*/

        /*This code snippet updates the 7-segment display with the timer value of the currently selected parking spot. 
        It also lights up an LED corresponding to the displayed timer and counts the number of unoccupied spots when 
        the fourth switch is turned on.*/

        /* !-------------------------------------------------------------------------------------------------------!*/
        int timer_display = 0; // Initialize a variable to hold the timer display value
        int value = parking_spots[displayed_spot].time_remaining; // Get the time remaining for the displayed spot
        int hundSec = value % 10; // Calculate the hundredths place of the time remaining
        int sec = (value / 100) % 10; // Calculate the seconds place of the time remaining
        int tensSec = (value / 1000) % 6; // Calculate the tens of seconds place of the time remaining
        // Combine the hundredths, seconds, and tens of seconds place values into the timer display value
        timer_display |= (hex[tensSec] << 8) | hex[sec] | (hex[hundSec] << 16);
        *seg2_ptr = timer_display; // Update the 7-segment display with the timer value

        // Light up an LED corresponding to the timer being displayed
        *ledr_ptr = 1 << displayed_spot;

        // Read switch status and count the number of switches off (unoccupied spots)
        if (swVal & 0x08) { // Check if the fourth switch is turned on
            for (int i = 0; i < 3; i++) { // Loop through the first 3 switches
                if (!(switchVal & (1 << i))) { // Check if the switch is off (unoccupied)
                    vacant_spots++; // Increment the count of vacant spots
                }
            }
            DisplayNum(vacant_spots); // Display the number of vacant spots on the 7-segment display
        }

        /* ^-------------------------------------------------------------------------------------------------------^*/

        // Delay loop
        for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count);
    }

    // Exit the program (unreachable)
    return 0;
}

