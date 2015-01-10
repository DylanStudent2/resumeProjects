#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#pragma pack(1)


/*
   Takes the data that was encoded and splits each group of multiple (2) bits into their own separate byte.
   Each byte in decode_pairs will contain 00, 01, 10, or 11 from this. size is for the number of 
   bytes in memory that decode_pairs holds. 
*/
void input_to_values (unsigned char  decode_pairs[], int size, unsigned char recieved[]);

/*
   What we'll use to run the program. main's only purpose is to put input into the recieved array.

*/
void viterbi(unsigned char recieved[], unsigned recieveSize);

//XOR a select amount of bits in the data to simulate noise using the rand() function.
void applyNoise(unsigned char recieved[], unsigned recieveSize);


/*
   There'll be six tables for making our viterbi algorithm:

   1. next_state. This stores all the possible states that you can get depending on what your current state x (array[x][2]) is. 
      say we had a current state of 10, that would be array[2]. At current state 10, there's only two possible "next" states it can have,
      depending on whether a 1 or 0 is input. array[2][0] would store 01 and array[2][1] would store 11.

   2. output. Works in the same way as the previous array. The name represents what's stored (in this case, products of XORing input and 
      flip-flops). The row bracket represents the current state once again, while the column is either 0 or 1 as input. 

   3. input. Inputs of either 0 or 1 are stored this time, while row represents current state and column is next state. Since 
      states can only have two different next states, not all the slots in this array will be used. 
*/

//The three arrays above must be initialiazed. They act as a sort of reference guide. This will be done through prototype:

void initialize(unsigned char  next[][2], unsigned char  output[][2], unsigned char input[][4]);

/*

   4. state_his. State history. Stores the best previous state into the current state. The "best" previous state
      is determined by the lowest "accumulated error" which will be explained in 5. According to a website 
      this array is limited to storing K x 5 + 1 data ( where K = 3). I wish to be able to store things in groups of 8
      (obviously) so this instead will hold 19 pieces of data. Indexes 0-15 holds the recieved data,
      and 16-17 are the "flush bits". These two will be shifted down to indexes 0 and 1, as they're actual data that needs
      to be processed. The final bits stored in [16] and [17] will be discarded since they are the actual flush bits.

   5. error_total. Stores "accumulated error metric" in  each [state]. The error metric is determined by XORING the output created with the
      recieved bits at that index. As is the usual, the current state has multiple (in this case 2) "next states" depending on whether
      the input is a 0 or 1. The error metric created (from Xoring the output with the recieved convolution bits) will be stored into that 
      next state. Each next state has multiple (2) different error metric. Whichever metric is lower will have its previous
      state that createcd the lower error stored into the state_his.
      If they turn out to be equal either can be picked. 

   6. state_seq. The states are selected from the end and works back to the beginning. The number selected is put through
      the input table which gives our real bit of data.

*/

//As it says in the name. Array "error_total" needs a third column in it to keep track of the previous state. The first and second columns
//represent the two different possible aggregate errors in a given state.
void getHistory(unsigned char decode_pairs[], unsigned char next_state[][2], unsigned char output [][2], unsigned char error_total[][3], unsigned char state_his[][18]);

//Follow a value starting at the final column in state_his, storing the values into state_seq
//along the way.
void traceBack(unsigned char state_his[][18], unsigned char state_seq[]);

//Take what we got from state_seq and turn it into actual data. 2 bytes of data will be stored in info at a time,
void interpret(unsigned char state_seq[], unsigned char input[][4], unsigned char info[]);

//Prepares the next pairs to be processed. Pairs stored in decode_pairs from indexes 0-1 get replaced
//by the two final pairs that acted as "flush" bits, while indexes 2-17 get replaced by the next 16 values after it.
//A pointer keeps track of the content that isn't processed yet. 
void nextDecode(unsigned char decode_pairs[]);

int main (int argc,char * argv[]){
   unsigned length = 0;

   int i = 0;
   length = (strlen(argv[1]) * 2) + 1;

   unsigned char recieved[length];

   for(i = 0; i < length; i++){
      recieved[i] = 0;
   }

   
   //The program is written in a way to work with two characters at a time.
   if(!(length%2)){
      fprintf(stderr, "Please enter an even number of characters!\n");
      exit(1);
   }

/*********************************************************************************************************************
   Test case from the website. The original data was shorter than 2 bytes,
   so the first pair of flush bits is part of recieved [3]. This meant
   the size of the array decode_pairs in viterbi was decreased by 3 instead of by 2. 

   recieved [0] = 0x3c;
   recieved [1] = 0x67;
   recieved [2] = 0xe0;
   recieved [3] = 0xce;
   recieved [4] = 0xc0;
*/


   convolution(argv[1], recieved, length);

   applyNoise(recieved, length);


   printf("Sent:\n");
   for(i = 0; i < strlen(argv[1]); i++){
      printf("%2x ", argv[1][i]);
   }

   printf("\n\n");



   viterbi(recieved, length);
   

   return 0;
}


void viterbi(unsigned char recieved[], unsigned recieveSize){

   //resulting info from the decoding.
   unsigned char info[(recieveSize -1)/2];   
   
   unsigned char next_state[4][2];
   unsigned char output[4][2];
   unsigned char input [4][4];
   unsigned char state_his[4][18];
   unsigned char error_total [4][3];
   unsigned char state_seq[18];
   int size = 0;

   //Each byte retrieved is split, every two bits will be stored in a byte in decode_pairs.
   size = (recieveSize * 4) - 2;    //highest four bits of the last value in recieved[] is the final flush pair.
                                        //the rest of those bits are not important, so the array is cut by 2 bytes.
   unsigned char decode_pairs[size];

   //counters.
   int i = 0;
   int k = 0;
   int f = 0;

   for(i = 0; i < 4; i++){
      for(k = 0; k < 2; k++){
         next_state[i][k] = 0;
      }
   }

   input_to_values(decode_pairs, size, recieved);

   initialize(next_state, output, input);


   //Go through the steps of turning our recieved message into
   //the actual data. 
   for(i = 0; i < sizeof(info); i += 2){ 
      getHistory(decode_pairs, next_state, output, error_total, state_his);
      traceBack(state_his, state_seq);
      interpret(state_seq, input, info);
      nextDecode(decode_pairs);

      /************--Displays state history table--************************************************
      for(f = 0; f < 4; f++){
         printf("row %d: ", f);

         for(k = 0; k < 18; k++){
            printf("  %d :", state_his[f][k]);
         }

         printf("\n");
      }
      */

   }//end for loop


   printf("Retrieved:\n");

   for(i = 0; i < sizeof(info); i++){
      printf("%2x ", info[i]);   

   }

   printf("\n");


}//end viterbi

void nextDecode(unsigned char decode_pairs[]){
   int i = 0;
   static unsigned char offset = 16;
   unsigned char * current = &decode_pairs[offset];
   

   for(i = 0; i < 18; i++){
      decode_pairs[i] = *current;
      current++;
      offset++;
   }

   //Set the offset back by two steps, I need to point the the next pair that were used
   //as "flush bits", but still need to be interpreted as data.
   offset -= 2;


}//nextDecode


//Light noise simulation. 
void applyNoise(unsigned char recieved[], unsigned recieveSize){
   int i = 0;
   int k = 0;
   unsigned value = 0;
   unsigned char shift = 0; //for changing a particular byte

   unsigned odds = 40;

   unsigned char mask = 0x80;

   time_t t;
   
   //Initialize random number generator.
   srand((unsigned) time(&t));
   
   

   for(i = 0; i < recieveSize; i++){

      for(k = 0; k < 8; k ++){

         if(k != 0){
            mask = mask >> 1;
         }

         value = rand() % odds;

         if(value < 1){ //one attempt of a 2.5% chance with seven 3% chances of a byte being modified.
                        

            recieved[i] = recieved[i] ^ mask;

            odds = 40;

            //jump to next byte
            break;
         }
         else{
            odds = 30;
         }


      }//end for

      mask = 0x80; //reset mask for next byte

   }//end for

   printf("The change applied:\n");

   for(i = 0; i < recieveSize; i++){
      printf("%2d ",i);

   }

   printf("\n");

   for(i = 0; i < recieveSize; i++){
      printf("%2x ", recieved[i]);

   }

   printf("\n");

}//applyNoise ends


//Remember: there will be 4 values stored into decode_pairs for every 1 value from recieved.
void input_to_values (unsigned char * decode_pairs, int size,  unsigned char recieved[]){
   //index for decode_pairs array.  
   int i = 0;
   //index for recieved array.
   int f = 0;

   //Take the two highest bits
   unsigned char mask = 0xc0;

   //The i is initialized to 1 instead of 0 to get the inner if statement to activate once every 4 loops. 
   //It would've activated on 5th loop otherwise. Since I still wish to iterate "sizeof(decode)"- times, I add an additional 1 to the limit.
   //Every assignment of decode will have its index substracted by 1 to keep things balanced. 
   for ( i = 1; i < size + 1; i++){

      //take the highest two bits into decode...
      decode_pairs[i-1] = (mask & recieved[f]);
      //Then shift them into a small value.
      decode_pairs[i-1] = decode_pairs[i-1] >> 6;
            
      //If there's bits left in recieved[f]
      if ((i % 4) != 0){
         //move the next bits to be &'ed and stored
         recieved[f] = recieved[f] << 2;
      }
      else{ 
         //otherwise go to the next stored byte in recieved[]
         f++;
      }

   }//end for loop

}//end input_to_values


/*
  It's easy to make a diagram of these three sets of reference. 
  See the link -- home.netcom.com/~chip.f/viterbi/aglrthms2.html -- to see charts
*/
void initialize(unsigned char  next[][2], unsigned char  output[][2], unsigned char input[][4]){
   
   int i = 0;
   int f = 0;
   for (i = 0; i < 4; i ++){


      next[i][1] += 2;

      if (i >= 2){

         next[i][0]++;
         next[i][1]++;
         input[i][0] = 0xf;
         input[i][1] = 0;
         input[i][2] = 0xf;
         input[i][3] = 1;
      }
      else{
         input[i][0] = 0;
         input[i][1] = 0xf;
         input[i][2] = 1;
         input[i][3] = 0xf;

      }

   }//end for

         //If there were more flip-flops and redundant bits being used, I'd 
         //do something more convenient than hammering it out. 
         output[0][0] = 0;
         output[0][1] = 3;
         output[1][0] = 3;
         output[1][1] = 0;
         output[2][0] = 2;
         output[2][1] = 1;
         output[3][0] = 1;
         output[3][1] = 2;

}//end initialize


void traceBack(unsigned char state_his[][18], unsigned char state_seq[]){
      int i = 0;
      //The previous state. This gets passed into state_seq's row bracket 
      //where we'll update this variable again and so forth.
      unsigned char previous = 0;

      //Initialize the sequence!
      for(i = 0; i < 18; i++){
         state_seq[i] = 0;
      }
      //Our traceback!!
      for (i = 17; i >= 0; i--){
         state_seq[i] = previous;
         previous = state_his[previous][i];
      }


//Shows the state sequence to be interpretted 
/*      printf("The state sequence is:\n");
      for (i = 0; i < 17; i++){
         printf("%d ", state_seq[i]);
      }

      printf("\n");
*/  

}//end traceback

void interpret(unsigned char state_seq[], unsigned char input[][4], unsigned char info[]){
   static unsigned char index = 0;
   int i = 0;
   int j = 7;

   //input table really isn't necessary for this. It's a sort of reference guide.
   //Any value below 2 has a data bit of 0 while anything equal or above it is 1. 
   for(i = 0; i < 16; i++){

      if(state_seq[i] < 2){
         state_seq[i] = 0;
      }
      else{
         state_seq[i] = 1;
      }
 
   }
   
   //Set up the first byte. Have state seq aligned properly to be XOR'd.   
   for(i = 0; i < 8; i++){

      state_seq[i] = state_seq[i] << j;
      j--;

   }

   //Reset our shift counter, since it's going to be reused for the second byte.
   j = 7;
       
   //Set up the second byte.
   for(i = 8; i < 16; i++){

      state_seq[i] = state_seq[i] << j;
      j--;

   }

   //I wouldn't want to Xor a variable with itself, so the i's start 1 digit above the byte that
   //stores the value.
   for(i = 1; i < 8; i++){
      state_seq[0] = state_seq[0] ^ state_seq[i];
   }
   for(i = 9; i < 16; i++){
      state_seq[8] = state_seq[8] ^ state_seq[i];
   }

   info[index++] = state_seq[0];
   info[index++] = state_seq[8];
      
//   printf("This is info %x at %x\n", index - 2, info[index - 2]);
//   printf("This is info %x at %x\n", index - 1, info[index - 1]);

}//end interpret


void getHistory(unsigned char decode_pairs[],  unsigned char next_state[][2], unsigned char output [][2], unsigned char error_total[][3], unsigned char state_his[][18]){

  //Variables for checking addition of error in each iteration:
    int er[4]; //For debugging purposes. 
    int k = 0; //index for tables with 4 rows.
    int i = 0; //incremental index for loops
    int f = 0;

    const unsigned size = 18;

    //initialize/reset error_total and state_his.
    for(i = 0; i < 4; i++){
       for (k = 0; k < 3; k++){
          error_total[i][k] = 0;
       }  
       er[i] = 0;
    }
    for(i = 0; i < 4; i++){
       for (k = 0; k < 18; k++){
          state_his[i][k] = 0;
       }   
    }
    /*
      There's certain cases occuring depending on iterator i's value.
      The first two iterations involve making the next states available
      in branching out. See the trellis on the website -http://home.netcom.com/~chip.f/viterbi/algrthms2.html -  
      to get an idea on what this is imitating. The final two iterations will
      only use  0 as input, which makes the diagram go back to state 0.

    */

    for(i = 0; i < size; i++){

       if( i == 0){
          //formula for recieving the bit difference (error metric) is 
          //(x ^ y) - ((x^y)/2). This lets 00 ^ 01 be unaffected by the subtraction
          //while any XOR of bit a and b (like ab ^ ba) will result in 2. 
        
          error_total[0][0] = ( er[0] = ((output[0][0] ^ decode_pairs[i]) - (output[0][0] ^ decode_pairs[i]) / 2));
          error_total[0][2]  =   error_total[0][0]; 

          error_total[next_state[0][1]][0] =  (er[2] = ((output[0][1] ^ decode_pairs[i])   - (output[0][1] ^ decode_pairs[i]) / 2));
          error_total[next_state[0][1]][2] = error_total[next_state[0][1]][0];
        

      //error totals in next iteration must have the first error values stored into them
      }
      else if ( i == 1){

         for(k = 0; k < 4; k += 2){

            error_total[next_state[k][0]][0]  = error_total[k][2] +  (er[f++] = ((output[k][0] ^ decode_pairs[i]) - (output[k][0] ^ decode_pairs[i]) / 2)); 
         
            error_total[next_state[k][1]][0]  = error_total[k][2] +  (er[++f] = ((output[k][1] ^ decode_pairs[i])   - (output[k][1] ^ decode_pairs[i]) / 2)); 
            f = 1;
            state_his[next_state[k][0]][i] = k;
            state_his[next_state[k][1]][i] = k;
         } 
         for(k = 0; k < 4; k++){
           error_total[k][2] = error_total[k][0];
         }
                 
      }//end else if 
      //When we're at the flush bits, we have only errors from inputs of 0.
      else if (i >= size - 2){
      
      //At the last flush bit only two states will create output.
         if ( i == size - 1){
 
            for ( k = 0; k < 2; k ++){      //was k%2
               error_total[next_state[k][0]][k]  = error_total[k][2] +  (er[k] = ((output[k][0] ^ decode_pairs[i]) - (output[k][0] ^ decode_pairs[i]) / 2));
         
            }
         
            //The final value is limited to two previous states.
            if(error_total[0][0] <= error_total[0][1]){
               state_his[0][i] = 0;

            }
            else{
               state_his[0][i] = 1;
            }

         }
         //The flush bit before it will still work with all states.
         else{

            for ( k = 0; k < 4; k ++){
               error_total[next_state[k][0]][k%2]  = error_total[k][2] +  (er[k] = ((output[k][0] ^ decode_pairs[i]) - (output[k][0] ^ decode_pairs[i]) / 2));
            }

            for ( k = 0; k < 2; k++){

               //As was *mentioned* earlier, state_history's value is limited to two different values depending
               //On which aggregate error is less. The small inner "if" statements address this.
               //Whichever error total at a current state is lower has its value stored into error_total[][2]
               //which keeps track of its total error for the next iteration. 
               if (error_total[k][0] <= error_total[k][1]){
                          
                  error_total[k][2] = error_total[k][0];

                  if(k == 0){
                     state_his[k][i] = 0;
                  }
                  else if (k == 1){
                     state_his[k][i] = 2;
                  }

               }//end if 
               else {

                  error_total[k][2] = error_total[k][1];

                  if (k == 0){
                     state_his[k][i] = 1;
                  }
                  else if (k == 1){
                     state_his[k][i] = 3;
                  }

               }//end else

            }//end for loop
        }//end else
   }//end else if
   else{

     for (k = 0; k < 4 ; k ++){
         
        /* error_total[][0] and error_total[][1] stores two different errors accumulated from two different previous states.
           previous states of 0 and 1 have their error aggregate stored into [0] for the current state,
           while states of 2 and 3 have theirs stored in [1]
           It's worth mentioning that prev. states 0 and 1 always leads to  0 or 2 while 2 and 3 always leads to 1 or 3. 
         */
         
         error_total[next_state[k][0]][k%2]  = error_total[k][2] +  (er[k] = ((output[k][0] ^ decode_pairs[i]) - (output[k][0] ^ decode_pairs[i]) / 2));
         
         error_total[next_state[k][1]][k%2]  =  error_total[k][2] + (er[k] = ((output[k][1] ^ decode_pairs[i])   - (output[k][1] ^ decode_pairs[i]) / 2));
        

     }//end for loop
        //Errors from inputs of 1 are included into this. This adds different possiblities on what the previous state can be.
        for ( k = 0; k < 4; k++){

             if (error_total[k][0] <= error_total[k][1]){

               error_total[k][2] = error_total[k][0];

               if(k == 0 || k == 2){
                  state_his[k][i] = 0;
               }
               else if (k == 1 || k == 3){
                  state_his[k][i] = 2;

               }

             }
             else {

                  if (k == 0 || k == 2){
                     state_his[k][i] = 1;
                  }
                  else if (k == 1 || k == 3){
                     state_his[k][i] = 3;

                  }
                  error_total[k][2] = error_total[k][1];
             }//end else

        }//end for

 }//end else
 
}//end for loop
        
}//end getHistory


