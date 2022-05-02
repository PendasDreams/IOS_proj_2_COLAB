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

//pointers for shm
int *count_id = NULL; //process counter
int *O_id = NULL;     //first phase O counter
int *H_id = NULL;
int *O_queued = NULL;  // second phase O counter
int *H_queued = NULL;
int *O_amount = NULL;   // third phace O counter
int *H_amount = NULL;
int *mutex_id = NULL;   // first phace mutex counter
int *mutex_id1 = NULL;  // second phase mutex counter
int *O_mutexing = NULL; // first counter of O already mutexing
int *H_mutexing = NULL;
int *O_mutexing1 = NULL; // second counter of O already mutexing
int *H_mutexing1 = NULL;
int *O_mutexing2 = NULL; // third counter of O already mutexing
int *H_mutexing2 = NULL;
int *H_mutexing3 = NULL;  // fourth counter of O already mutexing
int *O_mutexing3 = NULL;

//shared memmory vars
int   shm_O_id = 0;
int   shm_H_id = 0;
int   shm_count_id = 1;
int   shm_queued_O = 0;
int   shm_queued_H = 0;
int   shm_O_amount = 0;
int   shm_H_amount = 0;
int   shm_mutex_id = 0;
int   shm_mutex_id1 = 0;
int   shm_O_mutexing = 1;
int   shm_H_mutexing = 1;
int   shm_O_mutexing1 = 1;
int   shm_H_mutexing1 = 1;
int   shm_O_mutexing2 = 1;
int   shm_H_mutexing2 = 1;
int   shm_H_mutexing3 = 0;
int   shm_O_mutexing3 = 0;

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
        fclose(file);
        exit(1);
        return 1;
    }
    setbuf(file, NULL);

    //prepare shm and semaphores
    prepare_sources();


    *H_amount = params.Hydro;
    *O_amount = params.Oxy;

    //create NO processes
    for(int i = 1; i <= params.Oxy; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {
            oxygen_f();
            fclose(file);
            exit(0);
        }
    }

    //create NH processes
    for(int i = 1; i <= params.Hydro; i++)
    {
        pid_t id = fork();
        if(id == 0)
        {  
            hydrogen_f();
            fclose(file);
            exit(0);
        }
    }


    //wait for kids
    while(wait(NULL) > 0);

    //clear shm and semaphores
    clear_sources();

    return 0;
}

// oxygen
void oxygen_f(){

    //process started
    sem_wait(print);
    (*count_id)++;
    (*O_id)++;
    fprintf(file,"%d: O %d: started\n", *count_id, *O_id);        
    fflush(file);
    sem_post(print);

    //wait some time representing time getting into queue
    usleep(rand() % (params.Max_queue_time + 1) * 1000);

    sem_wait(print);
    (*count_id)++;
    (*O_queued)++;
    fprintf(file,"%d: O %d: going to queue\n", *count_id, *O_queued);        
    fflush(file);
    sem_post(print);

    //kill child if there is not enough other atoms
    if(params.Hydro <=1)
    {
        sem_wait(print);
        (*count_id)++;
        fprintf(file,"%d: O %d: not enough H\n", *count_id, *O_queued);        
        fflush(file);
        sem_post(print);
        sem_post(mutex);
        fclose(file);
        exit(1);
    }

    //mutex place where atoms goes one by one
    sem_wait(mutex);

    (*O_mutexing)++;

    //if there is enough atoms to create molecule than let go two hydrogens and one oxygen
    if(*H_mutexing >= 2)
    {
        (*mutex_id)++;
        sem_post(hydrogen);
        sem_post(hydrogen);
        *H_mutexing = *H_mutexing - 2;
        sem_post(oxygen);
        *O_mutexing = *O_mutexing - 1;
    }
    //if there is not enough atoms to create molecule let other atoms get into mutex place
    else
    {
        sem_post(mutex);
    }

    //queue of oxygens
    sem_wait(oxygen);

    //if there is not enough oxygens to create molecule kill childs
    if(*H_amount <= 1 || *O_amount <=0)
    {
        sem_wait(print);
        (*count_id)++;
        (*O_mutexing1)++;
        fprintf(file,"%d: O %d: not enough H \n", *count_id, *O_mutexing1);
        fflush(file);
        sem_post(print);
        fclose(file);
        exit(0);
    }

    //mutex is creating molecule now letting two atoms of H and one of O
    sem_wait(print);
    (*count_id)++;
    (*O_mutexing1)++;
    fprintf(file,"%d: O %d: creating molecule %d\n", *count_id, *O_mutexing1,*mutex_id);
    fflush(file);
    sem_post(print);

    //wait some time that represents creating molecule
    usleep(rand() % (params.Max_build_time + 1) * 1000);


    //wait for two hydrogens in process
    sem_wait(mutex2);

    sem_wait(print);
    (*count_id)++;
    (*O_mutexing2)++;
    fprintf(file,"%d: O %d: molecule %d created \n", *count_id, *O_mutexing2,*mutex_id); 
    fflush(file);
    //O part was created ... let hydrogens to finish
    sem_post(barier);
    sem_post(barier);
    sem_post(print);

    //wait for ultimate molecule finish
    sem_wait(barier1);

    sem_post(mutex);
}

// hydrogen
void hydrogen_f(){

    //process started
    sem_wait(print);
    (*count_id)++;
    (*H_id)++;
    fprintf(file,"%d: H %d: started\n", *count_id, *H_id);  
    fflush(file);
    sem_post(print);

    //wait some time representing time getting into queue
    usleep(rand() % (params.Max_queue_time + 1) * 1000);

    sem_wait(print);
    (*count_id)++;
    (*H_queued)++;
    fprintf(file,"%d: H %d: going to queue\n", *count_id, *H_queued); 
    fflush(file);
    sem_post(print);

    //kill child if there is not enough other atoms
    if(params.Oxy <= 0 || params.Hydro <= 1)
    {
        sem_wait(print);
        (*count_id)++;
        fprintf(file,"%d: H %d: not enough O or H\n", *count_id, *H_queued);        
        fflush(file);
        sem_post(print);
        fclose(file);
        exit(1);
    }

    //mutex place where atoms goes one by one
    sem_wait(mutex);

    (*H_mutexing)++;
 
     //if there is enough atoms to create molecule than let go two hydrogens and one oxygen
    if(*H_mutexing >= 2 && *O_mutexing >= 1)
    {
        (*mutex_id)++;
        sem_post(hydrogen);
        sem_post(hydrogen);
        *H_mutexing = *H_mutexing - 2;
        sem_post(oxygen);
        *O_mutexing = *O_mutexing - 1;
    }
    //if there is not enough atoms to create molecule let other atoms get into mutex place
    else
    {
        sem_post(mutex);
    }

    //queue of hydrogens
    sem_wait(hydrogen);

    //if there is not enough oxygens to create molecule kill childs
    if(*O_amount <= 0 || *H_amount <=1)
    {
        sem_wait(print);
        (*count_id)++;
        (*H_mutexing1)++;
        fprintf(file,"%d: H %d: not enough O or H\n", *count_id, *H_mutexing1);
        fflush(file);
        sem_post(print);

        fclose(file);
        exit(0);
    }
    

    //mutex is creating molecule now letting two atoms of H and one of O
    sem_wait(print);
    (*count_id)++;
    (*H_mutexing1)++;
    fprintf(file,"%d: H %d: creating molecule %d\n", *count_id, *H_mutexing1,*mutex_id);
    fflush(file);
    (*H_mutexing3)++;
    sem_post(print);

    //wait for two hydrogens in process
    if(*H_mutexing3 % 2 == 0)
    {
        sem_post(mutex2);
    }

    //wait for oxygen to finish creating molecule
    sem_wait(barier);

    sem_wait(print);
    (*count_id)++;
    (*mutex_id1)++;
    (*H_mutexing2)++;
    fprintf(file,"%d: H %d: molecule %d created\n", *count_id, *H_mutexing2,*mutex_id); 
    fflush(file);
    sem_post(print);

    //if last hydrogen finished molecule let oygen process ultimate finish moleule creation
    if(*mutex_id1 == 2)
    {
        *O_amount = *O_amount - 1;
        *H_amount = *H_amount - 2;
        *mutex_id1 = 0;

        sem_post(barier1);   
  
        // if there are not enugh atoms to create another molecule than kill all kids and finish process
        if(*O_amount <= 0 || *H_amount <=1)
        {
            for (int i = 0; i < *H_amount; i++)
            {
                sem_post(mutex);
                sem_post(hydrogen);
            }
            for (int i = 0; i < *O_amount; i++)
            {
                sem_post(mutex);
                sem_post(oxygen);
            }
        }  
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
        
        fprintf(stderr,"Fourth argument out of range [0-1000].\n");
        exit(1);
    }
    if(params->Max_queue_time > 1000 || params->Max_queue_time < 0)
    {
        fprintf(stderr,"Third argument out of range [0-1000].\n");
        exit(1);
    }
    

    return error;
}


int prepare_sources(){

    // semaphore inicialization
    if(
    (oxygen= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (hydrogen= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (print= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (mutex= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (mutex2= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (barier= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (barier1= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
    (sem_init(oxygen, 1, 0)) == -1 ||
    (sem_init(hydrogen, 1, 0)) == -1 ||
    (sem_init(print, 1, 1)) == -1 ||
    (sem_init(mutex, 1, 1)) == -1 ||
    (sem_init(mutex2, 1, 0)) == -1 ||
    (sem_init(barier, 1, 0)) == -1 ||
    (sem_init(barier1, 1, 0)) == -1)
    {
        fprintf(stderr,"Error has occured during inicializacion of semaphores.\n");
        clear_sources();
        exit(1);
    }

    // shared memmory
    if(
    (shm_O_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_queued_H = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_queued_O = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_O_amount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_amount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_mutex_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_mutex_id1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_mutexing = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_O_mutexing = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_mutexing1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_O_mutexing1 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_mutexing2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_O_mutexing2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_H_mutexing3 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||
    (shm_O_mutexing3 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1 ||


    (O_id = (int *) shmat(shm_O_id, NULL, 0)) == NULL ||
    (H_id = (int *) shmat(shm_H_id, NULL, 0)) == NULL ||
    (count_id = (int *) shmat(shm_count_id, NULL, 0)) == NULL ||
    (O_queued = (int *) shmat(shm_queued_O, NULL, 0)) == NULL ||
    (H_queued = (int *) shmat(shm_queued_H, NULL, 0)) == NULL ||
    (O_amount = (int *) shmat(shm_O_amount, NULL, 0)) == NULL ||
    (H_amount = (int *) shmat(shm_H_amount, NULL, 0)) == NULL ||
    (mutex_id = (int *) shmat(shm_mutex_id, NULL, 0)) == NULL ||
    (mutex_id1 = (int *) shmat(shm_mutex_id1, NULL, 0)) == NULL ||
    (H_mutexing = (int *) shmat(shm_H_mutexing, NULL, 0)) == NULL ||
    (O_mutexing = (int *) shmat(shm_O_mutexing, NULL, 0)) == NULL ||
    (H_mutexing1 = (int *) shmat(shm_H_mutexing1, NULL, 0)) == NULL ||
    (O_mutexing1 = (int *) shmat(shm_O_mutexing1, NULL, 0)) == NULL ||
    (H_mutexing2 = (int *) shmat(shm_H_mutexing2, NULL, 0)) == NULL ||
    (O_mutexing2 = (int *) shmat(shm_O_mutexing2, NULL, 0)) == NULL ||
    (H_mutexing3 = (int *) shmat(shm_H_mutexing3, NULL, 0)) == NULL ||
    (O_mutexing3 = (int *) shmat(shm_O_mutexing3, NULL, 0)) == NULL)
    {
        fprintf(stderr,"Error has occured during inicializacion of shared memmory.\n");
        clear_sources();
        exit(1);
    }
    
    return 0;
}

int clear_sources(){

    // semaphore celanup
    if(
    (sem_destroy(oxygen)) == -1 ||
    (sem_destroy(hydrogen)) == -1 ||
    (sem_destroy(print)) == -1 ||
    (sem_destroy(mutex)) == -1 ||
    (sem_destroy(mutex2)) == -1 ||
    (sem_destroy(barier)) == -1 ||
    (munmap(oxygen, sizeof(sem_t))) == -1 ||
    (munmap(hydrogen, sizeof(sem_t))) == -1 ||
    (munmap(print, sizeof(sem_t))) == -1 ||
    (munmap(mutex, sizeof(sem_t))) == -1 ||
    (munmap(mutex2, sizeof(sem_t))) == -1 ||
    (munmap(barier, sizeof(sem_t))) == -1 ||
    (munmap(barier1, sizeof(sem_t))) == -1 )
    {
        fprintf(stderr,"Error has occured during deleting semaphores.\n");
        clear_sources();
        exit(1);
    }

    // shared memmory cleanup
    if(
    (shmctl(shm_O_id, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_count_id, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_id, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_queued_O, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_queued_H, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_O_amount, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_amount, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_mutex_id, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_mutex_id1, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_mutexing, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_O_mutexing, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_mutexing1, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_O_mutexing1, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_mutexing2, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_O_mutexing2, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_H_mutexing3, IPC_RMID, NULL)) == -1 ||
    (shmctl(shm_O_mutexing3, IPC_RMID, NULL)) == -1 ||
    

    (shmdt(O_id)) == -1 ||
    (shmdt(O_queued)) == -1 ||
    (shmdt(H_id)) == -1 ||
    (shmdt(H_queued)) == -1 ||
    (shmdt(count_id)) == -1 ||
    (shmdt(O_amount)) == -1 ||
    (shmdt(H_amount)) == -1 ||
    (shmdt(mutex_id)) == -1 ||
    (shmdt(mutex_id1)) == -1 ||
    (shmdt(H_mutexing)) == -1 ||
    (shmdt(O_mutexing)) == -1 ||
    (shmdt(H_mutexing1)) == -1 ||
    (shmdt(O_mutexing1)) == -1 ||
    (shmdt(H_mutexing2)) == -1 ||
    (shmdt(O_mutexing2)) == -1 ||
    (shmdt(H_mutexing3)) == -1 ||
    (shmdt(O_mutexing3)) == -1)
    {
        fprintf(stderr,"Error has occured during deleting share memmory.\n");
        exit(1);
    }


    fclose(file);

    return 0;
}