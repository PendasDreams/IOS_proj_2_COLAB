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

//pointers
int *count_id = NULL;
int *O_id = NULL;
int *H_id = NULL;
int *O_queued = NULL;
int *H_queued = NULL;

//shared memmory vars
int   shm_O_id = 0; // id of Nth atom
int   shm_H_id = 0;
int   shm_count_id = 1;
int   shm_queued_O = 0; // first O counter
int   shm_queued_H = 0;

//strcture of paramerters
typedef struct Params
{
    int Oxy; // Atoms of oxygen
    int Hydro; // Atoms of hydrogen
    int Max_queue_time; // Max time before queueing 
    int Max_build_time; // Max time before building molecule

} params_t;

params_t params;


//semaphores
sem_t   *mutex,
        *oxygen,
        *hydrogen,
        *print;


// functions
int input_parse(int argc, char *argv[], params_t *params);
int prepare_sources();
int clear_sources();


// main
int main(int argc, char **argv){
        
    input_parse(argc,argv, &params);


    // check output file
    if((file = fopen("proj2.out", "w")) == NULL) 
    {
        fprintf(stderr, "ERROR something went wrong with output fille\n");
        exit(1);
        return 1;
    }
    setbuf(file, NULL);

    prepare_sources();

    for(int i = 1; i <= params.Oxy; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {
            sem_wait(print);
            (*count_id)++;
            (*O_id)++;
            fprintf(file,"%d: O %d: started\n", *count_id, *O_id);        
            fflush(file);
            sem_post(print);

            usleep(rand() % (params.Max_queue_time + 1) * 1000);

            sem_wait(print);
            (*count_id)++;
            (*O_queued)++;
            fprintf(file,"%d: O %d: Going to queue\n", *count_id, *O_queued);        
            fflush(file);
            sem_post(print);
            
            exit(0);
        }
    }

    for(int i = 1; i <= params.Hydro; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {
            sem_wait(print);
            (*count_id)++;
            (*H_id)++;
            fprintf(file,"%d: H %d: started\n", *count_id, *H_id);  
            fflush(file);
            sem_post(print);

            usleep(rand() % (params.Max_queue_time + 1) * 1000);

            sem_wait(print);
            (*count_id)++;
            (*H_queued)++;
            fprintf(file,"%d: H %d: Going to queue\n", *count_id, *H_queued); 
            sem_post(print);
            
            exit(0);
        }
    }



    while(wait(NULL) > 0);

    clear_sources();

    return 0;
}

// check input 
int input_parse(int argc, char *argv[], params_t *params){

    int error = 0;
    long val;
    char *next;

    if(argc != 5)
    {
        fprintf(stderr, "Wrong amount of parameters.\n");
        exit(1);
    }

    for(int i = 1; i < argc; i++)
    {
        val = strtol(argv[i], &next,10);

        if((next == argv[i]) || (*next != '\0')){
            fprintf(stderr,"Parameters must be numbers\n");
            exit(1);
        }
    }


    //only for translator to stop talking
    val = val + 1;


    params->Oxy =strtol(argv[1],NULL,10);
    params->Hydro =strtol(argv[2],NULL,10);
    params->Max_queue_time =strtol(argv[3],NULL,10);
    params->Max_build_time =strtol(argv[4],NULL,10);


    if(params->Hydro < 0)
    {
        fprintf(stderr, "Negative amount of Hydrogens\n");
        exit(1);
    }

    if(params->Oxy < 0)
    {
        fprintf(stderr, "Negative amount of Oxygens.\n");
        exit(1);
    }

    if(params->Max_build_time > 1000 || params->Max_build_time < 0)
    {
        
        fprintf(stderr,"Third argument out of range [0-1000].\n");
        exit(1);
    }
    if(params->Max_queue_time > 1000 || params->Max_queue_time < 0)
    {
        fprintf(stderr,"Fourth argument out of range [0-1000].\n");
        exit(1);
    }
    

    return error;
}


int prepare_sources(){

    // semaphore inicialization
    oxygen= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    hydrogen= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    print= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    sem_init(oxygen, 1, 1);
    sem_init(hydrogen, 1, 1);
    sem_init(print, 1, 1);

    // shared memmory
    shm_O_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_queued_H = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_queued_O = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);

    O_id = (int *) shmat(shm_O_id, NULL, 0);
    H_id = (int *) shmat(shm_H_id, NULL, 0);
    count_id = (int *) shmat(shm_count_id, NULL, 0);
    O_queued = (int *) shmat(shm_queued_O, NULL, 0);
    H_queued = (int *) shmat(shm_queued_H, NULL, 0);
    return 0;
}

int clear_sources(){

    // semaphore celanup
    sem_destroy(oxygen);
    sem_destroy(hydrogen);
    sem_destroy(oxygen);
    munmap(oxygen, sizeof(sem_t));
    munmap(hydrogen, sizeof(sem_t));
    munmap(print, sizeof(sem_t));

    // shared memmory cleanup
    shmctl(shm_O_id, IPC_RMID, NULL);
    shmctl(shm_count_id, IPC_RMID, NULL);
    shmctl(shm_H_id, IPC_RMID, NULL);
    shmctl(shm_queued_O, IPC_RMID, NULL);
    shmctl(shm_queued_H, IPC_RMID, NULL);

    shmdt(O_id);
    shmdt(O_queued);
    shmdt(H_id);
    shmdt(H_queued);
    shmdt(count_id);
    

    return 0;
}