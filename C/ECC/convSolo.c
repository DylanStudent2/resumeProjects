#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#pragma pack(1)




/*
   Struct representing flip flops (single-bit save registers). 
   left would take the previous input, while right would take the input that was saved in left. 
   These two have 0 stored in the beginning, and through 
   "flushing" will end with 0's stored. 
*/
typedef struct reg{
   unsigned char left : 1;
   unsigned char right : 1;

}val;

static val flip;

/*
   Each char contains 1 bit of data and 1 bit of redundancy. These "redundancy" bits help make the data more robust against noise.
   If a channel were to sometimes have a voltage that is too low/too high (ie become noisey), it could be rounded incorrectly to the opposite bit.
                                                
                                                **
   first represents the two lowest bits,  11111111
   second's the next two higher bits (bit positions 2 and 3)
   third is bit positions 4 and 5,
   fourth represents the two highest bit positions; 6 and 7.
*/
typedef struct output{
   unsigned char first : 2;
   unsigned char second : 2;
   unsigned char third : 2;
   unsigned char fourth : 2;
}bit_pair;

static bit_pair bits; 

/*
   This function will add redundancy to our image data.
   For every bit, there will be a pair with it. This is called
   1/2 convolution code. 
*/
void  encode(unsigned char byte, unsigned char data[]);

/*
   A miniature function that makes the bits in the struct "bits" stored in a byte
   to be returned and then saved into the data array. 
*/
unsigned char byteReturn();

static unsigned count = 0;
/*
  Our convolutional encoder This will take 1 bit and return 2 bits. Each "input" bit
  goes through what's essentially a queue that saves two previous inputs at a time.
  Depending on what the input and two saved bits are, we get certain pairs of bits as output.
  
*/
void output (unsigned char  num, int i);


void convolution(char input[], unsigned char data[], unsigned dataLength);





//Uncomment out if you wish to run convolution by itself.
int main(int argc, char * argv[]){
   

   unsigned char data[(strlen(argv[1])*2) + 1];

   convolution(argv[1], data, ((strlen(argv[1])*2) + 1));

   return 0;
}




//data[] will have an extra bit paired with every bit in buf, as well as an additional byte (+1) to store four "flush" bits. The four lowest bits in
//the additional byte are left as zero.
void convolution(char input[], unsigned char data[], unsigned dataLength){

   unsigned char buf;
   int i = 0;
   unsigned length = 0;
   length = strlen(input);

   printf("Data LENGTH: %d\n", sizeof(data));

//This takes in 1 byte at a time from the text passed through input[] 
   for (i = 0; i < length; i++){
      buf = input[i];
      encode(buf, data);
   
   }
/*
  When all the input bits are entered, the last two input bits are still stored in the flip-flops.
  Four additional bits are added onto the data's array (which are created from the XOR's of the remaining 
  input bits saved in the convolutional encoder).
  These "flush bits" help give the code's last four bits (the bits that were produced from the
  data's input) the same corrective property as the rest of the previous bits.
*/
   output(0,0);
   output(0,1);
   data[count] = byteReturn();

   printf("Flush bits are: %x\nEncoding is: \n\n", data[count]);

   for(i = 0; i < dataLength; i++){
      printf("Position %d is %x\n",i  ,data[i]);

   }

   printf("\n");


}






/*
   Every byte 
   I read from the data will double in size as each bit is paired with a "dummy" bit.
*/
void encode (unsigned char byte, unsigned char data[]){

   //Convolution encoding works by taking the bits of data from left to right, so the mask has to be
   //the highest bit of the byte. 
   unsigned char MASK = 0x80;
   unsigned char num = 0;
   int i = 0;

   //Save the highest bit into num, then shift byte left for next highest bit.
   //Every four bits from input is 8 bits of output. This loop works with the higher bits 
   for (i = 0; i < 4; i ++){
      num = MASK & byte;
      output(num, i);
      byte = byte << 1;

   }

      //Data[0] and Data[1] contain the first byte of redundancy, and so on and so forth.
      data[count] = byteReturn();
      //printf("data of position %d is %x: \n", count, data[count]);
      count++;


   //This loop takes the lower four bits
   for (i = 0; i < 4; i ++){
      num = MASK & byte;
      byte = byte << 1;
      output(num, i);
   }
      data[count] = byteReturn();
      //printf("data of position %d is %x: \n", count, data[count]);
      count++;
      //printf("count is %d\n", count);     


}
/*
   Our convolutional encoder. 
*/
void output (unsigned char  num, int i){
   
   //This represents two bits of output by the encoder.
   unsigned char redundancy = 0;
  //The highest bit extracted must be treated as a one or a zero.
   num = num >> 7;   
 
   //Input mod with first flip flop modded with second flip flop. If it's true, the left (higher) bit of output is toggled.
   if (((num ^ flip.left) ^ flip.right) == 1){
      redundancy = 2;
   }
   //Input mod with second flip flop. If true, the right (lower) bit of output is toggled.
   if ((num ^ flip.right) == 1){
      redundancy++;
   }
   //Update the registers. Input is shifted into left flip-flop, making its stored bit move to the right flip-flop.
   //The right flip-flop's stored bit ends up being deleted.
//   printf("This is the current bit: %x\n", num);
//   printf("Flip_Flop %x %x\n", flip.left, flip.right);
 

   flip.right = flip.left;
   flip.left = num;
 
   switch (i){
      case 0:
         bits.fourth = redundancy;
         break;
      case 1: 
         bits.third = redundancy;
         break;
      case 2:
         bits.second = redundancy;
         break;
      case 3:
         bits.first = redundancy;
         break;
   }
}

/*
   Take all the bit fields defined by the output function and OR them all together.
   Each byte representing a bit field is aligned onto its respective position, 
   before they're all OR'd into one byte.

*/
unsigned char byteReturn(){

    unsigned char byte = 0;
    unsigned char byte2 = 0;
    unsigned char byte3 = 0;
    unsigned char byte4 = 0;

    byte4 = bits.fourth;
    byte4 = byte4 << 6;
    byte3 = bits.third;
    byte3 = byte3 << 4;
    byte2 = bits.second;
    byte2 = byte2 << 2;
    byte = bits.first;

//    printf("This is what we wish to have: %x %x %x %x\n", bits.fourth, bits.third, bits.second, bits.first);


//Reset the global struct variable "bits".
    bits.fourth = 0;
    bits.third = 0;
    bits.second = 0;
    bits.first = 0;

//Form the encoded byte.
    byte = byte | byte2 | byte3 | byte4;

    return byte;    
}
