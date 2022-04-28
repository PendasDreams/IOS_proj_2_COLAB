// Synchronization of processes using shared memmory and semaphores //
// IOS project 2
// 
// Login:  xnovos14
// Name:   Denis Novosád
// Date:   04.2022



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>

FILE *file;

//strcture of paramerters
typedef struct Params
{
    int Oxy; // Atoms of oxygen
    int Hydro; // Atoms of hydrogen
    int Max_queue_time; // Max time before queueing 
    int Max_build_time; // Max time before building molecule

} params_t;


//semaphores
sem_t   *mutex;


// functions
int input_parse(int argc, char *argv[], params_t *params);
int prepare_sources();
int clear_sources();


int main(){
    printf("hello world\n");
    return 0;
}

// check input 
int input_parse(int argc, char *argv[], params_t *params){

    int error = 0;
    long val;
    char *next;

    if(argc != 5)
    {
        error = 1;
    }

    for(int i = 1; i < argc; i++)
    {
        val = strtol(argv[i], &next,10);

        if((next == argv[i]) || (*next != '\0')){
            error = 1;
        }
    }

    //only for translator to stop talking
    val = val + 1;


    params->Oxy =strtol(argv[1],NULL,10);
    params->Hydro =strtol(argv[2],NULL,10);
    params->Max_queue_time =strtol(argv[3],NULL,10);
    params->Max_build_time =strtol(argv[4],NULL,10);
    

    if(params->Max_build_time > 1000 || params->Max_build_time < 0)
    {
        error = 1;
    }
    if(params->Max_queue_time > 1000 || params->Max_queue_time < 0)
    {
        error = 1;
    }

    if(error == 1){
        fprintf(stderr, "Parameters was used wrong.\n");
        fprintf(stderr, "Insert arguments like:  ./program [N]O [N]H TI TB\n");
        fprintf(stderr, "For more info look at the project website\n");
        exit(1);
    }

    return 0;
}


int prepare_sources(){

    // semaphore inicialization
    mutex= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    sem_init(mutex, 1, 1);

    return 0;
}

int clear_sources(){

    // semaphore celanup
    sem_destroy(mutex);

    return 0;
}