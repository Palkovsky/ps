#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>

// If WRITERS_COUNT*WRITER_TURNS != READERS_COUNT*READER_TURNS program won't terminate.
#define WRITERS_COUNT  4
#define WRITER_TURNS  4
#define READERS_COUNT 2
#define READER_TURNS  8

#define BUFF_CAP 6

// Conditional used for signaling free space for writer.
pthread_cond_t buff_notfull_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t buff_writer_mut = PTHREAD_MUTEX_INITIALIZER;

// Conditional used for informing reader that there's something to read.
pthread_cond_t buff_notempty_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t buff_reader_mut = PTHREAD_MUTEX_INITIALIZER;

// Conditional used for informing shared buffer about new data.
pthread_cond_t buff_update_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t buff_update_mut = PTHREAD_MUTEX_INITIALIZER;


// buff_wr represents data queued for shared buffer.
// can't be greater than BUFF_CAP
int buff_wr = 0;
// buff_rd represents data availablie to read for readers
// can't be greater than BUFF_CAP
int buff_rd = 0;

useconds_t
random_time(long max) {
  return rand()%max;
}

// Prints how many elements in write and read buffer.
void
status(void) {
  printf("WR: %d/%d, RD: %d/%d\n", buff_wr, BUFF_CAP, buff_rd, BUFF_CAP);
}

// Writer thread function
int
writer(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i < WRITER_TURNS; i++) {
    printf("WRITER(%d): Starting turn %d/%d. ", threadId, i+1, WRITER_TURNS);
    status();

    pthread_mutex_lock(&buff_writer_mut);
    while(buff_wr == BUFF_CAP) {
      printf("WRITER(%d): Write buffer full. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&buff_notfull_cond, &buff_writer_mut);
    }

    printf("WRITER(%d): Write is now possible. Writing to buffer. ", threadId);
    status();

    // There is something to write.
    usleep(random_time(800));
    buff_wr++;

    printf("WRITER(%d): Write finished. Signaling new data. ", threadId);
    status();

    pthread_mutex_unlock(&buff_writer_mut);

    // Inform shared buff about new data available.
    pthread_cond_signal(&buff_update_cond);

    usleep(random_time(1000));
  }

  free(data);
  return 0;
}

// Shared buff moves data from write buffer to read buffer.
// Writers and readses share their respective buffer through mutex.
// Shared buff, to perfom move from write to read buffers must lock both.
int
shared_buff(void* data) {
  for (;;) {
    pthread_mutex_lock(&buff_update_mut);

    while(buff_rd >= BUFF_CAP || buff_wr == 0) {
      printf("SHARED BUFF: Awaiting for new data. ");
      status();

      pthread_cond_wait(&buff_update_cond, &buff_update_mut);
    }
    pthread_mutex_unlock(&buff_update_mut);

    printf("SHARED BUFF: Transfer is now possible. Awaitng buffers lock. ");
    status();

    // Two locks needed for transfer.
    pthread_mutex_lock(&buff_writer_mut);
    pthread_mutex_lock(&buff_reader_mut);

    printf("SHARED BUFF: Buffer lock acquired. Moving data.");
    status();

    // There is soma data to be moved to rd buff.
    usleep(random_time(800));
    while(buff_rd < BUFF_CAP && buff_wr > 0) {
      buff_wr--;
      buff_rd++;
    }

    printf("SHARED BUFF: Data moved. Singaling to writers and readers and releasing lock. ");
    status();

    // Inform writers about free space
    pthread_cond_broadcast(&buff_notfull_cond);
    // Inform readers about new available data
    pthread_cond_broadcast(&buff_notempty_cond);

    pthread_mutex_unlock(&buff_reader_mut);
    pthread_mutex_unlock(&buff_writer_mut);
  }

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

    pthread_mutex_lock(&buff_reader_mut);
    while(buff_rd == 0) {
      printf("READER(%d): Read buffer empty. Awaiting. ", threadId);
      status();
      pthread_cond_wait(&buff_notempty_cond, &buff_reader_mut);
    }

    printf("READER(%d): Read is now possible. Reading. ", threadId);
    status();

    // There is something to read.
    usleep(random_time(800));
    buff_rd--;

    printf("READER(%d): New data read. Singaling result. ", threadId);
    status();

    pthread_mutex_unlock(&buff_reader_mut);

    // Inform shared buff about free space in rd_buff.
    pthread_cond_signal(&buff_update_cond);

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
    pthread_t sharedBuffThread;

    int i, rc;

    // Create shared buff thread
    rc = pthread_create(&sharedBuffThread,
                        NULL,
                        (void*) shared_buff,
                        NULL);
    if (rc != 0)
      {
        fprintf(stderr,"Couldn't create the writer thread");
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
