#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int MAX_CHAIRS;
int CUT_TIME;
int NUM_BARB;
int MAX_CUST;
int CURR_BARB = 0;
int numlog = 1;

// sem_t customers;
sem_t barbers;
sem_t mutex;

int numberOfFreeSeats;
int seatPocket[10000][3];
// Task: Implement as linked list for better memory efficiency
int sitHereNext = 0;
int serveMeNext = 0;
static int count = 0;

bool isHiring = false; // A flag to control hiring
bool isFiring = false; // A flag to control firing
bool insuff = false;
bool isnew = true;
bool cusisnew = true;
bool logging = false;

pthread_mutex_t currbarbMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int customerID;
  int type;
  int satisfactionLevel; // You can use a scale from 1 to 5, for example
  time_t entryTime;
  time_t serviceTime;
  time_t waitingTime;
} CustomerFeedback;

enum BarberType { REGULARB, SPEEDY, CHATTY, EXPERIENCED };

typedef struct {
  int barberID;
  enum BarberType type;
  int skill;
} Barber;

Barber barberData[10000] = {0, 0, 0};

CustomerFeedback customerFeedbackData[10000];

const char *BarberTypeStrings[] = {"REGULAR", "SPEEDY", "CHATTY",
                                   "EXPERIENCED"};
const char *CustomerType[] = {"REGULAR", "VIP", "CHILD", "SENIOR"};
void barberThread(void *tmp);
void customerThread(void *tmp);
void wait();
void hireBarber();
void fireBarber();
void managementThread(void *tmp);
void loggingthread();

void writelog() {

  // creating a FILE variable
  FILE *fptr;

  // open the file in append mode

  if (isnew) {
    fptr = fopen("Logs.txt", "w");
    fprintf(fptr,
            "\t\t\t*****************QUEUE LOG***************\t\t\t\n\n\n");
    isnew = false;
  } else
    fptr = fopen("Logs.txt", "a");
  if (fptr != NULL) {
    printf("[Logged]\n");
  } else if (fptr == NULL) {
    printf("Failed to create the file.\n");
    // exit status for OS that an error occurred
  }

  time_t t = time(NULL);
  struct tm *tmp = gmtime(&t);

  int h = (t / 360) % 24; /* ### My problem. */
  int m = (t / 60) % 60;
  int s = t % 60;
  int i = 1;

  fprintf(fptr, "--[Time: %02d:%02d:%02d]-----[Log: %d]--------------------\n",
          h, m, s, numlog++);
  fprintf(fptr, "Number of chairs in waiting queue:\t %d\n", MAX_CHAIRS);
  fprintf(fptr, "Number of free seats:\t %d\n", numberOfFreeSeats);
  fprintf(fptr, "The waiting queue:\n");
  fprintf(fptr, "Customer ID: \t");
  for (; i < count; i++) {
    if (seatPocket[i][0] == -1)
      fprintf(fptr, "| E ");

    else
      fprintf(fptr, "| %d ", seatPocket[i][0]);
  }
  for (int i = 0; i < MAX_CHAIRS - count + 1; i++) {
    fprintf(fptr, "| E ");
  }
  fprintf(fptr, "|\n");

  fprintf(fptr, "\n\n");
  // close connection
  fclose(fptr);
}
void custlog(int i) {

  // creating a FILE variable
  FILE *fptr;

  if (cusisnew) {
    fptr = fopen("CustLogs.txt", "w");
    fprintf(fptr,
            "\t\t\t*****************CUSTOMER LOG***************\t\t\t\n\n\n");
    cusisnew = false;
  }
  // open the file in append mode
  else
    fptr = fopen("CustLogs.txt", "a");
  if (fptr != NULL) {
    printf("[Customer data recorded]\n");
  } else if (fptr == NULL) {
    printf("Failed to create the file.\n");
    // exit status for OS that an error occurred
  }
  int id = customerFeedbackData[i].customerID;
  int type = customerFeedbackData[i].type;
  int satisfactionLevel = customerFeedbackData[i].satisfactionLevel;

  time_t t = customerFeedbackData[i].waitingTime;
  struct tm *tmp = gmtime(&t);

  // int m = (t / 60) % 60;
  // int s = t % 60;
  int m = tmp->tm_min;
  int s = tmp->tm_sec;
  // printf("waitingTime: %lld, m: %d, s: %d\n", (long
  // long)customerFeedbackData[i].waitingTime, m, s);

  fprintf(fptr, "---[Customer: %d]--------------------\n", id);

  fprintf(fptr, "Customer ID:\t %d\n", id);
  fprintf(fptr, "Customer type:\t %s\n", CustomerType[type]);
  fprintf(fptr, "Satisfaction Level:\t %d\n",
          customerFeedbackData[i].satisfactionLevel);
  fprintf(fptr, "Time waited %d:%d", tmp->tm_min, tmp->tm_sec);

  fprintf(fptr, "\n\n");

  // close connection
  fclose(fptr);
}

int main() {
  printf("Enter the number of chairs in waiting room: ");
  scanf("%d", &MAX_CHAIRS);

  printf("Enter number of barbers: ");
  scanf("%d", &NUM_BARB);

  printf("Enter value of Max customers: ");
  scanf("%d", &MAX_CUST);

  numberOfFreeSeats = MAX_CHAIRS;

  pthread_t barber[NUM_BARB], customer[MAX_CUST], managerThread, logger;

  int i, status = 0;

  sem_init(&barbers, 0, 0);
  sem_init(&mutex, 0, 1);

  printf("\n\n");

  printf("!!Barber Shop Opens!!\n\n");

  for (i = 0; i < NUM_BARB; i++) {
    int *serial_number = malloc(sizeof(int));
    *serial_number = i;
    status =
        pthread_create(&barber[i], NULL, (void *)barberThread, serial_number);
    if (status != 0) {
      perror("No Barber Present... Sorry!!\n\n");
    }
  }
  pthread_create(&logger, NULL, (void *)loggingthread, NULL);
  for (i = 0; i < MAX_CUST; i++) {
    status = pthread_create(&customer[i], NULL, (void *)customerThread, NULL);

    int waittimes[6] = {320000, 160000, 640000, 5000, 1200, 320};
    int x = rand() % 7;
    sleep(rand() % 5);

    if (status != 0) {
      perror("No Customers Yet!!!\n\n");
    }
  }

  status = pthread_create(&managerThread, NULL, (void *)managementThread,
                          (void *)&barbers);
  if (status != 0) {
    perror("Error creating the management thread\n\n");
  }
  pthread_join(logger, NULL);
  // Wait for the management thread to finish
  // pthread_join(managerThread, NULL);
  for (i = 0; i < MAX_CUST; i++) {
    pthread_join(customer[i], NULL);
  }

  printf("Pending hires and fires cancelled\n!!Barber Shop Closes!!\n\n");
  exit(EXIT_SUCCESS);
}

void customerThread(void *tmp) {
  while (logging)
    ;
  int mySeat, B, id;
  int satisfaction;
  int type = (rand() % 4); // Randomly assign customer types
  time_t entryTime;
  time_t serviceTime;
  time_t waitingTime;
  sem_wait(&mutex);
  count++;
  id = count;
  time(&entryTime);
  sem_post(&mutex);
  printf("Customer-%d Entered Shop. There are %d free seats.\n\n", id,
         numberOfFreeSeats);
  if (numberOfFreeSeats > 0) {
    sem_wait(&mutex);
    numberOfFreeSeats--;
    ++sitHereNext;
    mySeat = sitHereNext;
    seatPocket[mySeat][0] = id;
    printf("Customer-%d Sits In Waiting Room.\n\n", id);
    sem_post(&barbers);
    // sem_wait(&customers);
    // while (seatPocket[mySeat][0] != -1);
    time(&serviceTime);

    // Record customer service time

    // Calculate waiting time
    waitingTime = difftime(serviceTime, entryTime);
    satisfaction = 5; // Start with maximum satisfaction
    if (waitingTime > 6) {
      satisfaction = 1; // Lowest satisfaction for very long wait times
    } else if (waitingTime > 3) {
      satisfaction = 2;
    } else if (waitingTime > 1) {
      satisfaction = 3;
    } else if (waitingTime > 0) {
      satisfaction = 4;
    }

    seatPocket[mySeat][1] = waitingTime;
    seatPocket[mySeat][2] = satisfaction;

    printf("Customer-%d is satisfied with level %d. Waited for %d seconds.\n\n",
           id, seatPocket[mySeat][2], seatPocket[mySeat][1]);
    customerFeedbackData[count].customerID = count;
    customerFeedbackData[count].type = type;
    customerFeedbackData[count].satisfactionLevel = satisfaction;
    customerFeedbackData[count].entryTime = entryTime;
    customerFeedbackData[count].serviceTime = serviceTime;
    customerFeedbackData[count].waitingTime = waitingTime;
    custlog(count);
    sem_post(&mutex);
  } else {
    printf("Customer-%d Finds No Seat & Leaves.\n\n", id);
    insuff = true;
    satisfaction = -1;
    sem_wait(&mutex);
    customerFeedbackData[count].customerID = count;
    customerFeedbackData[count].type = type;
    customerFeedbackData[count].satisfactionLevel = satisfaction;
    customerFeedbackData[count].entryTime = entryTime;
    customerFeedbackData[count].serviceTime = 0;
    customerFeedbackData[count].waitingTime = 0;
    custlog(count);
  }

  pthread_exit(0);
}

void barberThread(void *tmp) {
  while (logging)
    ;
  pthread_mutex_lock(&currbarbMutex);
  CURR_BARB++;
  int index = *(int *)(tmp);
  free(tmp);
  int myNext, C;
  int typenum = (rand() % 4);
  int skill = rand() % 10 + 1;
  int id = pthread_self();
  enum BarberType type = typenum;

  if (typenum == 1) {
    skill *= 1.5;
  }
  barberData[CURR_BARB].barberID = id;
  barberData[CURR_BARB].skill = skill;
  barberData[CURR_BARB].type = type;
  pthread_mutex_unlock(&currbarbMutex);
  printf("Barber-%d Joins Shop. Type: %s, Skill: %d\n\n", index,
         BarberTypeStrings[typenum], skill);
  while (1) {
    printf("Barber-%d Gone To Sleep.\n\n", index);
    sem_wait(&barbers);
    sem_wait(&mutex);

    ++serveMeNext;
    myNext = serveMeNext;
    numberOfFreeSeats++;
    sem_post(&mutex);
    C = seatPocket[myNext][0];

    printf("Customer-%d gets up from their seat\nThere are %d free seats\n\n",
           C, numberOfFreeSeats);

    printf("Barber-%d Wakes Up & Is Cutting Hair Of Customer-%d.\n\n", index,
           C);

    sleep(30 / skill);
    printf("Barber-%d Finishes.\n", index);

    sem_wait(&mutex);
    seatPocket[myNext][0] = -1;

    sem_post(&mutex);
  }
  pthread_exit(0);
}

void managementThread(void *tmp) {
  while (logging)
    ;
  while (1) {
    pthread_t lastbarb = *(pthread_t *)(tmp);

    // Check the need for hiring or firing barbers based on certain criteria
    // For example, you can evaluate the number of customers and free seats
    if (insuff || (numberOfFreeSeats < 10 && CURR_BARB < 5 &&
                   !(MAX_CHAIRS == numberOfFreeSeats))) {

      hireBarber();
      // Hire a new barber if there are more than 10 free seats and fewer than 5
      // barbers
      insuff = false;

    } else if (numberOfFreeSeats > (0.5) * MAX_CHAIRS && CURR_BARB > 1) {
      // Fire a barber if there are fewer than 5 free seats and more than 1
      // barber fireBarber();
    }

    // Sleep for a while before making the next evaluation
    sleep(2); // Adjust the sleep duration as needed
  }
}
void wait() {
  while (logging)
    ;
  int waittimes[6] = {320000, 160000, 640000, 5000, 1200, 320};
  int x = rand() % 7;
  usleep(waittimes[x]);
}

// Define a mutex for controlling access to the activeBarbers variable

void fireBarber(pthread_t barberThread) {
  // Lock the mutex to ensure exclusive access to the activeBarbers variable
  pthread_mutex_lock(&currbarbMutex);

  // Check if there are active barbers to fire
  if (CURR_BARB > 0) {
    /*
    barberData[CURR_BARB--].barberID =0;
    barberData[CURR_BARB].skill=0;
    barberData[CURR_BARB].type=0;
*/
    // Decrease the count of active barbers

    printf("Fired a barber. Total barbers: %d\n\n", CURR_BARB--);
    // Unlock the mutex before terminating the thread

    // Terminate the barber thread
    if (pthread_cancel(barberThread) != 0) {
      perror("Failed to cancel the barber thread");
    } else {
      printf("Firing a barber!\n\n");
    }
    pthread_mutex_unlock(&currbarbMutex);
  } else {
    // No active barbers to fire
    pthread_mutex_unlock(&currbarbMutex);
    printf("No active barbers to fire!\n\n");
  }
}

// Task: Separate counter variable for barber indices that will not create
// overlaps when firing barbers
void hireBarber() {
  // Hire a new barber
  printf("Hiring a new barber!\n\n");
  pthread_mutex_lock(&currbarbMutex);
  // Create a new thread for the hired barber
  pthread_t newBarber;
  int *serial_number = (int *)malloc(sizeof(int));
  *serial_number = CURR_BARB;
  // CURR_BARB is incremented in the barber thread itself;
  int status =
      pthread_create(&newBarber, NULL, (void *)barberThread, serial_number);

  if (status != 0) {
    perror("Failed to hire a new barber!\n\n");
  } else {
    printf("Hired a new barber. Total barbers: %d\n\n", CURR_BARB + 1);
  }

  pthread_mutex_unlock(&currbarbMutex);
}

void loggingthread() {
  int okay = count;
  while (1) {
    sem_wait(&mutex);
    int okay = count;
    sem_post(&mutex);
    logging = true;
    writelog();
    logging = false;
    sleep(2);
    if (count == MAX_CUST) {
      break;
    }
  }
  pthread_exit(0);
}
