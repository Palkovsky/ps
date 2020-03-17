#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>

#define WRITERS_COUNT  4
#define WRITER_TURNS  4
#define READERS_COUNT 3
#define READER_TURNS  8

// Number of shared resources.
#define BUFF_COUNT 3

#define NO_LOCK 0
#define LOCK_BY_WRITER 1
#define LOCK_BY_READER 2

pthread_mutex_t buffers[BUFF_COUNT] = { PTHREAD_MUTEX_INITIALIZER };
int             buffers_status[BUFF_COUNT] = { 0 };

// For awaiting on buffer release.
pthread_cond_t  await_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t await_mut  = PTHREAD_MUTEX_INITIALIZER;

// Prints buffer statuses.
void
status(void) {
  for(int i=0; i<BUFF_COUNT; i++) {
    printf("%d - ", i);
    switch(buffers_status[i]) {
    case NO_LOCK:
      printf("AVAILABLE");
      break;
    case LOCK_BY_WRITER:
      printf("LOCK BY WRITER");
      break;
    case LOCK_BY_READER:
      printf("LOCK BY READER");
      break;
    }
    printf(", ");
  }
  printf("\n");
}

useconds_t
random_time(long max) {
  return rand()%max;
}

int
try_lock(void) {
  for(int i=0; i<BUFF_COUNT; i++) {
    if (pthread_mutex_trylock(buffers+i) == 0) {
      return i;
    }
  }
  return -1;
}


// Writer thread function
int
writer(void* data) {
  int i;
  int buff_idx;
  int threadId = *(int*) data;

  for (i = 0; i < WRITER_TURNS; i++) {
    printf("WRITER(%d): Starting turn %d/%d.\n", threadId, i+1, WRITER_TURNS);
    status();

    pthread_mutex_lock(&await_mut);
    while((buff_idx = try_lock()) < 0) {
      pthread_cond_wait(&await_cond, &await_mut);
    }
    buffers_status[buff_idx] = LOCK_BY_WRITER;
    pthread_mutex_unlock(&await_mut);

    printf("WRITER(%d): Acquired buffer %d. Writing.\n", threadId, buff_idx);
    status();

    usleep(random_time(800));

    printf("WRITER(%d): Write done. Releasing buffer %d.\n", threadId, buff_idx);
    status();

    buffers_status[buff_idx] = NO_LOCK;
    pthread_mutex_unlock(&buffers[buff_idx]);
    pthread_cond_signal(&await_cond);

    printf("WRITER(%d): Buffer %d released.\n", threadId, buff_idx);
    status();

    buff_idx = -1;
  }

  free(data);
  return 0;
}

// Reader thread function
int
redaer(void* data) {
  int i;
  int buff_idx;
  int threadId = *(int*) data;

  for (i = 0; i < READER_TURNS; i++) {
    printf("READER(%d): Starting turn %d/%d.\n", threadId, i+1, READER_TURNS);
    status();

    pthread_mutex_lock(&await_mut);
    while((buff_idx = try_lock()) < 0) {
      pthread_cond_wait(&await_cond, &await_mut);
    }
    buffers_status[buff_idx] = LOCK_BY_READER;
    pthread_mutex_unlock(&await_mut);

    printf("READER(%d): Acquired buffer %d. Reading.\n", threadId, buff_idx);
    status();

    usleep(random_time(800));

    printf("READER(%d): Read done. Releasing buffer %d.\n", threadId, buff_idx);
    status();

    buffers_status[buff_idx] = NO_LOCK;
    pthread_mutex_unlock(&buffers[buff_idx]);
    pthread_cond_signal(&await_cond);

    printf("READER(%d): Buffer %d released.\n", threadId, buff_idx);
    status();

    buff_idx = -1;
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
                          (void*) redaer,
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
