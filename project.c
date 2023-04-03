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
	
//variables that point to the objects
volatile int* const hex1_ptr = (int*) HEX3_HEX0_BASE; 
//volatile int* const hex2_ptr = (int*) HEX5_HEX4_BASE;
volatile int* const switch_ptr = (int*) SWITCH_BASE;
volatile int* const push_ptr = (int*) PUSH_BASE;
volatile a9_timer* const a9_ptr = (a9_timer*) PRIVATE_TIMER_BASE;
    int car1_present, car2_present, car3_present;
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

	// Read the state of the ultrasonic sensors and switches
	car1_present = (*switch_ptr & 0x1); // Switch 1
	car2_present = (*switch_ptr & 0x2);// Switch 2
	car3_present = (*switch_ptr & 0x4); // Switch 3

	// Update the seven-segment displays based on the sensor readings
	hex1_ptr[0] = car1_present ? occupied : vacant; // Display "occupied" if car1 is present, else "vacant"
	hex1_ptr[1] = car2_present ? occupied : vacant; // Display "occupied" if car2 is present, else "vacant"
	hex1_ptr[2] = car3_present ? occupied : vacant; // Display "occupied" if car3 is present, else "vacant"
	
	for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count); // delay loop
	}
}
