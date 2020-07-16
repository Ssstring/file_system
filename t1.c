#include<stdio.h>
#include<sys/wait.h>
#include<unistd.h>
#include<string.h>
#include<semaphore.h>
int main(){
    char tt[20];
    int t = 1;
    sem_t mutex;
    // sem_init(&mutex, 0, 1);

    while (t)
    {
        t = scanf("%12s", tt);
        printf("%d\n", strlen(tt));
        printf("%d\n%s\n", t, tt);
    }
    printf("%d\n", sizeof(sem_t));
    
    return 0;
}
