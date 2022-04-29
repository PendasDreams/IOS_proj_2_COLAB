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
int *O_amount = NULL;
int *H_amount = NULL;
int *mutex_id = NULL;
int *mutex_id1 = NULL;
int *O_mutexing = NULL;
int *H_mutexing = NULL;
int *H_mutexing_counter = NULL;
int *O_mutexing1 = NULL;
int *H_mutexing1 = NULL;
int *O_mutexing2 = NULL;
int *H_mutexing2 = NULL;

//shared memmory vars
int   shm_O_id = 0; // id of Nth atom
int   shm_H_id = 0;
int   shm_count_id = 1;
int   shm_queued_O = 0; // first O counter
int   shm_queued_H = 0;
int   shm_O_amount = 0; // amaount of atoms in process at exact time 
int   shm_H_amount = 0;
int   shm_mutex_id = 0; // id of molecule
int   shm_mutex_id1 = 0; // id of molecule
int   shm_O_mutexing = 1;
int   shm_H_mutexing = 1;
int   shm_H_mutexing_counter = 0;
int   shm_O_mutexing1 = 1;
int   shm_H_mutexing1 = 1;
int   shm_O_mutexing2 = 1;
int   shm_H_mutexing2 = 1;

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
sem_t   *barier,
        *barier1,
        *mutex,
        *mutex2,
        *oxygen,
        *hydrogen,
        *print;


// functions
int input_parse(int argc, char *argv[], params_t *params);
int prepare_sources();
int clear_sources();
void oxygen_f();
void hydrogen_f();


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


    *H_amount = params.Hydro;
    *O_amount = params.Oxy;

    for(int i = 1; i <= params.Oxy; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {
            oxygen_f();
            exit(0);
        }
    }

    for(int i = 1; i <= params.Hydro; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {  
            hydrogen_f();
            exit(0);
        }
    }



    while(wait(NULL) > 0);

    clear_sources();

    return 0;
}

// oxygen
void oxygen_f(){
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
    fprintf(file,"%d: O %d: going to queue\n", *count_id, *O_queued);        
    fflush(file);
    sem_post(print);


    sem_wait(mutex);

    

   

    (*O_mutexing)++;


    if(*O_amount <= 0 || *H_amount <=1){

        sem_wait(print);
        (*count_id)++;
        (*O_mutexing1)++;
        fprintf(file,"%d: O %d: not enough H\n", *count_id, *O_mutexing1); 
        fflush(file);
        sem_post(print);
        sem_post(mutex);
        exit(1);

    }
    else if(*H_mutexing >= 2){
        (*mutex_id)++;
        sem_post(hydrogen);
        sem_post(hydrogen);
        *H_mutexing = *H_mutexing - 2;
        sem_post(oxygen);
        *O_mutexing = *O_mutexing - 1;
    }
    else
    {
        sem_post(mutex);
    }



    sem_wait(oxygen);

    sem_wait(print);
    (*count_id)++;
    (*O_mutexing1)++;
    fprintf(file,"%d: O %d: creating molecule %d\n", *count_id, *O_mutexing1,*mutex_id); 
    fflush(file);
    sem_post(print);

    usleep(rand() % (params.Max_build_time + 1) * 1000);

    sem_wait(print);
    (*count_id)++;
    (*O_mutexing2)++;
    fprintf(file,"%d: O %d: molecule %d created \n", *count_id, *O_mutexing2,*mutex_id); 
    fflush(file);
    sem_post(print);

    sem_post(barier);
    sem_post(barier);
   




    
}

// hydrogen
void hydrogen_f(){

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
    fprintf(file,"%d: H %d: going to queue\n", *count_id, *H_queued); 
    fflush(file);
    sem_post(print);

    sem_wait(mutex);

    (*H_mutexing)++;

    if(*O_amount <= 0 || *H_amount <=1){

        sem_wait(print);
        (*count_id)++;
        (*H_mutexing1)++;
        fprintf(file,"%d: H %d: not enough O or H \n", *count_id, *H_mutexing1); 
        fflush(file);
        sem_post(print);
        sem_post(mutex);
        sem_post(oxygen);

        exit(1);

    }

    
    else if(*H_mutexing >= 2 && *O_mutexing >= 1)
    {
        (*mutex_id)++;
        sem_post(hydrogen);
        sem_post(hydrogen);
        *H_mutexing = *H_mutexing - 2;
        sem_post(oxygen);
        *O_mutexing = *O_mutexing - 1;
    }
    else
    {
        sem_post(mutex);
    }

    

    sem_wait(hydrogen);

    sem_wait(print);
    (*count_id)++;
    (*H_mutexing1)++;
    fprintf(file,"%d: H %d: creating molecule %d\n", *count_id, *H_mutexing1,*mutex_id);
    fflush(file);
    sem_post(print);

    sem_wait(barier);

    sem_wait(print);
    (*count_id)++;
    (*mutex_id1)++;
    (*H_mutexing2)++;
    fprintf(file,"%d: H %d: molecule %d created\n", *count_id, *H_mutexing2,*mutex_id); 
    fflush(file);
    sem_post(print);

    if(*mutex_id1 == 2){
        *O_amount = *O_amount - 1;
        *H_amount = *H_amount - 2;
        *mutex_id1 = 0;
        sem_post(mutex);
        
    }
    



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


    if(params->Hydro <=0)
    {
        fprintf(stderr, "Negative amount of Hydrogens\n");
        exit(1);
    }

    if(params->Oxy <= 0)
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
    mutex= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    mutex2= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    barier= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    barier1= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    sem_init(oxygen, 1, 0);
    sem_init(hydrogen, 1, 0);
    sem_init(print, 1, 1);
    sem_init(mutex, 1, 1);
    sem_init(mutex2, 1, 2);
    sem_init(barier, 1, 0);
    sem_init(barier1, 1, 0);

    // shared memmory
    shm_O_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_queued_H = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_queued_O = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_O_amount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_amount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_mutex_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_mutex_id1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_mutexing = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_mutexing_counter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_O_mutexing = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_mutexing1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_O_mutexing1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_H_mutexing2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    shm_O_mutexing2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);


    O_id = (int *) shmat(shm_O_id, NULL, 0);
    H_id = (int *) shmat(shm_H_id, NULL, 0);
    count_id = (int *) shmat(shm_count_id, NULL, 0);
    O_queued = (int *) shmat(shm_queued_O, NULL, 0);
    H_queued = (int *) shmat(shm_queued_H, NULL, 0);
    O_amount = (int *) shmat(shm_O_amount, NULL, 0);
    H_amount = (int *) shmat(shm_H_amount, NULL, 0);
    mutex_id = (int *) shmat(shm_mutex_id, NULL, 0);
    mutex_id1 = (int *) shmat(shm_mutex_id1, NULL, 0);
    H_mutexing = (int *) shmat(shm_H_mutexing, NULL, 0);
    H_mutexing_counter = (int *) shmat(shm_H_mutexing_counter, NULL, 0);
    O_mutexing = (int *) shmat(shm_O_mutexing, NULL, 0);
    H_mutexing1 = (int *) shmat(shm_H_mutexing1, NULL, 0);
    O_mutexing1 = (int *) shmat(shm_O_mutexing1, NULL, 0);
    H_mutexing2 = (int *) shmat(shm_H_mutexing2, NULL, 0);
    O_mutexing2 = (int *) shmat(shm_O_mutexing2, NULL, 0);
    

    return 0;
}

int clear_sources(){

    // semaphore celanup
    sem_destroy(oxygen);
    sem_destroy(hydrogen);
    sem_destroy(print);
    sem_destroy(mutex);
    sem_destroy(mutex2);
    sem_destroy(barier);
    munmap(oxygen, sizeof(sem_t));
    munmap(hydrogen, sizeof(sem_t));
    munmap(print, sizeof(sem_t));
    munmap(mutex, sizeof(sem_t));
    munmap(mutex2, sizeof(sem_t));
    munmap(barier, sizeof(sem_t));
    munmap(barier1, sizeof(sem_t));

    // shared memmory cleanup
    shmctl(shm_O_id, IPC_RMID, NULL);
    shmctl(shm_count_id, IPC_RMID, NULL);
    shmctl(shm_H_id, IPC_RMID, NULL);
    shmctl(shm_queued_O, IPC_RMID, NULL);
    shmctl(shm_queued_H, IPC_RMID, NULL);
    shmctl(shm_O_amount, IPC_RMID, NULL);
    shmctl(shm_H_amount, IPC_RMID, NULL);
    shmctl(shm_mutex_id, IPC_RMID, NULL);
    shmctl(shm_mutex_id1, IPC_RMID, NULL);
    shmctl(shm_H_mutexing, IPC_RMID, NULL);
    shmctl(shm_H_mutexing_counter, IPC_RMID, NULL);
    shmctl(shm_O_mutexing, IPC_RMID, NULL);
    shmctl(shm_H_mutexing1, IPC_RMID, NULL);
    shmctl(shm_O_mutexing1, IPC_RMID, NULL);
    shmctl(shm_H_mutexing2, IPC_RMID, NULL);
    shmctl(shm_O_mutexing2, IPC_RMID, NULL);


    shmdt(O_id);
    shmdt(O_queued);
    shmdt(H_id);
    shmdt(H_queued);
    shmdt(count_id);
    shmdt(O_amount);
    shmdt(H_amount);
    shmdt(mutex_id);
    shmdt(mutex_id1);
    shmdt(H_mutexing);
    shmdt(H_mutexing_counter);
    shmdt(O_mutexing);
    shmdt(H_mutexing1);
    shmdt(O_mutexing1);
    shmdt(H_mutexing2);
    shmdt(O_mutexing2);

    return 0;
}