/**
 * Example showing creq used with pthread for multithreading reques
 * compiled with: gcc -Wall -g  mt.c -o mt ../src/crequests.c  -I../include/ -lcurl -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <crequests.h>
#include <curl/curl.h>

void * worker_routine(void * ptr);

int main(int argc, char ** argv){
    curl_global_init(CURL_GLOBAL_ALL);
    // let's check the creq library in multithreading situation
    char url[] = "https://api.ipify.org";
    int ret;
    pthread_t * thread[10];
    for (int i=0; i< 10; ++i){
        thread[i] = (pthread_t*) malloc(sizeof(pthread_t));
        ret = pthread_create(thread[i], NULL,worker_routine, (void*)url);
        if (ret != 0){
            fprintf(stderr, "Can not create thread#%d\n", i);
            return 1;
        }
    }
    for (int i=0; i< 10; ++i){
        pthread_join(*thread[i], NULL);
        free(thread[i]);
    }
    fprintf(stdout, "Done getting the data\n");
    return 0;
}

void * worker_routine(void * ptr){
    char * url = (char*) ptr;
    CREQ_CTX * ctx = creq_init();
    if (!ctx){
        fprintf(stderr, "Error in thread#%ld Can not create creq contenxt\n", pthread_self());
        return NULL;
    }
    creq_get(ctx, url);
    fprintf(stdout, "Thread#%ld: %s\n", pthread_self(), creq_get_content(ctx));
    creq_close(ctx);
    return NULL;
}
