/***********************************************************************************
 * PRODUCER-CUSTOMER PROCESSOR                                                     *
 *                                                                                 *
 * INTRODUCTION:                                                                   *
 * A limit buffer, producer put products into buffer, and customer get products    *
 * from buffer. When buffer was full, the producer must be wait, customer also     *
 * need to wait when buffer was empty.                                             *
 *
 ***********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>

#define MYFIFO            "myfifo"   /* the name of FIFO */
#define BUFFER_SIZE       3          /* the unit number of BUFFER */
#define UNIT_SIZE         5          /* the size of each unit */
#define RUN_TIME          30         /* runnint time */
#define DELAY_TIME_LEVELS 5.0

int     fd;
time_t  end_time;

/*-------------------------------------------------------------------------------- *
 * avail - represent the current avalible size of buffer, 
           initial value is BUFFER_SIZE. 
 */
sem_t   mutex, full, avail;

/**********************************************************************************
 * DESCRIPTIOIN:
 *   The thread of producer.
 *
 **********************************************************************************/
void *producer(void *arg)
{
  int real_write;
  int delay_time = 0;

  while (time(NULL) < end_time)
  {
    delay_time = (int)(rand() * DELAY_TIME_LEVELS/(RAND_MAX)) + 1;
    sleep(delay_time);
    /*-----------------------------------------------------------
     * P operate for avail and mutex.
     * 1. producer put data once, avail reduce 1 equal to BUFFER_SIZE -1.
     *---------------*/
    sem_wait(&avail);
    sem_wait(&mutex);
    printf("\nProducer: delay = %d\n", delay_time);
    /* producer write data into fifo */
    if ((real_write = write(fd, "hello", UNIT_SIZE)) == -1)
    {
      if (errno == EAGAIN)
      {
        printf("The FIFO has not been read yet. Please try later");
      }
    }
    else
    {
      printf("Write %d to the FIFO\n", real_write);
    }
    
    /*-----------------------------------------------------------
     * V operate for full and mutex.
     *---------------*/
    sem_post(&full);
    sem_post(&mutex);
  }
  pthread_exit(NULL);
}

/**********************************************************************************
 * DESCRIPTIOIN:
 *   The thread of customer.
 *
 **********************************************************************************/
void *customer(void *arg)
{
  unsigned char read_buffer[UNIT_SIZE];
  int real_read;
  int delay_time = 0;

  while (time(NULL) < end_time)
  {
    delay_time = (int)(rand() * DELAY_TIME_LEVELS/(RAND_MAX)) + 1;
    sleep(delay_time);
    /*-----------------------------------------------------------
     * P operate for avail and mutex.
     *---------------*/
    sem_wait(&full);
    sem_wait(&mutex);
    printf("\nCustomer: delay = %d\n", delay_time);
    /* producer write data into fifo */
    if ((real_read = read(fd, read_buffer, UNIT_SIZE)) == -1)
    {
      if (errno == EAGAIN)
      {
        printf("No data yet\n");
      }
    }
    printf("Read %d from the FIFO\n", real_read);
    
    /*-----------------------------------------------------------
     * V operate for full and mutex.
     *---------------*/
    sem_post(&avail);
    sem_post(&mutex);
  }
  pthread_exit(NULL);
}

int main()
{
  pthread_t thrd_prd_id, thrd_cst_id;
  pthread_t mon_th_id;
  int ret;

  srand(time(NULL));
  end_time = time(NULL) + RUN_TIME;

  umask(0);
  /* Create FIFO */
  if ((mkfifo(MYFIFO, 0666) < 0) && (errno != EEXIST))
  {
    printf("Cannot create fifo\n");
    return errno;
  }

  /* Open FIFO */
  fd = open(MYFIFO, O_RDWR);
  if (fd == -1)
  {
    printf("Open fifo error\n");
    return fd;
  }

  /*----------------------------------------------- 
   * Initialize semaphore:
   * 1) mutex - 1
   * 2) avail - N
   * 3) full  - 0
   *---------------------------------------------*/
  ret = sem_init(&mutex, 0, 1);
  ret += sem_init(&avail, 0, BUFFER_SIZE);
  ret += sem_init(&full, 0, 0);
  if (ret != 0)
  {
    printf("Any semaphore initialization failed\n");
    return ret;
  }
  
  /* Create two thread, producer and customer */
  ret = pthread_create(&thrd_prd_id, NULL, producer, NULL);
  if (ret != 0)
  {
    printf("Create producer thread error\n");
    return ret;
  }
  
  ret = pthread_create(&thrd_cst_id, NULL, customer, NULL);
  if (ret != 0)
  {
    printf("Create custmoer thread error\n");
    return ret;
  }

  pthread_join(thrd_prd_id, NULL);
  pthread_join(thrd_cst_id, NULL);
  close(fd);
  unlink(MYFIFO);

  return 0;
}

