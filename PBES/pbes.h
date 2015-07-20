#include "md5file.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>


//16 bytes that will store the md5sum
struct mdHash{
   uint64_t highOrder;
   uint64_t lowOrder;
};

union keyBuffer
{
   unsigned short K[64];
   unsigned char L[128];
};

union text
{
   unsigned short R[4];
   unsigned char M[8];
};

//Password-based key derivation function                     
void pbkdf(char * password, long unsigned salt, unsigned iterations,  struct mdHash * key);

//Generate the salt with a random number generator. For now I'm assuming a party won't take 
//advantage of reusing our salt.
unsigned long saltGen(unsigned seed1, unsigned seed2);

//Make sure the length of the message is divisible by 8, fill in empty space to make it so.
void padMessage(char * string);

void rc2Init(uint64_t key, uint64_t iv, char * message);
void rc2Chain(char option);

char checkIfUsed(unsigned char * pointer, unsigned char randval);
void piTable(unsigned char * table);

void rol(unsigned short * number, unsigned short  amount);
void ror(unsigned short * number, unsigned short  amount);

void mix(char option);
void mash(char option);
