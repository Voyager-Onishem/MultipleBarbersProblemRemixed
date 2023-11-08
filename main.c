#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

int MAX_CHAIRS;
int CUT_TIME;
int NUM_BARB;
int MAX_CUST;
int CURR_BARB;

sem_t customers;
sem_t barbers;
sem_t mutex;

int numberOfFreeSeats;
int seatPocket[10000];
int sitHereNext = 0;
int serveMeNext = 0;
static int count = 0;

bool isHiring = false;  // A flag to control hiring
bool isFiring = false;  // A flag to control firing
bool insuff = false; 

pthread_mutex_t currbarbMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int customerID;
    int satisfactionLevel; // You can use a scale from 1 to 5, for example
    time_t entryTime;
    time_t serviceTime;
    time_t waitingTime;
} CustomerFeedback;

CustomerFeedback customerFeedbackData[10000];

CustomerFeedback customerFeedbackData[10000];

void barberThread(void *tmp);
void customerThread(void *tmp);
void wait();
void hireBarber();
void fireBarber();

int main() {
    printf("Enter the number of chairs in waiting room: ");
    scanf("%d", &MAX_CHAIRS);

    printf("Enter value of cutting time: ");
    scanf("%d", &CUT_TIME);

    printf("Enter number of barbers: ");
    scanf("%d", &NUM_BARB);

    printf("Enter value of Max customers: ");
    scanf("%d", &MAX_CUST);

    numberOfFreeSeats = MAX_CHAIRS;

    pthread_t barber[NUM_BARB], customer[MAX_CUST];
    int i, status = 0;

    sem_init(&customers, 0, 0);
    sem_init(&barbers, 0, 0);
    sem_init(&mutex, 0, 1);

    printf("!!Barber Shop Opens!!\n");

    for (i = 0; i < NUM_BARB; i++) {
        status = pthread_create(&barber[i], NULL, (void *)barberThread, (void *)&i);
        sleep(1);
        if (status != 0) {
            perror("No Barber Present... Sorry!!\n");
        }
    }

    for (i = 0; i < MAX_CUST; i++) {
        status = pthread_create(&customer[i], NULL, (void *)customerThread, (void *)&i);
        wait();
        if (status != 0) {
            perror("No Customers Yet!!!\n");
        }
    }

    for (i = 0; i < MAX_CUST; i++) {
        pthread_join(customer[i], NULL);
    }

    printf("!!Barber Shop Closes!!\n");
    exit(EXIT_SUCCESS);
}

void customerThread(void *tmp) {
    int mySeat, B;
    sem_wait(&mutex);
    count++;
    printf("Customer-%d[Id:%lu] Entered Shop. ", count, pthread_self());
    time_t entryTime;
    time(&entryTime);
    if (numberOfFreeSeats > 0) {
        numberOfFreeSeats--;
        printf("Customer-%d Sits In Waiting Room.\n", count);
        sitHereNext = (sitHereNext + 1) % MAX_CHAIRS;
        mySeat = sitHereNext;
        seatPocket[mySeat] = count;
        sem_post(&mutex);
        sem_post(&barbers);
        sem_wait(&customers);
        sem_wait(&mutex);
        B = seatPocket[mySeat];
        numberOfFreeSeats++;
        sem_post(&mutex);
        // Record customer service time
        time_t serviceTime;
        time(&serviceTime);

        // Calculate waiting time
        time_t waitingTime = difftime(serviceTime, entryTime);
        int satisfactionLevel = 5; // Start with maximum satisfaction
        if (waitingTime > 6) {
            satisfactionLevel = 1; // Lowest satisfaction for very long wait times
        } else if (waitingTime > 3) {
            satisfactionLevel = 2;
        } else if (waitingTime > 1) {
            satisfactionLevel = 3;
        } else if (waitingTime > 0) {
            satisfactionLevel = 4;
        }

        printf("Customer-%d is satisfied with level %d. Waited for %ld seconds.\n", count, satisfactionLevel, waitingTime);
        customerFeedbackData[count].customerID = count;
        customerFeedbackData[count].satisfactionLevel = satisfactionLevel;
        customerFeedbackData[count].entryTime = entryTime;
        customerFeedbackData[count].serviceTime = serviceTime;
        customerFeedbackData[count].waitingTime = waitingTime;
    } else {
        sem_post(&mutex);
        printf("Customer-%d Finds No Seat & Leaves.\n", count);
        insuff=true;
        sleep(2); // Sleep for 2 seconds as a placeholder
    }
    int arrivalTime = rand() % 5; // Adjust the range as needed
    sleep(arrivalTime);
    pthread_exit(0);
}

void barberThread(void *tmp) {
    int index = *(int *)(tmp);
    int myNext, C;
    int skill = rand() % 10 + 1;   
    printf("Barber-%d[Id:%lu] Joins Shop. ", index, pthread_self());
    while (1) {
        printf("Barber-%d Gone To Sleep.\n", index);
        sem_wait(&barbers);
        sem_wait(&mutex);
        serveMeNext = (serveMeNext + 1) % MAX_CHAIRS;
        myNext = serveMeNext;
        C = seatPocket[myNext];
        seatPocket[myNext] = pthread_self();
        sem_post(&mutex);
        sem_post(&customers);
        printf("Barber-%d Wakes Up & Is Cutting Hair Of Customer-%d.\n", index, C);
        sleep(CUT_TIME/skill);
        printf("Barber-%d Finishes. ", index);
    }
}

void wait() {
    int x = rand() % (5000000 - 5000 + 1) + 50000;
    srand(time(NULL));
    usleep(x);
}

// Define a mutex for controlling access to the activeBarbers variable


void fireBarber(pthread_t barberThread) {
    // Lock the mutex to ensure exclusive access to the activeBarbers variable
    pthread_mutex_lock(&currbarbMutex);

    // Check if there are active barbers to fire
    if (CURR_BARB > 0) {
        // Decrease the count of active barbers
        CURR_BARB--;

        // Unlock the mutex before terminating the thread
        pthread_mutex_unlock(&currbarbMutex);

        // Terminate the barber thread
        if (pthread_cancel(barberThread) != 0) {
            perror("Failed to cancel the barber thread");
        } else {
            printf("Firing a barber!\n");
        }
    } else {
        // No active barbers to fire
        pthread_mutex_unlock(&currbarbMutex);
        printf("No active barbers to fire!\n");
    }
}

void hireBarber() {
    // Hire a new barber
    printf("Hiring a new barber!\n");
    if(CURR_BARB<NUM_BARB){
    // Create a new thread for the hired barber
    pthread_t newBarber;
    int status = pthread_create(&newBarber, NULL, (void *)barberThread, (void *)&CURR_BARB);
      CURR_BARB++;


      if (status != 0) {
          perror("Failed to hire a new barber!\n");

      }
    }
}