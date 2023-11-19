#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int MAX_CHAIRS;
int CUT_TIME;
int NUM_BARB;
int MAX_CUST;
int CURR_BARB = 0;

// sem_t customers;
sem_t barbers;
sem_t mutex;

int numberOfFreeSeats;
int seatPocket[10000][3];
int sitHereNext = 0;
int serveMeNext = 0;
static int count = 0;

bool isHiring = false; // A flag to control hiring
bool isFiring = false; // A flag to control firing
bool insuff = false;

pthread_mutex_t currbarbMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int customerID;
  int satisfactionLevel; // You can use a scale from 1 to 5, for example
  time_t entryTime;
  time_t serviceTime;
  time_t waitingTime;
} CustomerFeedback;

enum CustomerType { REGULAR, VIP, CHILD, SENIOR };
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

void barberThread(void *tmp);
void customerThread(void *tmp);
void wait();
void hireBarber();
void fireBarber();
void managementThread(void *tmp);
int main() {
  printf("Enter the number of chairs in waiting room: ");
  scanf("%d", &MAX_CHAIRS);

  printf("Enter number of barbers: ");
  scanf("%d", &NUM_BARB);

  printf("Enter value of Max customers: ");
  scanf("%d", &MAX_CUST);

  numberOfFreeSeats = MAX_CHAIRS;

  pthread_t barber[NUM_BARB], customer[MAX_CUST], managerThread;

  int i, status = 0;

  //sem_init(&customers, 0, 0);
  sem_init(&barbers, 0, 0);
  sem_init(&mutex, 0, 1);

  printf("!!Barber Shop Opens!!\n\n");

  for (i = 0; i < NUM_BARB; i++) {
    int *serial_number = malloc(sizeof(int));
    *serial_number = i;
    status = pthread_create(&barber[i], NULL, (void *)barberThread, serial_number);
    if (status != 0) {
      perror("No Barber Present... Sorry!!\n\n");
    }
  }

  for (i = 0; i < MAX_CUST; i++) {
    status =
        pthread_create(&customer[i], NULL, (void *)customerThread, NULL);
    // wait();
    int waittimes[6] = {320000, 160000, 640000, 5000, 1200, 320};
    int x = rand() % 7;
    sleep(rand() % 5);
    // usleep(waittimes[x]);
    if (status != 0) {
      perror("No Customers Yet!!!\n\n");
    }
  }
  status = pthread_create(&managerThread, NULL, (void *)managementThread,
                          (void *)&barbers);
  if (status != 0) {
    perror("Error creating the management thread\n\n");
  }

  // Wait for the management thread to finish
  // pthread_join(managerThread, NULL);
  for (i = 0; i < MAX_CUST; i++) {
    pthread_join(customer[i], NULL);
  }

  printf("Pending hires and fires cancelled\n!!Barber Shop Closes!!\n\n");
  exit(EXIT_SUCCESS);
}

void customerThread(void *tmp) {
  int mySeat, B, id;
  int satisfaction;
  enum CustomerType type =
      (enum CustomerType)(rand() % 4); // Randomly assign customer types
  time_t entryTime;
  time_t serviceTime;
  time_t waitingTime;
  sem_wait(&mutex);
  count++;
  id = count;
  printf("Customer-%d Entered Shop. \n\n", id);
  time(&entryTime);
  printf("There are %d free seats \n",numberOfFreeSeats);
  if (numberOfFreeSeats > 0) {
    numberOfFreeSeats--;
    printf("Customer-%d Sits In Waiting Room.\n\n", id);
    sitHereNext = (sitHereNext + 1) % MAX_CHAIRS;
    mySeat = sitHereNext;
    seatPocket[mySeat][0] = id;
    sem_post(&mutex);
    sem_post(&barbers);
    //sem_wait(&customers);
    while(seatPocket[mySeat][0] != -1);
    time(&serviceTime);
    sem_wait(&mutex);
    //B = seatPocket[mySeat];
    numberOfFreeSeats++;
    
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
    seatPocket[mySeat][1]=waitingTime;
    seatPocket[mySeat][2]=satisfaction;
    sem_post(&mutex);
    
    
    

  } else {
    sem_post(&mutex);
    printf("Customer-%d Finds No Seat & Leaves.\n\n", id);
    insuff = true;
    satisfaction = -1;
    // sleep(2); // Sleep for 2 seconds as a placeholder
  }

  customerFeedbackData[count].customerID = count;
  customerFeedbackData[count].satisfactionLevel = satisfaction;
  customerFeedbackData[count].entryTime = entryTime;
  customerFeedbackData[count].serviceTime = serviceTime;
  customerFeedbackData[count].waitingTime = waitingTime;
  pthread_exit(0);
}

void barberThread(void *tmp) {
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
  printf("Barber-%d Joins Shop. Type: %s, Skill: %d\n\n", index, BarberTypeStrings[typenum], skill);
  while (1) {
    printf("Barber-%d Gone To Sleep.\n\n", index);
    sem_wait(&barbers);
    sem_wait(&mutex);
    serveMeNext = (serveMeNext + 1) % MAX_CHAIRS;
    myNext = serveMeNext;
    C = seatPocket[myNext][0];
    //seatPocket[myNext] = pthread_self();
    
    printf("Barber-%d Wakes Up & Is Cutting Hair Of Customer-%d.\n\n", index,
            C);
    sleep(5);
    printf("Barber-%d Finishes. ", index);
    printf(
    "Customer-%d is satisfied with level %d. Waited for %d seconds.\n\n",
    C, seatPocket[myNext][1],seatPocket[myNext][2]);
    //sem_post(&customers);
    seatPocket[myNext][0] = -1;
    seatPocket[myNext][1] = -1;
    seatPocket[myNext][2] = -1;
    sem_post(&mutex);
  }
}

void managementThread(void *tmp) {

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
      // barber
      // fireBarber();
    }

    // Sleep for a while before making the next evaluation
    sleep(2); // Adjust the sleep duration as needed
  }
}
void wait() {
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

    printf("Fired a barber. Total barbers: %d\n\n", CURR_BARB);
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

void hireBarber() {
  // Hire a new barber
  printf("Hiring a new barber!\n\n");
  pthread_mutex_lock(&currbarbMutex);
  // Create a new thread for the hired barber
  pthread_t newBarber;
  int *serial_number = (int*)malloc(sizeof(int));
  *serial_number = CURR_BARB;
  int status = pthread_create(&newBarber, NULL, (void *)barberThread,
                              serial_number);

  printf("Hired a new barber. Total barbers: %d\n\n", CURR_BARB);

  if (status != 0) {
      perror("Failed to hire a new barber!\n\n");
  }
  pthread_mutex_unlock(&currbarbMutex);
}