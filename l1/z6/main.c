#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>

// Conditions required for program to terminate:
// WRITERS_COUNT*WRITERS_TURNS = READERS_COUNT*READER_TURNS
// (WRITERS_COUNT*WRITERS_TURNS or READERS_COUNT*READER_TURNS)%BUFF_CAP = 0
#define WRITERS_COUNT  4
#define WRITER_TURNS   4
#define READERS_COUNT  2
#define READER_TURNS   8

#define BUFF_CAP       4

pthread_mutex_t buff_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t reader_notify_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t writer_notify_cond = PTHREAD_COND_INITIALIZER;
int buff_sz = 0;
int writing = 1;

useconds_t
random_time(long max) {
  return rand()%max;
}

// Prints how many elements in write and read buffer.
void
status(void) {
  printf("BUFF SIZE: %d/%d\n", buff_sz, BUFF_CAP);
}

// Writer thread function
int
writer(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i < WRITER_TURNS; i++) {
    printf("WRITER(%d): Starting turn %d/%d. ", threadId, i+1, WRITER_TURNS);
    status();

    pthread_mutex_lock(&buff_mut);
    while(writing == 0) {
      printf("WRITER(%d): Write not possible. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&writer_notify_cond, &buff_mut);
    }

    printf("WRITER(%d): Write is now possible. Writing to buffer. ", threadId);
    status();

    usleep(random_time(800));
    buff_sz++;

    printf("WRITER(%d): Update of buff: %d->%d. ", threadId, buff_sz-1, buff_sz);
    status();

    if(buff_sz == BUFF_CAP) {
      writing = 0;
      pthread_cond_signal(&reader_notify_cond);

      printf("WRITER(%d): That was last write. Giving control to readers. ", threadId);
      status();
    } else {
      pthread_cond_signal(&writer_notify_cond);
    }

    pthread_mutex_unlock(&buff_mut);
    usleep(random_time(1000));
  }

  free(data);
  return 0;
}

// Reader thread function
int
reader(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i<READER_TURNS; i++) {
    printf("READER(%d): Starting turn %d/%d. ", threadId, i+1, READER_TURNS);
    status();

    pthread_mutex_lock(&buff_mut);
    while(writing == 1) {
      printf("READER(%d): Read not possible. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&reader_notify_cond, &buff_mut);
    }

    printf("READER(%d): Read is now possible. Reading. ", threadId);
    status();

    usleep(random_time(800));
    buff_sz--;

    printf("READER(%d): Update of buff: %d->%d. ", threadId, buff_sz+1, buff_sz);
    status();

    if(buff_sz == 0) {
      writing = 1;
      pthread_cond_signal(&writer_notify_cond);

      printf("READER(%d): That was last read. Giving control to writers. ", threadId);
      status();
    } else {
      pthread_cond_signal(&reader_notify_cond);
    }

    pthread_mutex_unlock(&buff_mut);
    usleep(random_time(1000));
  }

  free(data);
  return 0;
}

int
main(int argc, char* argv[]) {
    srand(100005);

    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];

    int i, rc;

    // Create the Writer thread
    for(i=0; i < WRITERS_COUNT; i++) {
      int* threadId = malloc(sizeof(int));
      *threadId = i+1;
      rc = pthread_create(&writerThreads[i],
                          NULL,
                          (void*) writer,
                          (void*) threadId);
      if (rc != 0)
        {
          fprintf(stderr,"Couldn't create the writer thread");
          exit (-1);
        }
    }

    // Create the Reader threads
    for (i = 0; i < READERS_COUNT; i++) {
      int* threadId = malloc(sizeof(int));
      *threadId = i+1;
      rc = pthread_create(&readerThreads[i],
                          NULL,
                          (void*) reader,
                          (void*) threadId);

      if (rc != 0) {
        fprintf(stderr,"Couldn't create the reader threads");
        exit (-1);
      }
    }

    // Wait for the Readers
    for (i = 0; i < READERS_COUNT; i++) {
        pthread_join(readerThreads[i],NULL);
        printf("READER(%d): Thread finished.\n", i+1);
    }

    // Wait for the Writer
    for (i = 0; i < WRITERS_COUNT; i++) {
      pthread_join(writerThreads[i], NULL);
      printf("WRITER(%d): Thread finished.\n", i+1);
    }

    return (0);
}
