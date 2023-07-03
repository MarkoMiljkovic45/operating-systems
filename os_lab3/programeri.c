#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#define PROGRAMER_TYPE_COUNT 2
#define EATING_DURATION_IN_SECS 1

int linux_programer_count;
int ms_programer_count;
int starvation_limit; //This limit is used to fix the starvation problem

int linux_programer_type = 0;
int ms_programer_type = 1;

int programer_count[PROGRAMER_TYPE_COUNT];
int resturant_queue[PROGRAMER_TYPE_COUNT];
int entered_count[PROGRAMER_TYPE_COUNT];

pthread_mutex_t monitor;
pthread_cond_t cond_queue[PROGRAMER_TYPE_COUNT];

void udji(int vrsta) {
    pthread_mutex_lock(&monitor);
    resturant_queue[vrsta]++;                               //Enter the resturant queue

    while (programer_count[1 - vrsta] > 0 ||                //Wait until there are no more programers of the opposite type in the resutrant OR
           entered_count[vrsta] == starvation_limit) {      //the number of programers of my type that entered is equal to the starvation limit
              
        if (vrsta == linux_programer_type)
            printf("Linux programer queue = %d\n", resturant_queue[vrsta]);
        else
            printf("Microsoft programer queue = %d\n", resturant_queue[vrsta]);

        pthread_cond_wait(&cond_queue[vrsta], &monitor);
    }

    if (vrsta == linux_programer_type)
        printf("Linux programer entered the resturant\n");
    else
        printf("Microsoft programer entered the resturant\n");

    programer_count[vrsta]++;                               //Increment the number of programers in the resturan of a certain type
    resturant_queue[vrsta]--;                               //Leave the resturant queue

    if (resturant_queue[1 - vrsta] > 0)                     //If there are programers of the opposite type waitng, start counting the number
        entered_count[vrsta]++;                             //of programers of my type that entered the resturant (starvation limit)

    pthread_mutex_unlock(&monitor);
}

void obavi(void) {
    sleep(EATING_DURATION_IN_SECS);
}

void izadji(int vrsta) {
    pthread_mutex_lock(&monitor);
    programer_count[vrsta]--;                               //Decrement the number of programers in the resturan of a certain type

    if (vrsta == linux_programer_type)
        printf("Linux programer left the resturant\n");
    else
        printf("Microsoft programer left the resturant\n");

    if (programer_count[vrsta] == 0) {                      //If all the programers of my type left the resturant
        entered_count[1 - vrsta] = 0;                       //reset the enterd count for the opposite type
        pthread_cond_broadcast(&cond_queue[1 - vrsta]);
    }
    pthread_mutex_unlock(&monitor);
}

void *programer(void *vrsta) {
    udji(*((int *) vrsta));
    obavi();
    izadji(*((int *) vrsta));
}

void create_thread(pthread_t *thread_id, int vrsta) {
    if (vrsta == linux_programer_type) {
        if (pthread_create(thread_id, NULL, programer, &linux_programer_type) != 0) {
            printf("Failed to create thread!\n");
            exit(1);
        }
    } else {
        if (pthread_create(thread_id, NULL, programer, &ms_programer_type) != 0) {
            printf("Failed to create thread!\n");
            exit(1);
        }
    }
}


int main(void) {

    srand(time(NULL));

    printf("Unesite broj linux programera: ");
    scanf("%d", &linux_programer_count);
    printf("Unesite broj microsoft programera: ");
    scanf("%d", &ms_programer_count);
    printf("Unesite granicu izgladnjivanja: ");
    scanf("%d", &starvation_limit);

    pthread_t linux_thread_ids[linux_programer_count];
    pthread_t ms_thread_ids[ms_programer_count];
    
    //Monitor init
    pthread_mutex_init(&monitor, NULL);

    //Condition queue init
    for (int i = 0; i < PROGRAMER_TYPE_COUNT; i++) {
        pthread_cond_init(&cond_queue[i], NULL);
    }

    //Programer threads init
    int linux_idx = 0;
    int ms_idx = 0;
    for (int i = 0; i < linux_programer_count + ms_programer_count; i++) {
        int random = rand();
        if (random % 2 == 0 && linux_idx < linux_programer_count) {
            create_thread(&linux_thread_ids[linux_idx++], linux_programer_type);
            continue;
        }
        if (ms_idx < ms_programer_count) {
            create_thread(&ms_thread_ids[ms_idx++], ms_programer_type);
            continue;
        }
        create_thread(&linux_thread_ids[linux_idx++], linux_programer_type);
    }

    //Wait for threads to finish
    for (int i = 0; i < linux_programer_count; i++)
        pthread_join(linux_thread_ids[i], NULL);

    for (int i = 0; i < ms_programer_count; i++)
        pthread_join(ms_thread_ids[i], NULL);
    
    return 0;
}