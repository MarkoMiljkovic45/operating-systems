#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#define PAGE_SIZE 64
#define TABLE_SIZE 16       // 4 most significant bits from the logical address
#define ADDRESS_SPACE 1024 // 10 bit logical addresses
#define SLEEP_TIME 1
#define SET 1
#define UNSET 0

int N = 0;  // Number of processes to simulate
int M = 0;  // Number of frames in memeory

/**
 * @brief Get the physical address from logical address
 * 
 * @param page_table Array containing the translation table. Each element has 10 bit physical address, 1 presence bit, 5 bit LRU metadata
 * @param logical_address Logical address to be translated
 * @return Physical address if presence bit set, -1 otherwise
 */
short get_physical_address(short page_table[], short logical_address);

/**
 * @brief Get the page entry from logical address
 * 
 * @param page_table Array containing the translation table. Each element has 10 bit physical address, 1 presence bit, 5 bit LRU metadata
 * @param logical_address Logical address
 * @return Page entry for logical_address
 */
short get_table_entry(short page_table[], short logical_address);

/**
 * @brief Assign a logical address of a process to a physcial address
 * 
 * @param disk Process data on a simulated disk. Contains PAGE_SIZE pages
 * @param page_frames Contains data currently in memory
 * @param page_table Translation table for logical addresses for all processes
 * @param process Process requesting a physical address
 * @param logical_address The logical address to be assigned to a phsycal address
 * @return The assigned physical address
 */
short assign_address(char disk[N][ADDRESS_SPACE], char page_frames[M][PAGE_SIZE], short page_table[N][TABLE_SIZE], int process, short logical_address);

/**
 * @brief Find a free frame in memory
 * 
 * @param page_table Translation table for logical addresses for all processes
 * @return Index of free frame in page_frames or -1 if page_frames is full
 */
int find_free_frame(short page_table[N][TABLE_SIZE]);

/**
 * @brief Remove oldest frame using LRU replacement algorithm
 * 
 * @param disk Process data on a simulated disk. Contains PAGE_SIZE pages
 * @param page_frames Contains data currently in memory
 * @param page_table Translation table for logical addresses for all processes
 * @return Index of newly freed frame in page_frames
 */
int remove_oldest_frame(char disk[N][ADDRESS_SPACE], char page_frames[M][PAGE_SIZE], short page_table[N][TABLE_SIZE]);

/**
 * @brief Inserts a page from the disk into the memory
 * 
 * @param disk Process data on a simulated disk. Contains PAGE_SIZE pages
 * @param page_frames Contains data currently in memory
 * @param physical_address Address of a frame in memoryn in which data from disk will be inserted
 * @param logical_address Logical addres of data on disk
 */
void insert_frame(char disk[], char page_frames[M][PAGE_SIZE], short physical_address, short logical_address);

/**
 * @brief Updates presence bit of a page in page_table
 * 
 * @param page_table Translation table for logical addresses for all processes
 * @param process Process whose page is being updated
 * @param page Page index of the process
 * @param state State of pressenc bit to be set
 */
void update_presence_bit(short page_table[N][TABLE_SIZE], int process, int page, int state);

/**
 * @brief Update the timer metadata for table entry
 * 
 * @param page_table Array containing the translation tables. Each element has 10 bit physical address, 1 presence bit, 5 bit LRU metadata
 * @param process Process whose page timer needs to be updated
 * @param logical_address Logical address whose timer will be updated
 * @param t Timer variable
 */
void update_timer(short page_table[N][TABLE_SIZE], int process, short logical_address, int *t);

/**
 * @brief Resets timers on all table entries
 * 
 * @param page_table Array containing the translation tables. Each element has 10 bit physical address, 1 presence bit, 5 bit LRU metadata
 */
void reset_timers(short page_table[N][TABLE_SIZE]);

/**
 * @brief Increment the data in memory
 * 
 * @param page_frames Contains data in memory
 * @param physical_address The address of the data to be incremented
 */
void increment_data(char page_frames[M][PAGE_SIZE], short physical_address);

int main(int argc, char *argv[]) {

    N = atoi(argv[1]);  // Number of processes to simulate
    M = atoi(argv[2]);  // Number of frames in memeory

    if (N == 0 || M == 0) return -1;

    char disk[N][ADDRESS_SPACE];
    char page_frames[M][PAGE_SIZE];
    short page_table[N][TABLE_SIZE];

    int t = 0;
    srand(time(NULL));

    // Process initialization
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < ADDRESS_SPACE; j++)
            disk[i][j] = 0;
        for (int j = 0; j < TABLE_SIZE; j++)
            page_table[i][j] = 0;
    }

    while (1) {
        for (int process = 0; process < N; process++) {
            short logical_address = (rand() % ADDRESS_SPACE) & 0x3FE;                               // Random even logical address
            short physical_address = get_physical_address(page_table[process], logical_address);

            char separator[] = "---------------------------";
            printf("%s\n", separator);
            printf("Process: %d\n", process);
            printf("\tt: %d\n", t);
            printf("\tLogical address: 0x%04x (Page: %d)\n", logical_address, logical_address >> 6 & 0xF);

            if (physical_address == -1) {
                printf("\tMiss!\n");
                physical_address = assign_address(disk, page_frames, page_table, process, logical_address);
            }
            update_timer(page_table, process, logical_address, &t);

            //Print info
            int page_idx = (physical_address >> 6) & 0xF;
            int data_idx = physical_address & 0x3F;
            printf("Physical address: 0x%04x (Frame: %d)\n", physical_address, physical_address >> 6 & 0xF);
            printf("Page table entry: 0x%04x\n", get_table_entry(page_table[process], logical_address));
            printf("Data: %d\n", page_frames[page_idx][data_idx]);
            increment_data(page_frames, physical_address);

            t++;
            sleep(SLEEP_TIME);
        }
    }

    return 0;
}

short get_physical_address(short page_table[], short logical_address) {
    short table_entry = get_table_entry(page_table, logical_address);
    short presence_bit = (table_entry >> 5) & 0x1;
    if (presence_bit) {
        int physical_address = table_entry >> 6 & 0x3C0;
        physical_address += logical_address & 0x3F;
        return physical_address;
    }
    else
        return -1;
}

short get_table_entry(short page_table[], short logical_address) {
    int table_idx = (logical_address >> 6) & 0xF;
    return page_table[table_idx];
}

short assign_address(char disk[N][ADDRESS_SPACE], char page_frames[M][PAGE_SIZE], short page_table[N][TABLE_SIZE], int process, short logical_address) {
    int free_frame_idx = find_free_frame(page_table);
    if (free_frame_idx == -1)
        free_frame_idx = remove_oldest_frame(disk, page_frames, page_table);

    short physical_address = free_frame_idx << 6;
    physical_address += logical_address & 0x3F;

    insert_frame(disk[process], page_frames, physical_address, logical_address);
    update_presence_bit(page_table, process, (logical_address >> 6) & 0xF, SET);

    // Update page table physical address
    short *table_entry = &page_table[process][logical_address >> 6 & 0xF];
    *table_entry = *table_entry & 0x3F;
    *table_entry += physical_address << 6;

    return physical_address;
}

int find_free_frame(short page_table[N][TABLE_SIZE]) {
    for (int i = 0; i < M; i++) {
        int free = 1;
        for (int process = 0; process < N; process++) {
            for (int page = 0; page < TABLE_SIZE; page++) {
                int table_entry = page_table[process][page];
                int page_frame = table_entry >> 12;
                int page_pressence_bit = table_entry >> 5 & 0x1;
                if (page_frame == i && page_pressence_bit) {        // Test the pressence bit for frame i in process
                    free = 0;
                    break;
                }
            }
            if (free == 0) break;
        }
        if (free == 1) return i;
    }

    return -1;
}

int remove_oldest_frame(char disk[N][ADDRESS_SPACE], char page_frames[M][PAGE_SIZE], short page_table[N][TABLE_SIZE]) {
    int lowest_time = 32;                                   // LRU metadata is 5 bits so 32 is larger than the biggest possible timer value
    int frame_idx;
    int process_idx;
    int page_idx;

    // Find oldest frame
    for (int process = 0; process < N; process++) {
        for (int page = 0; page < TABLE_SIZE; page++) {
            short table_entry = page_table[process][page];
            if ((table_entry >> 5) & 0x1) {                 // Test presence bit
                int table_entry_time = table_entry & 0x1F;
                if (table_entry_time < lowest_time) {
                    lowest_time = table_entry_time;
                    frame_idx = table_entry >> 12;
                    process_idx = process;
                    page_idx = page;
                }
            }
        }
    }

    // Move data from memory to disk
    int disk_address = page_idx * PAGE_SIZE;
    for (int i = 0; i < PAGE_SIZE; i++) {
        disk[process_idx][disk_address + i] = page_frames[frame_idx][i];
        page_frames[frame_idx][i] = 0;
    }

    // Update presence bit
    update_presence_bit(page_table, process_idx, page_idx, UNSET);
    printf("\tRemoving page %d from Proces %d\n", page_idx, process_idx);
    printf("\tLRU metadata from the removed page: %d\n", lowest_time);
    printf("\tAssigned frame: %d\n", frame_idx);

    return frame_idx;
}

void insert_frame(char disk[], char page_frames[M][PAGE_SIZE], short physical_address, short logical_address) {
    int disk_address = ((logical_address >> 6) & 0xF) * PAGE_SIZE;
    int frame_idx = (physical_address >> 6) & 0xF;

    // Move data from disk to memory
    for (int i = 0; i < PAGE_SIZE; i++) {
        page_frames[frame_idx][i] = disk[disk_address + i];
    }
}

void update_presence_bit(short page_table[N][TABLE_SIZE], int process, int page, int state) {
    if (state == SET)
        page_table[process][page] = page_table[process][page] | 0x0020;
    else
        page_table[process][page] = page_table[process][page] & 0xFFDF;
}

void update_timer(short page_table[N][TABLE_SIZE], int process, short logical_address, int *t) {
    short *table_entry = &page_table[process][(logical_address >> 6) & 0xF];
    if (*t >= 31) {
        reset_timers(page_table);
        *t = 0;
        (*table_entry)++;
    } else {
        *table_entry = (*table_entry) & 0xFFE0;
        *table_entry += *t;
    }
    
}

void reset_timers(short page_table[N][TABLE_SIZE]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < TABLE_SIZE; j++) {
            page_table[i][j] = page_table[i][j] & 0xFFE0;
        }
    }
}

void increment_data(char page_frames[M][PAGE_SIZE], short physical_address) {
    int page_idx = (physical_address >> 6) & 0xF;
    int data_idx = physical_address & 0x3F;
    page_frames[page_idx][data_idx]++;
}