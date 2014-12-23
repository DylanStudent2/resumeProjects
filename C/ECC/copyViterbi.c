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


/*
   There'll be six tables for making our viterbi algorithm:

   1. next_state. This stores all the possible states that you can get depending on what your current state x (array[x][2]) is. 
      say we had a current state of 10, that would be array[2]. At current state 10, there's only two possible "next" states it can have,
      depending on whether a 1 or 0 is input. array[2][0] would store 01 and array[2][1] would store 11.

   2. output. Works in the same way as the previous array. The name represents what's stored (in this case, products of XORing input and 
      flip-flops). The row bracket represents the current state once again, while the column is either 0 or 1 as input. 

   3. input. Inputs of either 0 or 1 are stored this time, while row reperesents current state and column is next state. Since 
      states can only have two different next states, not all the slots in this array will be used. 
*/

//The three above must be initialiazed. They act as a sort of reference guide. This will be done through prototype:

void initialize(unsigned char  next[][2], unsigned char  output[][2], unsigned char input[][4]);

/*

   4. state_his. State history. Stores the best previous state into the current state. The "best" previous state
      is determined by the lowest "accumulated error" which will be explained in 5. According to a website 
      this array is limited to storing K x 5 + 1 data ( where K = 3). I wish to be able to store things in groups of 8
      (obviously) so this instead will hold 19 pieces of data. Index 0 Holds the initial set up, 1-16 holds the recieved data,
      and 17-18 are the "flush bits". These two will be shifted down to indexes 1 and 2, as they're actual data that needs
      to be processed. The final bits stored in 17 and 18 will be discarded since they are the actual flush bits.

   5. error_total. Stores "accumulated error metric" in  each [state]. The error metric is determined by XORING the output created with the
      recieved bits at that index. As is the usual, the current state has multiple (in this case 2) "next states" depending on whether
      the input is a 0 or 1. The error metric created (from Xoring the output with the recieved convolution bits) will be stored into that 
      next state. Each next state has multiple (2) different error metric. Whichever metric is lower will have its previous
      state that createcd the lower error stored into the state_his.
      If they turn out to be equal either can be picked. 

   6. state_seq. The states are selected from the end 

*/

//
void getHistory(unsigned char decode_pairs[], int size, unsigned char next_state[][2], unsigned char output [][2], unsigned char error_total[][2], unsigned char state_his[][19]);


int main (int argc,char * argv[]){
   unsigned char recieved[5];

   //recieved [0] = 0x35;
   //recieved [1] = 0xc3;
   //recieved [2] = 0xb0;
   recieved [0] = 0x3c;
   recieved [1] = 0x67;
   recieved [2] = 0xe0;
   recieved [3] = 0xce;
   recieved [4] = 0xc0;

   viterbi(recieved, sizeof(recieved));
   

   return 0;
}


void viterbi(unsigned char recieved[], unsigned recieveSize){

   unsigned char info[(sizeof(recieved)-1)/2];   
   
   printf("size: %d\n", sizeof(info));
   printf("size of recieved : %d\n",recieveSize);
   unsigned char next_state[4][2];/////////////////[2^K-1][2^k] where k is number of real bits to paired bits. k will alwas be 1. 
   unsigned char output[4][2];
   unsigned char input [4][4];
   unsigned char state_his[4][19];
   unsigned char error_total [4][2];
   unsigned char state_seq[16];
   int size = (recieveSize * 4) - 3;//was 2  //highest two bits of the last value in recieved[] is the final flush pair.
                                                  //the rest of the bits are not important.
   unsigned char decode_pairs[size];
   int i = 0;
   int k = 0;
   printf("size of decode pairs: %d\n", sizeof(decode_pairs));
   input_to_values(decode_pairs, size, recieved);
   initialize(next_state, output, input);
   //for(i = 0; i < sizeof(decode_pairs); i++){
   //   printf("pos %d is %d \n", i, decode_pairs[i]);
   //}
   getHistory(decode_pairs, size, next_state, output, error_total, state_his);


/////////////////////////////////////////////
/*   for(i = 0; i < size; i++){
      for(k = 0; k < 4; k++){
      printf("column %d row %d,  %d :\n", i, k, state_his[k][i]);
      }
   }
*/
}

//Remember: there will be 4 values stored into decode_pairs for every 1 value from recieved.
void input_to_values (unsigned char decode_pairs[], int size,  unsigned char recieved[]){
   //index for decode_pairs array.  
   int i = 0;
   //index for recieved array.
   int f = 0;

   unsigned char mask = 0xc0;
   printf("decode_pairs is apparently %d", size);
   printf("\n");


   //The i is initialized to 1 instead of 0 to get the inner if statement to activate once every 4 loops. 
   //It would've activated on 5th loop otherwise. Since I still wish to iterate sizeof(decode) times, I add an additional 1 to the limit.
   //Every assignment of decode will have its index substracted by 1 to keep things balanced. 
   for ( i = 1; i < size + 1; i++){

      //take the highest two bits into decode...
      decode_pairs[i-1] = (mask & recieved[f]);
//////////////      printf("\nrecieve %x at index %d\n", recieved[f], f);    
      //then turn the high bits into low bits.
      decode_pairs[i-1] = decode_pairs[i-1] >> 6;
            

      printf("%d\n ", decode_pairs[i-1]);
    
      //If there's bits left in recieved[f]
      if ((i % 4) != 0){
         //move the next bits to be &'ed and stored
         recieved[f] = recieved[f] << 2;
      }
      else{ 

         //otherwise go to next stored byte in recieved[]
         f++;
      }
         


   }
   printf("\n");

   for(i = 0; i < size; i++){

      printf(" %d ", decode_pairs[i]);

   }
   printf("\n");

   
}


/*
  It's easy to make a diagram of these three sets of reference, though in here it's harder to read. 
  See home.netcom.com/~chip.f/viterbi/aglrthms2.html to see charts
  in a more readable format. 

*/
void initialize(unsigned char  next[][2], unsigned char  output[][2], unsigned char input[][4]){
   
   int i = 0;
   int f = 0;
   for (i = 0; i < 4; i ++){


      next[i][1] += 2;

      if (i >= 2){

         next[i][0]++;
         next[i][1]++;
         input[i][0] = 0xf;//0xf means unavailable. empty. etc, since 0 actually serves a purpose. 
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

   }
            //I'm not going to spend time figuring out a loop to initialize this one conveniently. 
            output[0][0] = 0;
            output[0][1] = 3;
            output[1][0] = 3;
            output[1][1] = 0;
            output[2][0] = 2;
            output[2][1] = 1;
            output[3][0] = 1;
            output[3][1] = 2;
/*
    for(i = 0; i <4; i++){
         printf("table of next state %d: ", i);
      for (f = 0; f < 2; f++){
         printf("%d ", next[i][f]);


      }
         printf("\n");

    }
 
    for(i = 0; i <4; i++){
         printf("table of output %d: ", i);
      for (f = 0; f < 2; f++){
         printf("%d ", output[i][f]);


      }
         printf("\n");

    }
    
      for(i = 0; i <4; i++){
         printf("current state %d: ", i);
      for (f = 0; f < 4; f++){
         printf("%d -> %d | ", f, input[i][f]);


      }
         printf("\n");

   
    }
    
  */   


}



void getHistory(unsigned char decode_pairs[], int size,  unsigned char next_state[][2], unsigned char output [][2], unsigned char error_total[][2], unsigned char state_his[][19]){


  //Error total[]
  //
    int k = 0; //index for tables with 4 rows.
    int i = 0; //incremental index for 
    printf("Inside getHistory\n");
    //initialiaze/reset error_total and state_his.
  
    for(i = 0; i < 4; i++){
         
      for (k = 0; k < 2; k++){
         error_total[i][k] = 0;


      }   
    }
    for(i = 0; i < 4; i++){
         
      for (k = 0; k < 19; k++){
         state_his[i][k] = 0;


      }   
    }
    /*
      There's certain cases occuring depending on iterator i's value.
      The first two iterations involves making the next states available
      in branching out. See the trellis on the website I linked to get 
      an idea on what this is imitating. The final two iterations will
      only use  0 as input, which makes the diagram go back to state 0.

    */
    for(i = 0; i < size; i++){
      if( i == 0){
         //formula for recieving the bit difference (error metric) is 
         //(x ^ y) - (x^y)/2. This lets 00 ^ 01 be unaffected by the subtraction
         //while any XOR of bit a and b (like ab ^ ba) will result in 2. 
         error_total[next_state[0][0]][0] +=  output[0][0] ^ decode_pairs[0];
         printf("decode_pairs %x\n", decode_pairs[i]);
         error_total[next_state[0][0]][1]  =   error_total[next_state[0][0]][0];         
         error_total[next_state[0][1]][0] +=  output[0][1] ^ decode_pairs[0];
         error_total[next_state[0][1]][1]  =   error_total[next_state[0][1]][0];         
         
         printf("error total of 0: %d \n", error_total[0][0]);
         printf("error total of 2: %d \n", error_total[2][0]);
      }

    }


    
    

    


}
