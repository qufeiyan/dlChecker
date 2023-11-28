#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_PHILOSOPHERS  5
/* Naive dining philosophers with inconsistent lock acquisition
   ordering. */
static pthread_t phil[NUMBER_OF_PHILOSOPHERS];
static pthread_mutex_t chop[NUMBER_OF_PHILOSOPHERS];

void* dine ( void* arg )
{
   int i;
   long left = (long)arg;
   long right = (left + 1) % NUMBER_OF_PHILOSOPHERS;
   for (i = 0; i < 1000/*arbitrary*/; i++) {
      pthread_mutex_lock(&chop[left]);
      pthread_mutex_lock(&chop[right]);
      /* eating */
      pthread_mutex_unlock(&chop[right]);
      pthread_mutex_unlock(&chop[left]);

      // usleep(40 * 1000);
   }
   return NULL;
}

int main ( void )
{
   #include "interface.h" 
   initDeadlockChecker(1);

   long i;
   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
      pthread_mutex_init( &chop[i], NULL);

   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
      pthread_create(&phil[i], NULL, dine, (void*)i );

   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
      pthread_join(phil[i], NULL);
   return 0;
}