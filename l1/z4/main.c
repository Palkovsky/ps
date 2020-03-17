#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

#define WRITERS_COUNT  4
#define WRITER_TURNS   4

#define READERS_COUNT  2
#define READER_TURNS   2

// Buffer mutex. Only one exclusvie reader or writer.
pthread_mutex_t buffer_mut = PTHREAD_MUTEX_INITIALIZER;
// Conditional for critic notification.
pthread_mutex_t critic_mut  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  critic_cond = PTHREAD_COND_INITIALIZER;
// Writer sets sender_id before signaling it to critic.
int sender_id = -1;

// Prints buffer statuses.
void
status(void) {
  printf("\n");
}

useconds_t
random_time(long max) {
  return rand()%max;
}

// Writer thread function
int
writer(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i < WRITER_TURNS; i++) {
    printf("WRITER(%d): Starting turn %d/%d.\n", threadId, i+1, WRITER_TURNS);

    pthread_mutex_lock(&buffer_mut);
    printf("WRITER(%d): Acquired buffer lock. Writing.\n", threadId);
    usleep(random_time(1000));

    printf("WRITER(%d): Write finished. Notifying critic.\n", threadId);

    // Notify critic.
    pthread_mutex_lock(&critic_mut);
    sender_id = threadId;
    pthread_cond_signal(&critic_cond);
    pthread_mutex_unlock(&critic_mut);

    printf("WRITER(%d): Critic notified. Releasing buffer lock.\n", threadId);

    // Don't unlock this mutex here.
    // Notified critic will handle this.
    // pthread_mutex_unlock(&buffer_mut);

    usleep(random_time(1000));
  }

  free(data);
  return 0;
}

int
critic(void* data) {
  for(;;) {
    pthread_mutex_lock(&critic_mut);
    while(sender_id == -1) {
      pthread_cond_wait(&critic_cond, &critic_mut);
    }
    printf("CRITIC: Writer %d just performed write.\n", sender_id);
    sender_id = -1;

    printf("CRITIC: Writing data.\n");
    usleep(800);
    printf("CRITIC: Write finished.\n");

    // Unlock buffer_mut, initally locked by writer.
    pthread_mutex_unlock(&buffer_mut);
    pthread_mutex_unlock(&critic_mut);

  }
  return 0;
}

// Reader thread function
int
redaer(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i < READER_TURNS; i++) {
    printf("READER(%d): Starting turn %d/%d.\n", threadId, i+1, READER_TURNS);

    pthread_mutex_lock(&buffer_mut);
    printf("READER(%d): Acquired buffer lock. Reading.\n", threadId);

    usleep(random_time(1000));

    printf("READER(%d): Read finished. Releasing buffer lock.\n", threadId);
    pthread_mutex_unlock(&buffer_mut);

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
    pthread_t criticThread;

    int i, rc;

    rc = pthread_create(&criticThread,
                        NULL,
                        (void*) critic,
                        NULL);
    if (rc != 0)
      {
        fprintf(stderr,"Couldn't create the critic thread.");
        exit (-1);
      }

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
