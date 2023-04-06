#define HEX3_HEX0_BASE 0xFF200020 //first 4 segs
#define HEX5_HEX4_BASE 0xFF200030//last two segs
#define SWITCH_BASE 0xFF200040//switches
#define PUSH_BASE 0xFF200050//push buttons
#define PRIVATE_TIMER_BASE 0xFFFEC600 //private timer

//timer "settings"
typedef struct{
	int load;
	int count;
	int control;
	int status;
}a9_timer;
	
volatile int *seg1_ptr = (int *)HEX3_HEX0_BASE;
volatile int *seg2_ptr = (int *)HEX5_HEX4_BASE;
volatile int* const switch_ptr = (int*) SWITCH_BASE;
volatile int* const push_ptr = (int*) PUSH_BASE;
volatile a9_timer* const a9_ptr = (a9_timer*) PRIVATE_TIMER_BASE;
int one, two, three;
int DELAY_LENGTH;
int occupied = 0x06; // 7-segment display value for "occupied" (red)
int vacant = 0x3F; // 7-segment display value for "vacant" (green)
int timer0Running = 0;
int hex[10] = {0b00111111, 0b0000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111100, 0b0000111, 0b01111111, 0b01101111};




int getPush(void){
	//create a value that will hold which push button is pressed
	int pushVal = *push_ptr;
	
	//return the push value
	return pushVal;
	
}

int ReadSwitch(void){
	//get the value in the switch
	int swVal = *switch_ptr;
	
	//return the switch value
	return swVal;
}

//function that increments the value in display digit
void StartTimer(int load) {
    a9_ptr -> control = 3;
}

//function that stops the timer
void stopTimer(){
	//the timer has been paused set the control register to zero
	a9_ptr -> control = 0;
}


void DisplayDigit(int value){
           //creating variables for the timer
           int hundSec = value%10;
           int tenthSec = (value/10)%10;
           int sec = (value/100)%10;
           int tensSec = (value/1000)%6;
           int min = (value/6000)%10;
           int tensMin = (value/60000)%6;
           
           //adding the timer to integers that will be stored in the seven segment display
        //    int seg1 = hex[hundSec] + (hex[tenthSec]<<8) + (hex[sec]<<16) + (hex[tensSec]<<24);
		           int seg1 = (hex[tenthSec]<<8) ;
           		int seg2 = hex[min] + (hex[tensMin]<<8);
           
           
           //setting the seven segment display addresses to the values in the integers
           *seg1_ptr = seg1;
           *(seg2_ptr) = (seg2);
           
}       


int main(void){
	a9_ptr -> control = 0b0000; //initialize the control to not run
    int delay_count=0; // volatile so the C compiler doesn't remove the loop
    int DELAY_LENGTH = 70000;//700000
    int timer0Running = 0;
	int swVal = ReadSwitch();
	int pushVal = getPush();
	int load = 2000000; 
	a9_ptr -> load = 2000000;

	
    while (1) {
        int swVal = ReadSwitch();

        // check if switch 0 is flipped and timer is not running
        if (swVal & 0x01 && !timer0Running) {
            StartTimer(load); // start the timer
            timer0Running = 1;
			 // update the display with the elapsed time
        }

        // check if switch 0 is flipped back to original position and timer is running
        if (!(swVal & 0x01) && timer0Running) {
            stopTimer(); // stop the timer
            timer0Running = 0;
        }
		// check if switch 1 is flipped and timer is not running
		if (swVal & 0x02 && !timer0Running) {
   			 StartTimer(load); // start the timer
    		timer0Running = 1;
			}

		// check if switch 1 is flipped back to original position and timer is running
		if (!(swVal & 0x02) && timer0Running) {
    		stopTimer(); // stop the timer
    		timer0Running = 0;
			}
		if (swVal & 0x04 && !timer0Running) {
            StartTimer(load); // start the timer
            timer0Running = 1;
        }

        // check if switch 0 is flipped back to original position and timer is running
        if (!(swVal & 0x04) && timer0Running) {
            stopTimer(); // stop the timer
            timer0Running = 0;
        }

          int elapsed_time = a9_ptr -> count / 100; // convert count value to hundredths of a second
        DisplayDigit(elapsed_time);

    
		int switchVal = ReadSwitch();
		// read switch status and set 7-seg display 1 accordingly
		if (switchVal & 0x1) {
			one = occupied; // display '1' on 7-seg display 1
		} 
		else {
			one = vacant; // display '0' on 7-seg display 1
		}

		// read switch status and set 7-seg display 2 accordingly
		if (switchVal & 0x2) {
			two = occupied; // display '1' on 7-seg display 2
		} 
		else {
			two = vacant; // display '0' on 7-seg display 2
		}
		// read switch status and set 7-seg display 3 accordingly
		if (switchVal & 0x4) {
			three = occupied; // display '1' on 7-seg display 3
		} 
		else {
			three = vacant; // display '0' on 7-seg display 3
		}
		*seg1_ptr = one + (two<<8)+ (three<<16);

		for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count); // delay loop
	}
	return 0;
}
