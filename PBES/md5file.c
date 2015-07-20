#include <stdio.h>
#include <fcntl.h>
#include "md5.h"

#define BUFSIZE 1024

int DEBUG = 0;

static void printDebug(char *s) {
   if (DEBUG)
       fprintf(stderr,"%s\n",s);
}
static void printDebug1(char *s, int n) {
   if (DEBUG)
       fprintf(stderr,"%s: %d\n",s,n);
}

// Convert the 16-byte bin to 32-byte hex string
static void binToHex(unsigned char* bin, char *result) {
   char *list = "0123456789abcdef";
   int i;
   unsigned char b1;
   unsigned char b2;
   for (i=0;i<16;i++) {
      b1 = bin[i]/16;
      b2 = bin[i]%16;
      result[2*i] = list[b1];
      result[2*i+1] = list[b2];
   }
   result[32] = 0;
}

// returns 0 on success, -1 on error
// result must be large enough (33 bytes) to hold the resulting hex string
int md5file(char *filename, char *result) {
   int fd;
   int bytesRead;
   MD5_CTX ctx;
   char buf[BUFSIZE];
   unsigned char bin[16];
   
   fd = open(filename,O_RDONLY);
   if (fd == -1)
      return -1;
   printDebug("file opened");
   MD5_Init(&ctx);
   while (1) {
      bytesRead = read(fd,buf,BUFSIZE);
      printDebug1("bytes read",bytesRead);
      if (bytesRead == 0) {
          printDebug("Got EOF");
          close(fd);
          MD5_Final(bin,&ctx);
          binToHex(bin,result);
          return 0;
      }
      if (bytesRead == -1) {
         printDebug("got error reading file");
         return -1;
      }
      MD5_Update(&ctx, buf, (unsigned long)bytesRead);
   }
   return 0;
}

