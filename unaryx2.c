
#include "turing.c"

// Left, right, and a global to stop execution
#define R 1
#define L 0
#define S 2

int t_halt = 0;
int t_read = 0;
int t_write = 0;
int t_move = R;

// IMPORTANT: presumes the read value is in retval.
void turing(int wr, int mov){
  // record the read state.  Allows for optimisations
  t_read = retval;

  t_write = wr;
  t_move = mov;
}

void error(){
  t_halt = 1;
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_DOWN);
}//end error

void s_q0(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 1:
    turing(0, R);
    retval = 1;
    break;

  case 0:
    turing(0, R);
    retval = 0;
    break;

  default:
    retval = -1;
  } // end case

} // end state q0

void s_q1(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 0:
    turing(1, L);
    retval = 2;
    break;

  case 1:
    turing(1, R);
    retval = 1;
    break;

  default:
    retval = -1;
  } // end case

} // end state q1

void s_q2(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 0:
    turing(1, R);
    retval = 3;
    break;

  case 1:
    turing(0, R);
    retval = 4;
    break;

  default:
    retval = -1;
  } // end case

} // end state q2

void s_q3(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 1:
    turing(1, R);
    retval = 3;
    break;

  case 0:
    turing(0, S);
    retval = 6;
    break;

  default:
    retval = -1;
  } // end case

} // end state q3

void s_q4(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 0:
    turing(1, L);
    retval = 5;
    break;

  case 1:
    turing(1, R);
    retval = 4;
    break;

  default:
    retval = -1;
  } // end case

} // end state q4

void s_q5(){
	//read();  now done in the while to save bytecode size

  switch(retval){

  case 1:
    turing(1, L);
    retval = 5;
    break;

  case 0:
    turing(1, L);
    retval = 2;
    break;

  default:
    retval = -1;
  } // end case

} // end state q5

void s_q6(){
  t_halt = 1;
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_UP);
}

task main () {

  // Main drive motor and write head motor
  acquire(ACQUIRE_OUT_A + ACQUIRE_OUT_B){ 
  int stateval = 0;  // temporarily stores the state to avoid clobbering retval

  pullup(); // turing pullup routine

  // Start at q0
  retval = 0;

      
  while(t_halt == 0){
        stateval = retval;  // copy state from retval
        read();             // Load read value into retval for the upcoming state to use
        switch(stateval){

          case 0:
            s_q0();
            break;

          case 1:
            s_q1();
            break;

          case 2:
            s_q2();
            break;

          case 3:
            s_q3();
            break;

          case 4:
            s_q4();
            break;

          case 5:
            s_q5();
            break;

          case 6:
            s_q6();
            break;

			} // End case


      // Check for error
      if(retval == -1)
        error();

			// Optimisation to prevent writing when
			// already in correct state
			if(t_read != t_write)
				write(t_write);

			// Move.
			if(t_move == R)
				right();
			if(t_move == L)
				left();
    } // end while trampoline

  } // end resource grab

} // end main
