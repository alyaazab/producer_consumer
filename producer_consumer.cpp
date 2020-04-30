#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <queue>

void *increment_count(void *);
void *write_to_buffer(void *);
void *read_from_buffer(void *);

sem_t counter_mutex, buff_mutex, buff_num_of_items, buff_space_left;

int counter = 0;
int buffer_size = 2;
bool is_monitor_done = false, is_collector_done = false;
std::queue<int> buffer;

int main()
{
  int num_of_counter_threads = 9;

  pthread_t monitor, collector;

  //initialise a semaphore on the integer counter to enforce mutual exclusion
  sem_init(&counter_mutex, 0, 1);

  //initialise a semaphore on the buffer to enforce mutual exclusion
  sem_init(&buff_mutex, 0, 1);

  //initialise a semaphore on the buffer to prevent producer from adding to full buffer
  sem_init(&buff_space_left, 0, buffer_size);

  //initialise a semaphore on the buffer to prevent consumer from taking from empty buffer
  sem_init(&buff_num_of_items, 0, 0);

  //create a monitor thread
  if (pthread_create(&monitor, NULL, write_to_buffer, NULL))
  {
    printf("Unable to create a thread");
    exit(-1);
  }

  //create a collector thread
  if (pthread_create(&collector, NULL, read_from_buffer, NULL))
  {
    printf("Unable to create a thread");
    exit(-1);
  }

  //create n counter threads
  pthread_t counter_threads[num_of_counter_threads];

  for (int i = 0; i < num_of_counter_threads; i++)
  {
    if (pthread_create(&counter_threads[i], NULL, increment_count, (void *)i))
    {
      printf("Unable to create a thread");
      exit(-1);
    }
  }

  // for (int i = 0; i < num_of_counter_threads; i++)
  //   pthread_join(counter_threads[i], NULL);

  // is_monitor_done = true;
  // pthread_join(monitor, NULL);

  // is_collector_done = true;
  // pthread_join(collector, NULL);

  //destroy all semaphores
  sem_destroy(&counter_mutex);
  sem_destroy(&buff_mutex);
  sem_destroy(&buff_space_left);
  sem_destroy(&buff_num_of_items);

  pthread_exit(NULL);

  return 0;
}

void *increment_count(void *tid)
{
  int thread_id = (int)tid;
  int val;

  while (true)
  {
    val = rand() % 3 + 1;
    // printf("%d", val);

    sleep(val);
    printf("\n\nCounter thread %d: received a message", thread_id);
    printf("\n\nCounter thread %d: waiting to write...", thread_id);

    //wait on mutex counter to enter critical section
    sem_wait(&counter_mutex);

    //increment counter
    counter++;
    printf("\n\nCounter thread %d: now adding to counter, counter value = %d", thread_id, counter);

    //signal that we're exiting critical section
    sem_post(&counter_mutex);
  }
  pthread_exit(NULL);
}

void *write_to_buffer(void *)
{
  int val;
  int value;

  while (true)
  {
    val = rand() % 10 + 1;
    sleep(val);

    printf("\n\nMonitor thread: waiting to read counter...");

    //wait on mutex_counter to enter critical section
    //read counter value and reset it
    //signal that we're exiting critical section
    sem_wait(&counter_mutex);
    printf("\n\nMonitor thread: counter value = %d", counter);
    value = counter;
    counter = 0;
    sem_post(&counter_mutex);

    printf("\n\nMonitor thread: waiting to write to buffer...");

    //wait on semaphore that indicates space left in buffer
    //if no space left (full buffer), block

    sem_wait(&buff_mutex);
    if (buffer.size() == buffer_size)
      printf("\nMonitor Thread: Buffer is full!");
    sem_post(&buff_mutex);

    sem_wait(&buff_space_left);

    //wait on mutex_buffer to enter critical section
    sem_wait(&buff_mutex);

    //write counter value to buffer
    printf("\n\nMonitor thread: writing to buffer at position %d", buffer.size());
    buffer.push(value);

    //signal that we're exiting critical section
    sem_post(&buff_mutex);
    //signal to increase semaphore nsem which indicates the number of items in the buffer
    sem_post(&buff_num_of_items);
  }

  pthread_exit(NULL);
}

void *read_from_buffer(void *)
{
  int val;
  int value;

  while (true)
  {
    val = rand() % 2 + 1;
    sleep(val);

    sem_wait(&buff_mutex);
    if (buffer.size() == 0)
      printf("\nCollector thread: Buffer is empty!");
    sem_post(&buff_mutex);

    //wait for there to be items in the buffer
    sem_wait(&buff_num_of_items);

    printf("\n\nCollector thread: waiting to read from the buffer...");

    //wait on mutex_buffer to enter critical section
    sem_wait(&buff_mutex);

    //print value being read from buffer and pop it from the queue
    value = buffer.front();
    printf("\n\nCollector thread: reading from buffer, value = %d", value);
    buffer.pop();

    //signal that we're leaving the critical section
    sem_post(&buff_mutex);
    //signal that there is now a new empty space left in the buffer
    sem_post(&buff_space_left);
  }

  pthread_exit(NULL);
}
