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
void startTimer(void){
	//the timer has started set the status register to 1
	a9_ptr -> control = 3;
	
}

//function that stops the timer
void stopTimer(){
	//the timer has been paused set the control register to zero
	a9_ptr -> control = 0;
}

int main(void){
	//initialize some start values
	a9_ptr -> control = 0b0000; //initialize the control to not run
     int delay_count=0; // volatile so the C compiler doesn't remove the loop
	 int DELAY_LENGTH = 70000;//700000

	//increment the load so that the timer goes at a good pace
	a9_ptr -> load = 2000000;
	        
	while(1){
		int swVal = ReadSwitch();
		// read switch status and set 7-seg display 1 accordingly
		if (swVal & 0x1) {
			one = occupied; // display '1' on 7-seg display 1
		} 
		else {
			one = vacant; // display '0' on 7-seg display 1
		}

		// read switch status and set 7-seg display 2 accordingly
		if (swVal & 0x2) {
			two = occupied; // display '1' on 7-seg display 2
		} 
		else {
			two = vacant; // display '0' on 7-seg display 2
		}
		// read switch status and set 7-seg display 3 accordingly
		if (swVal & 0x4) {
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
