#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

/*
  #6

  N writers, M threads, 1 buffer of K size

  Assumption: Only one thread can have exclusive access to the buffer.

  Readers can start reading from buffer only when it's full.
  Writers can start writing to buffer only when it's empty.
  Writers and readers notify each other using conds.

  Conditions required for program to terminate:
    WRITERS_COUNT*WRITERS_TURNS = READERS_COUNT*READER_TURNS
    (WRITERS_COUNT*WRITERS_TURNS or READERS_COUNT*READER_TURNS)%BUFF_CAP = 0
*/
#define WRITERS_COUNT  4
#define WRITER_TURNS   4

#define READERS_COUNT  2
#define READER_TURNS   8

#define BUFF_CAP       4

pthread_mutex_t buff_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t reader_notify_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t writer_notify_cond = PTHREAD_COND_INITIALIZER;

int buff_sz = 0;

#define WRITING 0
#define READING 1

int buff_status = WRITING;

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

    // Get notified when buff_status changes to WRITING
    pthread_mutex_lock(&buff_mut);
    while(buff_status == READING) {
      printf("WRITER(%d): Write not possible. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&writer_notify_cond, &buff_mut);
    }

    printf("WRITER(%d): Acquired lock on the buffer. Writing to buffer. ", threadId);
    status();

    // Perform write
    usleep(random_time(1500));
    buff_sz++;

    printf("WRITER(%d): Update of buff: %d->%d. ", threadId, buff_sz-1, buff_sz);
    status();

    // If reached full buffer capacity, change buff_status to READING
    // and notify via reader_notify_cond
    if(buff_sz == BUFF_CAP) {
      buff_status = READING;
      pthread_cond_signal(&reader_notify_cond);

      printf("WRITER(%d): That was last write. Giving control to readers. ", threadId);
      status();
    }
    // Otherwise, notify other writer thread.
    else {
      pthread_cond_signal(&writer_notify_cond);
    }

    printf("WRITER(%d): Releasing buffer. ", threadId);
    status();

    // Unlock to allow other thread to access this buffer.
    pthread_mutex_unlock(&buff_mut);

    usleep(random_time(3000));
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
    while(buff_status == WRITING) {
      printf("READER(%d): Read not possible. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&reader_notify_cond, &buff_mut);
    }

    printf("READER(%d): Acquired lock on the buffer. Reading. ", threadId);
    status();

    usleep(random_time(1500));
    buff_sz--;

    printf("READER(%d): Update of buff: %d->%d. ", threadId, buff_sz+1, buff_sz);
    status();

    // If buffer empty, change state to WRITINT
    // and notify via reader_notify_cond
    if(buff_sz == 0) {
      buff_status = WRITING;
      pthread_cond_signal(&writer_notify_cond);

      printf("READER(%d): That was last read. Giving control to writers. ", threadId);
      status();
    }
    // Otherwise, notify other reader threads.
    else {
      pthread_cond_signal(&reader_notify_cond);
    }

    printf("READER(%d): Releasing buffer. ", threadId);
    status();

    // Unlock to allow other thread to access this buffer.
    pthread_mutex_unlock(&buff_mut);

    usleep(random_time(30000));
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
