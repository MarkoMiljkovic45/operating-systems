#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define OUTPUT_FILE_NAME "radno_cekanje.out"

int sharedMemoryID;
int *sharedMemory;

int sharedVariable;

void freeSharedMemory(int sig);
void childProcess(int n);
void *outputThread(void *args);
void generateInput();
void processInput();
void saveOutput();

int main(int argc, char* argv[]) {
    printf("Pokrenut ULAZNI PROCES\n");
    srand(time(0));

    //Citanje argumenta programa
    int n = strtol(argv[1], NULL, 10);

    //Osiguravanje ciscenja dijeljene memorije u slucaju preranog prekida
    sigset(SIGINT, freeSharedMemory);
    sigset(SIGTERM, freeSharedMemory);

    //Zauzimanje dijeljene memorije
    sharedMemoryID = shmget(IPC_PRIVATE, sizeof(int), 0600);
    if (sharedMemoryID == -1) exit(1);

    sharedMemory = (int*) shmat(sharedMemoryID, NULL, 0);
    *sharedMemory = 0;

    //Stvaranje novog procesa
    if (fork() == 0) {
        childProcess(n);
        printf("Zavrsila RADNA DRETVA\n");
        exit(0);
    }

    //Rad ULAZNE DRETVE
    int i = 0;
    while (i < n) {
        if (*sharedMemory == 0) {
            int sleepTime = rand() % 5 + 1;
            for (int j = 0; j < sleepTime; j++) sleep(1);

            generateInput();
            i++;
        }
    }

    //Cekamo da zavrsi proces dijete i oslobadamo dijeljenu memoriju
    wait(NULL);
    printf("Zavrsio ULAZNI PROCES\n");
    freeSharedMemory(0);
    return 0;
}

void freeSharedMemory(int sig) {
    shmdt((char *) sharedMemory);
    shmctl(sharedMemoryID, IPC_RMID, NULL);
    exit(0);
}

void childProcess(int n) {
    printf("Pokrenuta RADNA DRETVA\n");

    sharedVariable = 0;

    pthread_t outputThreadID;

    if (pthread_create(&outputThreadID, NULL, outputThread, &n) != 0) {
        printf("Greska pri stvaranju IZLAZNE DRETVE\n");
        exit(1);
    }

    int i = 0;
    while (i < n) {
        if (*sharedMemory != 0 && sharedVariable == 0) {
            processInput();
            i++;
        }
    }

    pthread_join(outputThreadID, NULL);
}

void *outputThread(void *args) {
    printf("Pokrenuta IZLAZNA DRETVA\n\n");

    int n = *((int*) args);
    FILE *f = fopen(OUTPUT_FILE_NAME, "w");

    int i = 0;
    while (i < n) {
        if (sharedVariable != 0) {
            saveOutput(f);
            i++;
        }
    }

    fclose(f);
    printf("Zavrsila IZLAZNA DRETVA\n");
}

void generateInput() {
    int r = rand() % 100 + 1;
    printf("ULAZNA DRETVA: broj %d\n", r);
    *sharedMemory = r;
    
}

void processInput() {
    (*sharedMemory)++;
    printf("RADNA DRETVA: procitan broj %d i povecan na %d\n", *sharedMemory - 1, *sharedMemory);
    sharedVariable = *sharedMemory;
    *sharedMemory = 0;
}

void saveOutput(FILE *f) {
    fprintf(f, "%d\n", sharedVariable);
    printf("IZLAZNI PROCES: broj upisan u datoteku %d\n\n", sharedVariable);
    sharedVariable = 0;
}