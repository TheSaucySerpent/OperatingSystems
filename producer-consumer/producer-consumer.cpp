#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <chrono>

void* producer(void *arg);
void* consumer(void *arg);

int buffer_size;
int buffer_count;
int *buffer;
bool producer_done;
std::string command_buffer;
std::mutex mtx;
std::condition_variable empty, full;
int producer_index, consumer_index;

int main(int argc, char *argv[]) {
  if(argc < 4) {
    std::cerr << "Err: must specify buffer size, producer sleep time, and consumer sleep time" << std::endl;
    exit(1);
  }
  buffer_size = atoi(argv[1]); // desired size of the buffer
  int producer_sleep_time = atoi(argv[2]); // desired sleep time for the producer
  int consumer_sleep_time = atoi(argv[3]); // desired sleep time for the consumer

  // initialize the buffer and buffer size
  buffer = new int[buffer_size];
  buffer_count = 0;

  // initialize the procucer and consumer index
  producer_index = 0;
  consumer_index = 0;

  // initialize producer done boolean
  producer_done = false;
  
  std::thread producer_thread(producer, &producer_sleep_time); // create the producer thread
  std::thread consumer_thread(consumer, &consumer_sleep_time); // create the consumer thread

  std::string user_input;
  // core loop to recieve user commands
  while(true) {
    // printf("Enter desired command: ");
    std::cin >> user_input;

    if(user_input == "q") {
      std::cout << "Preparing to quit" << std::endl;
    }

    mtx.lock(); // lock the mutex

    command_buffer = user_input;

    mtx.unlock(); // unlock the mutex

    if(user_input == "q") {
      break; // can check user_input after of sem_post since not shared (as opposed to command_buffer)
    }
  }

  producer_thread.join(); // join the producer thread
  consumer_thread.join(); // join the consumer thread

  delete[] buffer; // free the buffer

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

    std::unique_lock<std::mutex> lock(mtx); // lock the mutex
    full.wait(lock, [] {return buffer_count < buffer_size;});

    if(command_buffer == "a") {
      sleep_time += 250;
      command_buffer.clear(); // clear the buffer
    }
    else if(command_buffer == "z") {
      sleep_time -= 250;
      command_buffer.clear(); // clear the buffer
    }

    // convert microseconds to milliseconds for sleep_for
    std::chrono::microseconds sleep_duraction(sleep_time);
    std::this_thread::sleep_for(sleep_duraction); // sleep for the desired time

    // add next_produced to the buffer
    buffer[producer_index] = next_produced;
    buffer_count++;

    std::cout << "Put " << next_produced << " into bin " << producer_index << std::endl;
    producer_index = (producer_index + 1) % buffer_size; // make indexing wrap around

    if(command_buffer == "q") {
      producer_done = true;
      empty.notify_one();
      break; // break if q is in the command buffer
    }
    empty.notify_one();
  }
  std::cout << "End of producer" << std::endl;
  return 0;
}

void* consumer(void *arg) {
  int next_consumed;
  int sleep_time = *(int *)arg;
  while (true) {
    std::unique_lock<std::mutex> lock(mtx); // lock the mutex
    empty.wait(lock, [] {return buffer_count > 0;});

    if(command_buffer == "s") {
      sleep_time += 250;
      command_buffer.clear(); // clear the buffer
    }
    else if(command_buffer == "x") {
      sleep_time -= 250;
      command_buffer.clear(); // clear the buffer
    }

    // convert microseconds to milliseconds for sleep_for
    std::chrono::microseconds sleep_duraction(sleep_time);
    std::this_thread::sleep_for(sleep_duraction); // sleep for the desired time

    // remove an item from buffer to next_consumed
    next_consumed = buffer[consumer_index];
    buffer_count--;

    // consume the item in next_consumed
    std::cout << "\tGet " << next_consumed << " from bin " << consumer_index << std::endl;
    consumer_index = (consumer_index + 1) % buffer_size; // make indexing wrap around

    if(command_buffer == "q" && consumer_index == producer_index  && producer_done == true) {
      full.notify_one();
      break; // break if q is in the command_buffer
    }
    full.notify_one();
  }
  std::cout << "\tEnd of consumer" << std::endl;
  return 0;
}