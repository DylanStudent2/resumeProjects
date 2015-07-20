#include "pbes.h"
#define ENCRYPT 0
#define DECRYPT 1

/*
   PBES1 encryption, 
   By Dylan Soderman

   arguments for compiling:

   gcc pbes.c permTable.c md5file.c md5.c -o read

   For executing:
   ./read <seed1> <seed2> <iteration count> <password> <message>

*/



static union keyBuffer buffer;
static unsigned char randTable [256];
static int w = 0;

//The designated plain/cipher text that will be worked with 
static union text txt;

//Our initialization vector for the RC2 chain
static unsigned char ivBuffer[8];

int main (const int argc, char * argv[]){
   unsigned long salt; 
   unsigned seed1;
   unsigned seed2;
   struct mdHash derivedKey;
   int index = 0;
   int iterations = 0;
   int i = 0;
   int j = 0; //Pointer for cipherarray
   int k = 0; //Counter for the number of encryption/decryption runs
   int blocks = 0;

   if(argc != 6){
      fprintf(stderr, "Need arguments: <seed1> <seed2> <iteration count> <password> <message>\n");
      return 0; 
   }
   index = strlen(argv[5]);
   char message[strlen(argv[5]) + ( 8 - (strlen(argv[5]) % 8))+ 1];
   unsigned char cipherText[sizeof(message)];
   unsigned char plainText[sizeof(message)];

   message[strlen(argv[5])] = '\0'; 
   seed1 = atoi(argv[1]); 
   seed2 = atoi(argv[2]);
   iterations = atoi(argv[3]);
   strncpy(message, argv[5], strlen(argv[5]));
   salt = saltGen(seed1, seed2);
   printf("Message: %s\n", message);


//Initialize my plaintext/ciphertext arrays
   for(i = 0; i < strlen(message); i ++){
      cipherText[i] = 0;
      plainText[i] = 0;
   }
   cipherText[i] = '\0';
   plainText[i] = '\0';
//-----------------------------------------------

   pbkdf(argv[4], salt, iterations,  &derivedKey);
   printf("\nMy derived key in hex:   %17llx %17llx\n", derivedKey.highOrder, derivedKey.lowOrder);

   padMessage(message);
   
   //rc2init stores first plaintext block xored with IV into array txt as well
   rc2Init(derivedKey.highOrder, derivedKey.lowOrder, message);
   
   printf("Text With IV XORED  : ");

   for(i = 0; i < 8; i++){
      printf("%2x", txt.M[i]);
   }
  
   //Number of times I'll go through encryption/decryption, which is N / 8
   blocks = sizeof(message) >> 3;
   

   //-------------------Encryption Segment-----------------------------
   for(k = 0; k < blocks; k++){

      rc2Chain(ENCRYPT);

      //set up text.M to contain the next 8 bytes of the message, to be XORED with 
      //the recently produced ciphertext which is stored in its respective array.

      for(i = 0; i < 8; i++){
         cipherText[i+j] = txt.M[i];
      }

      //Set up next 8 bytes to be encrypted
      //Do not do this if there's only 8 bytes to encrypt, or if I've encrypted
      //the final block
      if(blocks != 1 && k != (blocks - 1) ){

         for(i=0; i < 8; i++){
                    //Previous cipher xored with current plaintext
            txt.M[i] = cipherText[(j + i)] ^ message[j + i + 8];
         }
      }
      j += 8;

   }//end for loop------------------------------------------------------

   printf("\nEntire Cipher Text   : ");
   for(i = 0; i < j; i++){
      printf("%2x", cipherText[i]);
   }
   printf("\n");
   
   //Reset j
   j = 0;

/////------------------------------Decryption Segment----------------------

//Start with the first cipher block
   for(i = 0; i < 8; i++){
      txt.M[i] = cipherText[i]; 
   }

   for(k = 0; k < blocks; k++){

      rc2Chain(DECRYPT);

      if(k != 0){
         for(i=0; i < 8; i++){
            plainText[i+j] = cipherText[i + j - 8] ^ txt.M[i];
         }
      }
      else{
         for(i=0; i < 8; i++){
            plainText[i+j] = ivBuffer[i] ^ txt.M[i];
         }
      }

      if(blocks != 1 && k != (blocks - 1) ){
         j += 8;
         for(i=0; i < 8; i++){
            //Set up for next block to be deciphered. 
            txt.M[i] = cipherText[(j + i)]; 
         }
      }
   }

   plainText[index] = '\0';
  
   printf("Entire Plain Text    : ");
   for(i = 0; i < index; i++){
      printf("%2x", plainText[i]);
   }
   
   printf("\nMessage: %s\n", plainText);

   return 0;
}


//Calls mix and mash 
void rc2Chain(char option){
   int i = 0;

   //Set the expanded-key buffer pointer to the beginning or 
   //end, depending on whether I'm encrypting, or decrypting.
   if(option == ENCRYPT){
      w = 0;  
   }
   else{
      w = 63;
   }

   for(i = 0; i < 5; i++){
      mix(option);
   }

   mash(option);

   for(i = 0; i < 6; i++){
      mix(option);
   }

   mash(option);

   for(i = 0; i < 5; i++){
      mix(option);
   } 

}//end rc2Chain

//One Mixing "round"
void mix(char option){
   short i = 0;
   unsigned short s[4];
   s[0] = 1;
   s[1] = 2;
   s[2] = 3;
   s[3] = 5;

   
   if(option == ENCRYPT){
      for(i = 0; i < 4; i++){
         txt.R[i] = txt.R[i] + buffer.K[w] + (txt.R[(unsigned)(i-1) % 4] & txt.R[(unsigned)(i-2) % 4]) + ((~txt.R[(unsigned)(i-1) % 4]) & txt.R[(unsigned)(i-3) %4]);
         w++;
         rol(&txt.R[i], s[i]);    
      }
   }
   else{
      for(i = 3; i >= 0; i--){
         ror(&txt.R[i], s[i]);
         txt.R[i] = txt.R[i] - buffer.K[w] - (txt.R[(unsigned)(i-1) % 4] & txt.R[(unsigned)(i-2) % 4]) - ((~txt.R[(unsigned)(i-1) % 4]) & txt.R[(unsigned)(i-3) %4]);
         w--;     
      }
   }
}//end mix

//One Mashing "round"
void mash(char option){
   short i = 0;
   
   if(option == ENCRYPT){

      for(i = 0; i < 4; i++){   
                            //Low order six bits of R[i-1] as an index..
         txt.R[i] = txt.R[i] + buffer.K[txt.R[(unsigned)(i-1) % 4] & 63];
      }
   }
   else{
      for(i = 3; i >= 0; i--){
         txt.R[i] = txt.R[i] - buffer.K[txt.R[(unsigned)(i-1) % 4] & 63];
      }
   }

}//end mash


void rc2Init(uint64_t key, uint64_t iv, char * message){

   //Our key buffer.
   int i = 0;
   unsigned char mask = 0x0FF;///Improve variable management!!!
   //unsigned t1;//Key length in bytes
   //unsigned t8;//Key lenth in bits
   unsigned char value = 0;

   //Set up a permuted table that contains values  0-255. 
   piTable(randTable);

   //TM is based on:  
   //T8 = (T1+7)/8, where T1 is the bit-length of our key (8*8 = 64)
   //TM = 255 MOD 2^(8+ T1 - 8* T8) = 255 MOD 2^ (72- 64) = 255 MOD 256 
   //It would change based on whether the key length is not divisible by 8.
   unsigned char TM = 0xFF;
   
   for(i = 0; i < 128; i++){
      buffer.L[i] = 0;
   }
//---------------------------------- 
//                                   
//Set up IV
   for(i = 7; i >= 0; i--){
      ivBuffer[i] = (unsigned char)(mask & iv);
      iv = iv >> 8;
   }
  
   printf("IV buffer           : ");
   for(i = 0; i < 8; i++){
      printf("%2x", ivBuffer[i]);
   }
   printf("\n");

   //XOR IV with the message's first 8 bytes, T-length 
   for(i=0; i < 8; i++){
      txt.M[i] = ivBuffer[i] ^ message[i];
   }

//-----------------------------------------

   for(i = 7; i >= 0; i--){
      buffer.L[i] = (unsigned char)(mask & key);
      key = key >> 8;
   }
//------------------------------------------
//Key expansion
      //   T
   for(i = 8; i < 128; i++){                            //T
      buffer.L[i] = randTable[(buffer.L[i - 1] + buffer.L[i - 8]) % 256];
   }
               //T8                          T8  
   buffer.L[128 - 8] = randTable[buffer.L[128-8] & TM];

   //          T8
   for(i= 127- 8; i >= 0 ; i--){                     //T8
      buffer.L[i] = randTable[buffer.L[i+1] ^ buffer.L[i + 8]];

   }

}//end rc2Init

//Stores the password using the key pointer, it will be of dkLength (16 bytes in this case)
void pbkdf(char * password, long unsigned  salt, unsigned iterations, struct mdHash * key){
   unsigned char str[17];//string that will contain the salt.   
   int i = 0;
   FILE * fp = NULL;
   unsigned char passAndSalt[] = "ArbitraryFileName";
   int fd = 0; 
   unsigned char stringKey[33];
   unsigned char firstHalf[19];
   unsigned char secondHalf[19];
   //unsigned long long value = 0;
   sprintf(str, "%lx", salt);

   fp = fopen(passAndSalt, "w+");
   

   if(fputs(password, fp) < 0){
      fprintf(stderr, "Error with writing password to file %s\n", passAndSalt);
      exit(-1);
   }

   if(fputs(str, fp) < 0){
      fprintf(stderr, "Error with writing salt to file %s\n", passAndSalt);
      exit(-1);
   }

   fclose(fp);

   //File pointer FP interferes with md5file
   md5file(passAndSalt, stringKey);
 
   //Md5sum a previous md5sum that's stored in the file. 
   for(i = 0; i < iterations; i++){
      fd = open(passAndSalt, O_RDWR | O_TRUNC, NULL);

      if(write(fd, stringKey, strlen(stringKey)) != strlen(stringKey)){
         fprintf(stderr, "Write failure! iteration %d", i);
         exit(-1);
      }
  
      close(fd);
      md5file(passAndSalt, stringKey);

   }//end of for loop

   //Set up the formatting for strtoull to read the string as hexidecimal.
   firstHalf[0] = '0';
   firstHalf[1] = 'x';
   secondHalf[0] = '0';
   secondHalf[1] = 'x';

   strncpy(firstHalf +2, stringKey      , 16);

   //First half needs its own null character.
   firstHalf[18] = '\0';

   strncpy(secondHalf+2, stringKey + 16 , 17);

   //Our derived key//Interesting note: casting a returned long int value to unsigned limits it to 1/2 its highest value in hex
   key->highOrder  = strtoull(firstHalf, NULL, 0);  
   key->lowOrder  = strtoull(secondHalf, NULL, 0);  
   
}//end of pbkdf

void padMessage(char * string){
   char i = 0;
   char iterations = 0;
   int length = 0;

   //save initial length of string.
   length = strlen(string);

   iterations = 8 - (length % 8);
   
   for(i = 0; i < iterations; i++){
      string[length + i] = iterations; // << 4?
   }

   string[length + i] = '\0';
  
   printf("Padded message:");
   for(i = 0; i < strlen(string) ; i++){
      printf("%2x", string[i]);
   }   
   printf("\n");

}//end padMessage

unsigned long saltGen(unsigned seed1, unsigned seed2){
   unsigned long salt = 0;

   srand((unsigned)seed1);
   salt = rand();

   salt = salt << 32; 

   srand(seed2);
   salt += rand();

   return salt;

}//end saltGen
