#include "turing.c"

// Left, right, and a global to stop execution
#define R 1
#define L 0

int t_halt = 0;
int t_read = 0;
int t_write = 0;
int t_move = R;

/* ----------------- [ Turing Machine Unary addition ] ---------------- */

// IMPORTANT: presumes the read value is in retval.
void turing(int wr, int mov){
	// record the read state.  Allows for optimisations
	t_read = retval;

	t_write = wr;
	t_move = mov;
}

/* States, named after the JFLap file in doc/ */
void q0(){
	read();
	if(retval == 0){
		turing( 0, R );
		retval = 0;
	}else{
		turing( 0 , R);
		retval = 1;
	}
}


void q1(){
	read();
	if(retval == 0){
		turing( 1, L );
		retval = 2;
	}else{
		turing( 1 , R);
		retval = 1;
	}
}


void q2(){
	read();
	if(retval == 0){
		turing( 1, R );
		retval = 3;
	}else{
		turing( 0 , R);
		retval = 4;
	}
}


void q3(){
	read();
	if(retval == 0){
		turing( 0, R );
		retval = 6;
	}else{
		turing( 1 , R);
		retval = 3;
	}
}


void q4(){
	read();
	if(retval == 0){
		turing( 1, L );
		retval = 5;
	}else{
		turing( 1 , R);
		retval = 4;
	}
}


void q5(){
	read();
	if(retval == 0){
		turing( 1, L );
		retval = 2;
	}else{
		turing( 1 , L);
		retval = 5;
	}
}

// Halt state
void q6(){
	t_halt = 1;
	PlaySound(SOUND_UP);
}




task main ()
{
	// Main drive motor and write head motor
	acquire(ACQUIRE_OUT_A + ACQUIRE_OUT_B){


		// Find beginning of tape
		//left_n(1);
		//right_n(1);


		// Start at q0
		retval = 0;
		
		while(t_halt == 0){
			switch(retval){
				case 0:
					q0();
					break;
				case 1:
					q1();
					break;
				case 2:
					q2();
					break;
				case 3:
					q3();
					break;
				case 4:
					q4();
					break;
				case 5:
					q5();
					break;
				default:
					q6();
			}


			// Optimisation to prevent writing when
			// already in correct state
			if(t_read != t_write)
				write(t_write);

			// Move.
			if(t_move == R)
				right();
			else
				left();
		}
	

		// Don't do anyting else.


	}
}




/* -------------------- [ Entry Point ] ---------------- */
/*
 *task main ()
 *{
 *    // Main drive motor and write head motor
 *    acquire(ACQUIRE_OUT_A + ACQUIRE_OUT_B){
 *
 *    // Start doing stuff
 *    PlaySound(SOUND_CLICK);
 *
 *
 *    [>move(200);<]
 *
 *    
 *    [> Seek left by 5 slots, then right by 5 <]
 *    left_n(5);
 *    right_n(5);
 *    
 *    [> Raise and lower the read head. <]
 * //	raise_arm();
 * //	lower_arm();
 * //	raise_arm();
 *    
 *    reset_write_head();
 *    [>left(); <]
 *    read();
 *    write_off();
 *    read();
 *    write_on();
 *    read();
 *    
 *    
 *    [>zero_tape();<]
 *    
 *    [>write_one();<]
 *            
 *
 *    PlaySound(SOUND_CLICK);
 *    }
 *
 *}
 */

