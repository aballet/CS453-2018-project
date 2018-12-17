#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "tm.h"
#include <pthread.h>
#include <unistd.h>

shared_t tm;

void* f(void* thr_data)
{
    bool did_commit = false;
    while(!did_commit) {
        tx_t tx = tm_begin(tm, false);
        void* region = tm_start(tm);
        void* tmp_region = malloc(sizeof(void*));

        double to_write = 10;
        bool can_continue_after_write = tm_write(tm, tx, &to_write, sizeof(double), region);
        printf("Can continue after write? %d\n", can_continue_after_write);
        if (!can_continue_after_write) {
            break;
        }

        to_write = 20;
        can_continue_after_write = tm_write(tm, tx, &to_write, sizeof(double), region+1* sizeof(double));
        printf("Can continue after write? %d\n", can_continue_after_write);
        if (!can_continue_after_write) {
            break;
        }

        to_write = 30;
        can_continue_after_write = tm_write(tm, tx, &to_write, sizeof(double), region+2* sizeof(double));
        printf("Can continue after write? %d\n", can_continue_after_write);
        if (!can_continue_after_write) {
            break;
        }

        bool can_continue_after_read = tm_read(tm, tx, region+1* sizeof(double), sizeof(double), tmp_region);
        printf("Can continue after read? %d\n", can_continue_after_read);
        printf("Value read = %f\n", *(double*)tmp_region);

        to_write = 40;
        can_continue_after_write = tm_write(tm, tx, &to_write, sizeof(double), region+3* sizeof(double));
        printf("Can continue after write? %d\n", can_continue_after_write);
        if (!can_continue_after_write) {
            break;
        }

        can_continue_after_read = tm_read(tm, tx, region+1* sizeof(double), sizeof(double), tmp_region);
        printf("Can continue after read? %d\n", can_continue_after_read);
        printf("Value read = %f\n", *(double*)tmp_region);

        did_commit = tm_end(tm, tx);
        printf("Committed? %d\n", did_commit);




        tx = tm_begin(tm, false);

        can_continue_after_read = tm_read(tm, tx, region+1* sizeof(double), sizeof(double), tmp_region);
        printf("Can continue after read? %d\n", can_continue_after_read);
        printf("Value read = %f\n", *(double*)tmp_region);

        to_write = 30;
        can_continue_after_write = tm_write(tm, tx, &to_write, sizeof(double), region+1* sizeof(double));
        printf("Can continue after write? %d\n", can_continue_after_write);
        if (!can_continue_after_write) {
            break;
        }

        can_continue_after_read = tm_read(tm, tx, region+1* sizeof(double), sizeof(double), tmp_region);
        printf("Can continue after read? %d\n", can_continue_after_read);
        printf("Value read = %f\n", *(double*)tmp_region);

        did_commit = tm_end(tm, tx);
        printf("Committed? %d\n", did_commit);
    }
    return NULL;
}

int main(int argc, char const *argv[]) {
    tm = tm_create(4*sizeof(double), sizeof(double));

    pthread_t thr[100];
    for(int n = 0; n < 100; ++n) {
        //sleep(1);
        printf("\n\n\n");
        pthread_create(&thr[n], NULL, f, NULL);
    }

    int i = 0;
    for(int n = 0; n < 100; ++n) {
        pthread_join(thr[n], NULL);
        i++;
        printf("Finished : %d\n", i);
    }

    printf("DONE\n");
    tm_destroy(tm);
}
