#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "md5file.h"
#include "md5.h"
#include <pthread.h>
#ifndef PATH_MAX       
#define PATH_MAX 255
#endif
#define MILLION 1000000L
#define NANOSECONDS 1000
#define TRUE 1

typedef struct dynamic_t {
   char **start;
   int count;
   int max;
   int error;
   int t_created;
   int t_finished;
   pthread_mutex_t mylock;

} dynamic_t;

//For passing in the arguments for md5file in a thread
typedef struct mdparam{
   char * filename;
   char * result;
   dynamic_t * link;

} mdparam;

void ftw(const char *filepath, int (*filefunc)(char *, void *), void *ptr);
int serialize(char *filepath, void *dynamic);


void *md5thread(void * arguments);
int md5file(char *filename, char *result);

void adddynamic(dynamic_t *listp, char *s);
void printdynamic(dynamic_t *listp);

static int dAmount; //number of directories.
static int fAmount; //number of files processed (including directories). 

int main (const int argc, const char * argv[]){
   long timedif1 = 0;
   long timedif2 = 0;
   struct timeval tpstart;
   struct timeval tpend;
   struct timespec nano;


   int i = 0;
   dynamic_t list;
   list.count = 0;
   list.max = 0;
   list.error = 0;
   list.t_created = 0;
   list.t_finished = 0;

   nano.tv_sec = 0;
   nano.tv_nsec = NANOSECONDS;
   pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

   //Copy the mutex into my dynamic struct, since the INITIALIZER
   //Is only valid when initializing the mutex...meaning that because
   //"mylock" has been written earlier, I'm not allowed to INITIALIZER it. 
   list.mylock = lock;


   if(gettimeofday(&tpstart, NULL)){
      fprintf(stderr, "Couldn't get start time!\n");
      return 1;
   }
   ftw(argv[1], serialize, &list);

   while(TRUE){

      pthread_mutex_lock(&list.mylock);
         if(list.t_created == list.t_finished){
            break;
         }
      pthread_mutex_unlock(&list.mylock);
      nanosleep(&nano, NULL);
   }

   if(gettimeofday(&tpend, NULL)){
      fprintf(stderr, "Couldn't get end time!\n");
      return 1;
   }

   timedif1 = MILLION *(tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
   fprintf(stderr, "ftw function took %ld microseconds\n", timedif1);


   if(gettimeofday(&tpstart, NULL)){
      fprintf(stderr, "Couldn't get start time!\n");
      return 1;
   }
   
   printdynamic(&list);

   if(gettimeofday(&tpend, NULL)){
      fprintf(stderr, "Couldn't get end time!\n");
      return 1;
   }

   timedif2 = MILLION *(tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;

   fprintf(stderr, "ftw function took %ld microseconds\n", timedif1);
   fprintf(stderr, "printdynamic function took %ld microseconds\n", timedif2);


   //Free mallocce'd space
   for(i = 0; i < list.count; i++){
      free(list.start[i]);
   }

   //free mallocce'd pointers   
   free(list.start);
 

   return 0;
}





//When ftw is over, it should print two descriptive lines to stderr giving
//The number of files processed and the number of directories processed. 
void ftw(const char *filepath, int (*filefunc)(char *, void *), void *ptr){
   DIR * dirpath = NULL;  
   char * fullpath = NULL;
   struct dirent entry;
   struct dirent * result = NULL;
   static int first = 1; //Boolean to see if this is the original device or not.
   int original = 0;

   if(first == 1){
      original = 1;
      first = 0;
      filefunc((char *)filepath, ptr);
   }

   fullpath = realpath(filepath, NULL);
   dirpath = opendir(fullpath);
  
   dAmount++;

   chdir(fullpath);
   readdir_r(dirpath, &entry, &result);
   do{

      //Skip anything that is only "." or ".."
      if(strcmp(".", entry.d_name) == 0 || strcmp("..", entry.d_name) == 0){

         readdir_r(dirpath, &entry, &result);
         continue;
      }
      if(filefunc(entry.d_name, ptr) == 1){  //This is serialize1

         ftw(entry.d_name, serialize, ptr);
         chdir(fullpath); //Restore to previous working directory
      }
      else{
         fAmount++;        
      }

      readdir_r(dirpath, &entry, &result);

   }while(result);


   if(original){
      fprintf(stderr, "Number of files: %d\nNumber of directories: %d\n", 
      fAmount, dAmount);
   }

   closedir(dirpath);
   free(fullpath);
}



int serialize(char *filepath, void *dynamic){
   
   FILE * none = NULL;

   none = fopen("/dev/null", "w");
   char * temp = NULL;
   //Let the code know what datatype this is.
   dynamic_t * ptr = (dynamic_t *) dynamic;
   
   mdparam * threadVar = malloc(sizeof(mdparam));
   pthread_t tid = 0;

   int length = 0;
   char thiscwd [PATH_MAX];
   char * md5 = NULL;
   char * string = NULL;
   char * filename = NULL;

   struct stat info;
   int status = 0;
   int boolean = 0;

   mode_t mask = 0;
   mask = S_IROTH | S_IWOTH ;
  
   if(getcwd(thiscwd, PATH_MAX) == ""){
      fprintf(stderr, "Couldn't get current working directory!\n");
      exit(1);
   }

   //Append current working directory with the file's name!
   temp = thiscwd;

   if(temp[strlen(temp) - 1] != '/'){
      strcat(temp, "/");
   }

   strcat(temp, filepath);

   if(temp[strlen(temp) - 1] == '/'){
      temp[strlen(temp) - 1] == '\0';
   }

   if(lstat(filepath, &info) < 0){
      free(threadVar);
      status = 0;
   }
   else if( S_ISDIR(info.st_mode)){
      free(threadVar);
      status = 1;
   }
   else if( S_ISREG(info.st_mode)){
      status = 0;
      //If the file's size is small enough, or if the number of 
      //running threads exceeds 50...
     
      pthread_mutex_lock(&ptr->mylock);

      if(info.st_size < 100000 || (ptr->t_created - ptr->t_finished) >= 50){
         boolean = 1; //mark it so that it won't be made into a thread. 
         free(threadVar); //Won't need a static struct 
                          //if the thread isn't being made. 
      }
      else{//set up the path name  to be passed into the thread. 
         filename = malloc(strlen(temp) + 1);
         strcpy(filename, temp);
      }
      pthread_mutex_unlock(&ptr->mylock);


   }
   else if( S_ISLNK(info.st_mode)){
      free(threadVar);
      status = 0;
   }
   else{
      free(threadVar);
      status = 0;
   }
   //If the permissions are set to owner-only
   if((info.st_mode & mask) == 0){
      status = 0;
   }
  
   length = fprintf(none,"%s/// %lld %lld %lld %lld %lld %lld %lld ", 
   temp, (long long) info.st_dev, (long long) info.st_mode, 
   (long long) info.st_uid, (long long) info.st_gid, (long long) info.st_size, 
   (long long) info.st_mtime, (long long) info.st_ctime);
                              //Including md5sum!
   string = malloc(length + 33);

   sprintf(string, "%s/// %lld %lld %lld %lld %lld %lld %lld ", 
   temp, (long long) info.st_dev,(long long) info.st_mode, 
   (long long) info.st_uid, (long long) info.st_gid, (long long) info.st_size, 
   (long long) info.st_mtime, (long long) info.st_ctime);
  
   md5 = string + length;

   //Set up all the md5 digits in serialize to 32 1's as default
   memset(md5, '1', 32);
   string[length + 32] = '\0';


   adddynamic(ptr, string);
   

   //Increment count of thread's made.
   if(S_ISREG(info.st_mode)){
   
      if(boolean == 0){
 
         threadVar->filename = filename;
         threadVar->result = md5;
         threadVar->link = ptr;

         ptr->t_created++;  

         if( pthread_create( &tid, NULL,  md5thread, threadVar)){
            //finished thread increments from error... 
            fprintf(stderr, "pthread failure");

           
            free(filename);
            free(threadVar);
            pthread_mutex_lock(&ptr->mylock);
               ptr->t_finished++;
            pthread_mutex_unlock(&ptr->mylock);
         }
      }
      else{
         md5file(temp, md5);
      }
   }

   fclose(none);

   return status;
}

void * md5thread(void * arguments){
   
   mdparam * ptr = (mdparam *) arguments;
   md5file(ptr->filename, ptr->result);

   pthread_mutex_lock(&ptr->link->mylock);
      ptr->link->t_finished++;
   pthread_mutex_unlock(&ptr->link->mylock);

      free(ptr->filename);
      free(ptr);


   if (!pthread_detach(pthread_self())){
      return;
   }
}

void adddynamic(dynamic_t *listp, char *s){
   static int amount = 500;
   if(listp->count == 0){//initialize the struct to take 500 entries. 
      if((listp->start =  malloc(sizeof(char *) * amount)) == NULL){
         listp->error = 1;
         return;
      }
      listp->max = amount;
   }
   else if( listp->count == listp->max){ 
      amount *= 2; //The amount of memory pointed by start will be doubled!
      if((listp->start =  realloc(listp->start, sizeof(char *) * amount)) == NULL){
         listp->error = 1;
         return;
      }

      listp->max = amount;//max number of elements is updated.
   }
   listp->start[listp->count] = s;
   listp->count++;  

}

void  printdynamic(dynamic_t *listp){
   int i = 0;
   for(i = 0; i < listp->count; i++){
      printf("%s\n", listp->start[i]);
   }
   
   printf("Number of entries: %d\nMaximum limit: %d\n", listp->count, listp->max);
   if(listp->error){
      printf("An error happened while working with this list.\n");
   }

}


