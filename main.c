#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int loopValue     = 10000;  // number of times a shared counter is incremented
int numThreads    = 100;    // number of threads created

// a lock, properly initialized (you can also use pthread_mutex_init() for this)
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// an example wrapper -- see if you can write your own 
// for the other functions (lock, unlock, join)!
void
Pthread_create(pthread_t  *thread, pthread_attr_t *attr, void * (*start_routine)(void *), void * arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);
    if (rc != 0) {
	perror("pthread_create");
	exit(1);
    }
}

// here is the function that the threads will run
// it takes a void ptr as an arg, and returns a void ptr
// you can use these void ptrs to pass arbitrary info
// in and out of the threads
void *
myfunc(void *ptr)
{
    // passed a pointer to an integer in, so unpack it
    int *p = (int *) ptr;
    int i;

    pthread_mutex_lock(&m);   // grab lock (XXX should use a wrapper instead!)
    for (i = 0; i < loopValue; i++) {
	*p = *p + 1;          // increment counter loopValue times
    }
    pthread_mutex_unlock(&m); // release lock (XXX should use a wrapper instead!)

    return NULL; // must return some pointer, but we're not using it, so return NULL
}

// the main routine just starts a bunch of threads and then
// waits for them to finish
int
main(int argc, char *argv[])
{
    pthread_t thread[numThreads];
    int x = 0;

    // Create independant threads each of which will execute function
    int i;
    for (i = 0; i < numThreads; i++) {
	Pthread_create(&thread[i], NULL, myfunc, (void *) &x);
    }

    // Wait till threads are complete before main continues. Unless we  
    // wait we run the risk of executing an exit which will terminate   
    // the process and all threads before the threads have completed.   
    for (i = 0; i < numThreads; i++) {
	pthread_join(thread[i], NULL); // (XXX should use a wrapper instead!)
    }

    printf("value of x is %d (should be %d) -- %s\n", 
	   x, loopValue * numThreads, 
	   (x == (loopValue * numThreads)) ? "correct!" : "wrong..."); // a puzzle for those new to C
    return 0;
}

