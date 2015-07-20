#include <stdio.h>
#include "pbes.h"   

void rol(unsigned short * number, unsigned short  amount){
                                          //Bit length of 16 basically
   *number = (*number << amount) | (*number >> ( 16 - amount)); 
}

void ror(unsigned short * number, unsigned short  amount){
   *number = *number >> amount | (*number << ( 16 - amount)); 
}


void piTable(unsigned char * table)
{
   unsigned char randval;
   FILE *f;
   char j = 0;
   short i = 0;
   char boolean = 0;
   char zeroUsed = 0;

   unsigned char count = 0;


   f = fopen("/dev/urandom", "r");



   //Initialize array
   for(i = 0; i < 256; i++){
         table[i] = 0;
   }//While loop


   for(i = 0; i < 256; i++){

         fread(&randval, sizeof(randval), 1, f);
     
         if(!zeroUsed && randval == 0){
            zeroUsed = 1;
            continue;
         }

         boolean = checkIfUsed(table, randval);
         //If the array doesn't hold the random value, then add it in
         if(!boolean){
            table[i] = randval;
         }//Otherwise redo the iteration
         else{   
            i--; 
         }

   }//While loop

   printf("Our Pi table: \n");
   for(i = 0; i < 256; i++){

      if((i % 16) == 0){
         printf("\n");
      }
      printf("%3x ", table[i]);

   }//while loop

   printf("\n");

   fclose(f);

}


char checkIfUsed(unsigned char * pointer, unsigned char randval){
   char used = 0;
   unsigned short i = 0;

   for(i = 0; i < 256; i++){

      if(randval == pointer[i]){
         used = 1;
         break;
      }
   }
   return used;
}
