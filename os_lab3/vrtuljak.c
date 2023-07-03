#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#define SEM_COUNT 4
#define CAROUSEL_DURATION 3

int carousel_limit;
int passanger_count;
int ID;
sem_t *sem;
// 0 - CAROUSEL
// 1 - SIT_DOWN
// 2 - END
// 3 - LEFT

void sjedi(int id) {
    printf("Sjeo posjetitelj broj %d\n", id);
}

void ustani(int id) {
    printf("Ustao posjetitelj broj %d\n", id);
}

void pokreni_vrtuljak(void) {
    printf("Vrtuljak pokrenut!\n");
    sleep(CAROUSEL_DURATION);
}

void zaustavi_vrtuljak(void) {
    printf("Vrtuljak zaustavljen!\n");
}

void posjetitelj(int id) {
    sem_wait(&sem[0]);  //Wait for the carousel to open
    sjedi(id);            
    sem_post(&sem[1]);  //Let the carousel that you have taken a seat
    sem_wait(&sem[2]);  //Wait for the ride to end
    ustani(id);
    sem_post(&sem[3]);  //Let the carousel know that you left
}

void vrtuljak(void) {
    int ride_count = ceil((double) passanger_count / carousel_limit);
    ride_count = ride_count == 0 ? 1 : ride_count;
    for (int i = 0; i < ride_count; i++) {
        for (int i = 0; i < carousel_limit; i++) //Let passengers board the carousel
            sem_post(&sem[0]);

        for (int i = 0; i < carousel_limit; i++) //Wait for the passangers to board
            sem_wait(&sem[1]);

        pokreni_vrtuljak();
        zaustavi_vrtuljak();

        for (int i = 0; i < carousel_limit; i++) //Let passengers know that the ride is over
            sem_post(&sem[2]);

        for (int i = 0; i < carousel_limit; i++) //Wait for the passangers to leave
            sem_wait(&sem[3]);
    }
}

int main(void) {

    printf("Unesite broj mjesta na vrtuljku: ");
    scanf("%d", &carousel_limit);
    printf("Unesite broj posjetitelja: ");
    scanf("%d", &passanger_count);

    ID = shmget(IPC_PRIVATE, SEM_COUNT * sizeof(sem_t), 0600);
    sem = shmat(ID, NULL, 0);

    //Sem init
    for (int i = 0; i < SEM_COUNT; i++)
        sem_init(&sem[i], 1, 0);

    //Vrtuljak init
    if (fork() == 0) {
        vrtuljak();
        exit(0);
    }

    //Posjetitelji init
    for (int i = 1; i <= passanger_count; i++) {
        if (fork() == 0) {
            posjetitelj(i);
            exit(0);
        }
    }

    //Wait for processes to end
    for(int i = 0; i <= passanger_count; i++)
        wait(NULL);

    sem_destroy(sem);
    shmdt(sem);
    shmctl(ID, IPC_RMID, NULL);
    return 0;
}