/** * @file   tm.c
 * @author [...]
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers

// Internal headers
#include <tm.hpp>
#include <mutex>
#include <string>
#include <memory>
#include <cstdlib>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <vector>
#include <map>
#include <thread>
#include <iostream>

using namespace std;

using align_t = uint64_t;
//const size_t NUM_TX = 100000;

struct txx;

struct txinfo {
    shared_t shared;
    size_t id;
};


void
run_thread(struct txinfo info){
    shared_t shared = info.shared;
    size_t id = info.id;
    tx_t txs[50000];
    //std:://cout << "start\n";
    
    for (size_t i = 0; i < 50000; ++i){
        do {
            bool success = false;
            bool is_ro = rand() % 2;
            uint64_t p_first;
            uint64_t p_second;
            int p_diff = 0;

            txs[i] = tm_begin(shared, is_ro);
            //std:://cout << i << std::endl;

            if (is_ro){
                void *s_first = tm_start(shared);
                void *s_second = (char *) s_first + 8*sizeof(char);

                //std:://cout << "p_first:" << p_first << " p_second:" << p_second << std::endl;

                bool succeeded = tm_read(shared, txs[i], s_first, 8, &p_first);
                if(!succeeded){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_first << "\n";
                }

                succeeded = tm_read(shared, txs[i], s_second, 8, &p_second);
                if(!succeeded){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_second << "\n";
                }

                succeeded = tm_end(shared, txs[i]);
                if (succeeded){
                    break;
                }

                p_diff = p_first - p_second;
                if(p_diff != 0){
                    int j = 0;
                }
            } else {
                void *s_first = tm_start(shared);
                void *s_second = (char *) s_first + 8*sizeof(char);

                bool succeeded = tm_read(shared, txs[i], s_first, 8, &p_first);
                if(!succeeded){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_first << "\n";
                }

                succeeded = tm_read(shared, txs[i], s_second, 8, &p_second);
                if(!succeeded){
                    delete (struct txx * ) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_second << "\n";
                }

                p_first += 4;
                p_second += 8;

                if(!tm_write(shared, txs[i], &p_first, 8, s_first)){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "w " << p_first << "\n";
                }

                if(!tm_write(shared, txs[i], &p_second, 8, s_second)){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "w " << p_second << "\n";
                }

                if(!tm_read(shared, txs[i], s_first, 8, &p_first)){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_first << "\n";
                }

                if(!tm_read(shared, txs[i], s_second, 8, &p_second)){
                    delete (struct txx *) txs[i];
                    continue;
                } else {
                    //cout << "r " << p_second << "\n";
                }

                succeeded = tm_end(shared, txs[i]);
                if(succeeded){
                    break;
                }
            }
        } while(1);
    } 
}


int main(){
    //std:://cout << "Running\n";
    volatile shared_t region = tm_create(16, 8);
    struct txinfo info1 = {region, 1};
    struct txinfo info2 = {region, 2};

    std::thread t1(run_thread, info1);
    std::thread t2(run_thread, info2);

    t1.join();
    t2.join();

    tm_destroy(region);

    return 0;
}


