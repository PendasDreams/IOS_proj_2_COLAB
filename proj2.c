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


int main(){
    printf("hello world\n");
    return 0;
}