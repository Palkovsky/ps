#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<pthread.h>
#include<semaphore.h>

/*
  #1
  Multiple readers. One writer.

  Multiple readers can access buffer at the same time.
  Implementation of pseudocode from the website.
 */

#define WRITER_TURNS  3
#define READERS_COUNT 2
#define READER_TURNS  5

sem_t reader_sem, writer_sem;
int readers_count;

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

    // wait(writer_sem)
    printf("WRITER(%d): Awaiting writer semaphore.\n", threadId);

    int result = sem_wait(&writer_sem);
    if (result != 0) {
      fprintf(stderr, "WRITER(%d): Error occured during locking the semaphore.\n", threadId);
      exit (-1);
    } else {
	    // Write
	    printf("WRITER(%d): Started writing. %d/%d\n", threadId, i+1, WRITER_TURNS);
	    fflush(stdout);
	    usleep(random_time(2000));
	    printf("WRITER(%d): Write %d/%d finished.\n", threadId, i+1, WRITER_TURNS);

      // release(writer_sem)
      printf("WRITER(%d): Releasing writer semaphore.\n", threadId);
      result = sem_post(&writer_sem);
      if (result != 0) {
        fprintf(stderr, "WRITER(%d): Error occured during unlocking the semaphore.\n", threadId);
        exit (-1);
      }

	    usleep(random_time(2000));
    }
  }

  return 0;
}

// Reader thread function
int
reader(void* data) {
  int i;
  int threadId = *(int*) data;

  for (i = 0; i<READER_TURNS; i++) {
    printf("READER(%d): Starting turn %d/%d.\n", threadId, i+1, READER_TURNS);
    // wait(reader)
    printf("READER(%d): Awaiting reader semaphore.\n", threadId);
    if (sem_wait(&reader_sem) != 0) {
      fprintf(stderr, "READER(%d): Error occured during locking the reader semaphore.\n", threadId);
      exit(-1);
    }
    printf("READER(%d): readers_count changed value %d->%d\n", threadId, readers_count, readers_count+1);
    readers_count++;
    if(readers_count == 1)
    {
      printf("READER(%d): Awaiting writer semaphore...\n", threadId);
      if(sem_wait(&writer_sem) != 0) {
        // wait(writer)
        fprintf(stderr, "READER(%d): Error occured during locking the writer semaphore.\n", threadId);
        exit(-1);
      } else {
        printf("READER(%d): Acquired writer semaphore.\n", threadId);
      }
    }
    // release(reader)
    printf("READER(%d): Releasing reader semaphore.\n", threadId);
    if(sem_post(&reader_sem) != 0) {
      fprintf(stderr, "READER(%d): Error occured during releasing the reader semaphore.\n", threadId);
      exit(-1);
    }

    // Read
    printf("READER(%d): Started reading. %d/%d\n", threadId, i+1, READER_TURNS);
    fflush(stdout);
    // Read, read, read
    usleep(random_time(2000));
    printf("READER(%d): Read %d/%d finished.\n", threadId, i+1, READER_TURNS);

    // wait(reader)
    printf("READER(%d): Awaiting reader semaphore.\n", threadId);
    if (sem_wait(&reader_sem) != 0) {
      fprintf(stderr, "READER(%d): Error occured during locking the reader semaphore.\n", threadId);
      exit(-1);
    }
    printf("READER(%d): readers_count changed value %d->%d\n", threadId, readers_count, readers_count-1);
    readers_count--;
    if(readers_count == 0) {
      // release(writer)
      printf("READER(%d): Releasing writer semaphore.\n", threadId);
      if(sem_post(&writer_sem) != 0) {
        fprintf(stderr, "READER(%d): Error occured during releasing the writer semaphore.\n", threadId);
        exit(-1);
      }
    }
    // release(reader)
    printf("READER(%d): Releasing reader semaphore.\n", threadId);
    if(sem_post(&reader_sem) != 0) {
      fprintf(stderr, "READER(%d): Error occured during releasing the reader semaphore.\n", threadId);
      exit(-1);
    }
  }

  free(data);

  return 0;
}

int
main(int argc, char* argv[]) {
    srand(100005);

    pthread_t writerThread;
    pthread_t readerThreads[READERS_COUNT];

    int i, rc;

    // Initialize writer and reader semaphores
    if (sem_init(&reader_sem, 0, 1) != 0 || sem_init(&writer_sem, 0, 1) != 0)
    {
      fprintf(stderr,"Couldn't create reader or writer semaphore.");
      exit(-1);
    }
    readers_count = 0;

    // Create the Writer thread
    int* threadId = malloc(sizeof(int));
    *threadId = 1;
    rc = pthread_create(
            &writerThread,  // thread identifier
            NULL,           // thread attributes
            (void*) writer, // thread function
            (void*) threadId);  // thread function argument

    if (rc != 0)
    {
    	fprintf(stderr,"Couldn't create the writer thread");
      exit (-1);
    }

    // Create the Reader threads
    for (i = 0; i < READERS_COUNT; i++) {
      // Reader initialization - takes random amount of time
      usleep(random_time(1000));
      int* threadId = malloc(sizeof(int));
      *threadId = i+1;
      rc = pthread_create(
                          &readerThreads[i], // thread identifier
                          NULL,              // thread attributes
                          (void*) reader,    // thread function
                          (void*) threadId);     // thread function argument

      if (rc != 0) {
        fprintf(stderr,"Couldn't create the reader threads");
        exit (-1);
      }
    }

    // At this point, the readers and writers should perform their operations

    // Wait for the Readers
    for (i = 0; i < READERS_COUNT; i++)
        pthread_join(readerThreads[i],NULL);

    // Wait for the Writer
    pthread_join(writerThread,NULL);

    return (0);
}
