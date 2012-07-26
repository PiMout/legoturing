
// Global that is always set to display
int display = 1337;
// Global used to return values from functions
int retval = 0;


void pullup(){
	// Cog sensor
	SetSensor(SENSOR_1, SENSOR_LIGHT);
	// Read head position, if 0 = up.
	SetSensor(SENSOR_2, SENSOR_LIGHT);
	// End of write sensors
	SetSensor(SENSOR_3, SENSOR_LIGHT);

	// Use our own global variable for output	
	SelectDisplay(DISPLAY_USER);
	SetUserDisplay(display,0);
}


#define MAIN_MOTOR OUT_A


/* -------------------- [ Low-level movement API ] ---------------- */
/* Move for a given time period at full power */
void move(int time){
	SetPower(MAIN_MOTOR, OUT_FULL);
	SetOutput(MAIN_MOTOR, OUT_ON);

	Wait(time);

	SetOutput(MAIN_MOTOR, OUT_OFF);
}

/* Move forward for a given time. */
void forward(int time){
	SetDirection(MAIN_MOTOR, OUT_FWD);
	move(time);
}

void forward_nostop(){
	SetDirection(MAIN_MOTOR, OUT_FWD);
	SetOutput(MAIN_MOTOR, OUT_ON);
}

void stop_moving(){
	SetOutput(MAIN_MOTOR, OUT_OFF);
}

/* Move backward for a given time */
void backward(int time){
	SetDirection(MAIN_MOTOR, OUT_REV);
	move(time);
}

void backward_nostop(){
	SetDirection(MAIN_MOTOR, OUT_REV);
	SetOutput(MAIN_MOTOR, OUT_ON);
}


/* -------------------- [ Turing-level movement API ] ---------------- */

/* Move some tape slots in some direction. 

slots: the number of whole slots to move.
direction: >0 for forwards, 0 for backwards
*/
// Above this threshold, the light sensor is ON, else off.
#define LIGHT_THRESHOLD 48
// FIXME: Make this continuous, if the drive is slow enough
void move_slots(int slots, int direction){
	release_sensor();
	int light = 0;
	int transitions = 0;
	int state = 0; // >0 for "on"/black, 0 for off/white.

	// Check if we are starting in the "on" position
	light = SensorValue(0);
	if(light < LIGHT_THRESHOLD)
		state = 1;

	// Move onto next thing
	if(direction > 0)
		forward_nostop();
	else
		backward_nostop();
	
	SetUserDisplay(display, 2);
	// Monitor progress
	while(true){
		light = SensorValue(0); // must be numeric, constants don't work

		// white, moving to black, turn state ON
		if(state == 0){
			if(light < LIGHT_THRESHOLD){
				state = 1;
				transitions += 1;
			}
		}else{
			// Black moving to white, turn state OFF
			if(light > LIGHT_THRESHOLD){
				state = 0;
				transitions += 1;
			}	
		}

		// update the display with distance travelled
		display = ((transitions/2) * 100) + slots;

	
		if((transitions/2) >= slots){
			stop_moving();
			PlaySound(SOUND_CLICK);
			return;
		}

	}
	SetUserDisplay(display, 0);
}

/* Move left by one position on the tape. */
void left(){
	left_n(1);
}

/* Move right by one position on the tape */
void right(){
	right_n(1);
}

/* Move left n tape slots.*/
void left_n(int slots){
	move_slots(slots, 1);
}

/* Move right n tape slots */
void right_n(int slots){
	move_slots(slots, 0);
}



/* -------------------- [ low-level Write Head API ] ---------------- */

#define WRITE_POSITION_SENSOR_THRESHOLD 90

// Moves the write head to position 1 or 0.
// position 1 FORCES a one.
// position 0 allows ONE OR ZERO on the tape
void activate_write(int direction){
	// Decide on the direction of motion
	if(direction == 0){ 
		/*PlaySound(SOUND_UP);*/
		SetDirection(OUT_C, OUT_FWD);
	}else{
		SetDirection(OUT_C, OUT_REV);
		/*PlaySound(SOUND_DOWN);*/
	}

	// Read the first value of the end sensor
	int at_end = 0;
   	if(direction == 0)
		at_end = SensorValue(2); // 0 is our "off" sensor"
	else
		at_end = SensorValue(0); // 2 is our "on" sensor

	// Actually move until we hit our directional sensor
	SetPower(OUT_C, OUT_FULL);
	SetOutput(OUT_C, OUT_ON);
	while(at_end < WRITE_POSITION_SENSOR_THRESHOLD){
		// Update the end sensor state
		if(direction == 0)
			at_end = SensorValue(2); // 0 is our "off" sensor
		else
			at_end = SensorValue(0); // 2 is our "on" sensor

	}
	SetOutput(OUT_C, OUT_OFF);

}

// Ensure the sensor is not overriding the light sensor input used for cogging.
void release_sensor(){
	int at_end = SensorValue(0);
	if(at_end > WRITE_POSITION_SENSOR_THRESHOLD){
		reset_write_head();
		raise_arm();
	}
}


// Reset the head
void reset_write_head(){
	activate_write(0);
}

// Write a one to the tape
void write_on(){
	reset_write_head();
	activate_write(1);
	reset_write_head();
}



/* -------------------- [ Read head API ] ---------------- */

// Read head is 3 sensors:
// Sensor 2 is value of light sensor ( up if == 0)'
// Motor B controls read head


#define READ_SENSOR_TIME_TO_LOWER 380
#define READ_SENSOR_TIME_TO_WRITE 40

// Above this, we think we have seen a block
#define READ_SENSOR_DETECT_THRESHOLD 50
// The delay between seeing a block and seeing the end of the arm swing for it to count as "on"
#define READ_SENSOR_SENSITVITY_DELAY 30
//Above this, we think the arm is up since the switch is shorting the sensor
#define READ_SENSOR_UP_THRESHOLD 90

void raise_arm(){
	// Read the arm position
	int at_end = SensorValue(1);

	// Run the motor to raise the arm
	SetPower(OUT_B, OUT_FULL);
	SetDirection(OUT_B, OUT_REV);
	SetOutput(OUT_B, OUT_ON);
	// Raise the arm until we breach the sensor threshold
	// ie the button is triggered
	while(at_end < READ_SENSOR_UP_THRESHOLD){
		at_end = SensorValue(1);   // drive motor 
	}
	SetOutput(OUT_B, OUT_OFF);
}

void lower_arm(){
	release_sensor();
	// Read the arm position
	int at_end = SensorValue(0); // use multi-use sensor 0

	// Run the motor to raise the arm
	SetPower(OUT_B, OUT_FULL);
	SetDirection(OUT_B, OUT_FWD);
	SetOutput(OUT_B, OUT_ON);
	// Raise the arm until we breach the sensor threshold
	// ie the button is triggered
	while(at_end < READ_SENSOR_UP_THRESHOLD){
		at_end = SensorValue(0);   // drive motor 
	}
	SetOutput(OUT_B, OUT_OFF);
}


// Lowers the arm, all the time checking to see
// if a block is there.
//
// If the block is seen before the arm is all the way down, 
// it beeps.
void read(){
	release_sensor(); // release the arm-down sensor (0);
	
	// Read the arm position
	int at_end = SensorValue(0); // use multi-use sensor 0
	int light_levels = 0; // light sensor and top button, only sense when moving
	int sensitivity_delay = READ_SENSOR_SENSITVITY_DELAY;

	// Run the motor to raise the arm
	SetPower(OUT_B, OUT_FULL);
	SetDirection(OUT_B, OUT_FWD);
	SetOutput(OUT_B, OUT_ON);
	// Raise the arm until we breach the sensor threshold
	// ie the button is triggered
	while(at_end < READ_SENSOR_UP_THRESHOLD && sensitivity_delay > 0){
		at_end = SensorValue(0);   // bottom stop
		light_levels = SensorValue(1); // light sensor
		// we are moving down, so wish to ignore if the arm is all the way up
		if(light_levels > READ_SENSOR_UP_THRESHOLD)
			light_levels = 0;
		else if(light_levels > READ_SENSOR_DETECT_THRESHOLD)
			sensitivity_delay --;
	}
	SetOutput(OUT_B, OUT_OFF);

	if(sensitivity_delay <= 0){
		//We detected a block
		PlaySound(SOUND_UP);
		raise_arm();
		write_on(); // make up for the damage we did
		retval = 1;
	}else{
		// We hit cancel before detection completed
		PlaySound(SOUND_DOWN);
		retval = 0;
		raise_arm();
	}
}


// Weirdly, this is done by the read head.
// so, well, lego is not easy to make nice mechanisms in.
void write_off(){
	lower_arm();
	raise_arm();
}




/* -------------------- [ High-level, combined APIs ] ---------------- */


void write(int data){
	if(data > 0)
		write_on();
	else
		write_off();
}

void zero_tape(){
	while(true){
		left();
		write(0);
	}
}
