#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

/*
  #5

  N writers, M readers, K buffers

  Each buffer has Ki_max, which stands for maximum simultaneous reader threads.
  This exercise is combination of exercies #1 and #3
*/

#define WRITERS_COUNT  2
#define WRITER_TURNS  4

#define READERS_COUNT 4
#define READER_TURNS  4

#define BUFF_COUNT 3

// States
#define NO_LOCK 0
#define LOCK_BY_WRITER 1
#define LOCK_BY_READER 2

// Similar to #1 but with arrays.
pthread_mutex_t readers_mutex[BUFF_COUNT] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t writers_mutex[BUFF_COUNT] = { PTHREAD_MUTEX_INITIALIZER };
const int readers_max[BUFF_COUNT] = { 1, 2, 1 }; // hardcoded values
int readers_count[BUFF_COUNT] = { 0 };
int buffers_status[BUFF_COUNT] = { NO_LOCK };

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
      printf("FREE");
      break;
    case LOCK_BY_WRITER:
      printf("LOCK BY WRITER");
      break;
    default:
      printf("LOCK BY READER. COUNT: %d/%d", readers_count[i], readers_max[i]);
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

// Locks buffer as writer.
// -1 if success
// buff_idx, otherwise
int
try_lock_writer(void) {
  for(int i=0; i<BUFF_COUNT; i++) {
    if (pthread_mutex_trylock(writers_mutex+i) == 0) {
      buffers_status[i] = LOCK_BY_WRITER;
      return i;
    }
  }
  return -1;
}

void
unlock_writer(int idx) {
  buffers_status[idx] = NO_LOCK;
  pthread_mutex_unlock(writers_mutex+idx);
}

// Locks buffer as reader. It will handle increming readers count.
// -1 if success
// buff_idx, otherwise
int
try_lock_reader(void) {
  for(int i=0; i<BUFF_COUNT; i++) {
    if (pthread_mutex_trylock(readers_mutex+i) != 0) {
      continue;
    }
    if(readers_count[i] >= readers_max[i]) {
      goto release_mutex;
    }
    readers_count[i]++;
    if(readers_count[i] != 1) {
      goto success;
    }
    if(pthread_mutex_trylock(writers_mutex+i) != 0) {
      goto decrement_count;
    }
    buffers_status[i] = LOCK_BY_READER;
    goto success;

  decrement_count:
    readers_count[i]--;
  release_mutex:
    pthread_mutex_unlock(readers_mutex+i);
    continue;

  success:
    pthread_mutex_unlock(readers_mutex+i);
    return i;
  }

  return -1;
}

void
unlock_reader(int idx) {
  pthread_mutex_lock(readers_mutex+idx);
  readers_count[idx]--;

  if(readers_count[idx] == 0) {
    pthread_mutex_unlock(writers_mutex+idx);
    buffers_status[idx] = NO_LOCK;
  }

  pthread_mutex_unlock(readers_mutex+idx);
}

// Writer thread function
int
writer(void* data) {
  int i;
  int buff_idx;
  int threadId = *(int*) data;

  for (i = 0; i < WRITER_TURNS; i++) {
    printf("WRITER(%d): Starting turn %d/%d. ", threadId, i+1, WRITER_TURNS);
    status();

    // Try locking as writer. Otherwise wait for notification.
    pthread_mutex_lock(&await_mut);
    while((buff_idx = try_lock_writer()) < 0) {
      pthread_cond_wait(&await_cond, &await_mut);
    }
    pthread_mutex_unlock(&await_mut);

    printf("WRITER(%d): Acquired buffer %d. Writing. ", threadId, buff_idx);
    status();

    usleep(random_time(1500));

    printf("WRITER(%d): Write done. Releasing buffer %d. ", threadId, buff_idx);
    status();

    // Unlock and notify awaiting threads.
    unlock_writer(buff_idx);
    pthread_cond_broadcast(&await_cond);

    printf("WRITER(%d): Buffer %d released. ", threadId, buff_idx);
    status();

    buff_idx = -1;
    usleep(random_time(3000));
  }

  free(data);
  return 0;
}

// Reader thread function
int
reader(void* data) {
  int i;
  int buff_idx;
  int threadId = *(int*) data;

  for (i = 0; i < READER_TURNS; i++) {
    printf("READER(%d): Starting turn %d/%d. ", threadId, i+1, READER_TURNS);
    status();

    // Try locking as reader. Otherwise wait for notification.
    pthread_mutex_lock(&await_mut);
    while((buff_idx = try_lock_reader()) < 0) {
      pthread_cond_wait(&await_cond, &await_mut);
    }
    pthread_mutex_unlock(&await_mut);

    printf("READER(%d): Acquired buffer %d. Reading. ", threadId, buff_idx);
    status();

    usleep(random_time(1500));

    printf("READER(%d): Read done. Releasing buffer %d. ", threadId, buff_idx);
    status();

    // Unlock and notify awaiting threads.
    unlock_reader(buff_idx);
    // Could be optimized.
    // There's no sense in notifying if readers_count is still greater than 0.
    pthread_cond_broadcast(&await_cond);

    printf("READER(%d): Buffer %d released. ", threadId, buff_idx);
    status();

    buff_idx = -1;
    usleep(random_time(3000));
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
        printf("READER(%d): Thread finished. ", i+1);
    }

    // Wait for the Writer
    for (i = 0; i < WRITERS_COUNT; i++) {
      pthread_join(writerThreads[i], NULL);
      printf("WRITER(%d): Thread finished.\n", i+1);
    }

    return (0);
}
