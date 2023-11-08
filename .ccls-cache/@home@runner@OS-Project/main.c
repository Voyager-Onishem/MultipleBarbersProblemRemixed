#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int MAX_CHAIRS;
int CUT_TIME;
int NUM_BARB;
int MAX_CUST;

sem_t customers;
sem_t barbers;
sem_t mutex;

int numberOfFreeSeats;
int seatPocket[10000];
int sitHereNext = 0;
int serveMeNext = 0;
static int count = 0;

void barberThread(void *tmp);
void customerThread(void *tmp);
void wait();

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
    } else {
        sem_post(&mutex);
        printf("Customer-%d Finds No Seat & Leaves.\n", count);
        sleep(2); // Sleep for 2 seconds as a placeholder
    }
    int arrivalTime = rand() % 5; // Adjust the range as needed
    sleep(arrivalTime);
    pthread_exit(0);
}

void barberThread(void *tmp) {
    int index = *(int *)(tmp);
    int myNext, C;
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
        sleep(CUT_TIME);
        printf("Barber-%d Finishes. ", index);
    }
}

void wait() {
    int x = rand() % (5000000 - 5000 + 1) + 50000;
    srand(time(NULL));
    usleep(x);
}