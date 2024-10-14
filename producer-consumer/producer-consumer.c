#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void* producer(void *arg);
void* consumer(void *arg);

int buffer_size;
int *buffer;
char *command_buffer;
sem_t mutex, empty, full;
int producer_index, consumer_index;

int main(int argc, char *argv[]) {
  pthread_t producer_thread, consumer_thread;

  if(argc < 4) {
    fprintf(stderr, "Err: must specify buffer size, producer sleep time, and consumer sleep time\n");
    exit(1);
  }
  buffer_size = atoi(argv[1]); // desired size of the buffer
  int producer_sleep_time = atoi(argv[2]); // desired sleep time for the producer
  int consumer_sleep_time = atoi(argv[3]); // desired sleep time for the consumer

  // initialize the buffer
  buffer = malloc(sizeof(int) * buffer_size);
  command_buffer = malloc(sizeof(char) * 2);

  // initialize necessary semaphores
  sem_init(&mutex, 0, 1);
  sem_init(&empty, 0, buffer_size);
  sem_init(&full, 0, 0);

  // initialize the procucer and consumer index
  producer_index = 0;
  consumer_index = 0;
  
  pthread_create(&producer_thread, NULL, producer, &producer_sleep_time); // create the producer thread
  pthread_create(&consumer_thread, NULL, consumer, &consumer_sleep_time);  // create the consumer thread

  char user_input[2];
  // core loop to recieve user commands
  while(true) {
    // printf("Enter desired command: ");
    fgets(user_input, 3, stdin);
    if(strcmp(user_input, "q\n") == 0) {
      printf("Preparing to quit\n");
    }

    sem_wait(&mutex); // wait for the mutex semaphore to unlock
    
    strcpy(command_buffer, user_input);

    sem_post(&mutex);

    if(strcmp(user_input, "q\n") == 0) {
      break;
    }
  }

  int producer_val, consumer_val;
  pthread_join(producer_thread, (void **) &producer_val); // join the producer thread
  pthread_join(consumer_thread, (void **) &consumer_val); // join the consumer thread

  free(buffer); // free the buffer
  free(command_buffer); // free the command buffer

  return 0;
}

void* producer(void *arg) {
  // need upper bound to be 8001 so that rand() % upper_bound generates from 0-8000, then add 1000 so final range is 1000-9000
  int upper_bound = 8001; 
  int sleep_time = *(int *)arg;
  int next_produced;

  srand(time(NULL)); // seed the random number generator with the current time

  while (true) {
    // produce an item in next_produced
    next_produced = (rand() % (upper_bound)) + 1000; // generate a random number between 1000 and 9000

    sem_wait(&empty); // wait for the empty semaphore to unlock
    sem_wait(&mutex); // wait for the mutex semaphore to unlock

    if(strcmp(command_buffer, "a\n") == 0) {
      sleep_time += 250;
      strcpy(command_buffer, " \n"); // clear the buffer
    }
    else if(strcmp(command_buffer, "z\n") == 0) {
      sleep_time -= 250;
      strcpy(command_buffer, " \n"); // clear the buffer
    }

    usleep(sleep_time); // sleep for the desired time

    // add next_produced to the buffer
    buffer[producer_index] = next_produced;

    sem_post(&mutex);
    sem_post(&full);

    printf("Put %d into bin %d\n", next_produced, producer_index);
    producer_index = (producer_index + 1) % buffer_size; // make indexing wrap around

    if(strcmp(command_buffer, "q\n") == 0) {
      break; // break if q is in the command buffer
    }
  }
  printf("End of producer\n");
  return 0;
}

void* consumer(void *arg) {
  int next_consumed;
  int sleep_time = *(int *)arg;
  while (true) {
    sem_wait(&full); // wait for the full semaphore to unlock
    sem_wait(&mutex); // wait for the mutex semaphore to unlock

    if(strcmp(command_buffer, "s\n") == 0) {
      sleep_time += 250;
      strcpy(command_buffer, " \n"); // clear the buffer
    }
    else if(strcmp(command_buffer, "x\n") == 0) {
      sleep_time -= 250;
      strcpy(command_buffer, " \n"); // clear the buffer
    }

    usleep(sleep_time); // sleep for the desired time

    // remove an item from buffer to next_consumed (TODO)
    next_consumed = buffer[consumer_index];

    sem_post(&mutex);
    sem_post(&empty);

    // consume the item in next_consumed (TODO)
    printf("\tGet %d from bin %d\n", next_consumed, consumer_index);
    consumer_index = (consumer_index + 1) % buffer_size; // make indexing wrap around

    if(strcmp(command_buffer, "q\n") == 0) {
      break; // break if q is in the command_buffer
    }
  }
  printf("\tEnd of consumer\n");
  return 0;
}