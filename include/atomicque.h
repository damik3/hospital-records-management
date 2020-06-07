#ifndef ATOMICQUE_H
#define ATOMICQUE_H

#include <stdlib.h>

#include <iostream>

#include "errExit.h"

using namespace std;



template <class T>
struct atomicque
{
    T *arr;    // Array of items
    int size;   // Size of array
    int count;  // Current items in array
    int start;  // Start of array
    int end;    // End of array
    pthread_mutex_t mtx;
    pthread_cond_t nonFull;
    pthread_cond_t nonEmpty;
    
    atomicque(int poolSize);
    ~atomicque();
    bool empty();               // Non atomic
    bool full();                // Non atomic
    void enq(const T& item);    // Atomic
    T deq();                    // Atomic
    void print();               // Non atomic
};



template <class T>
atomicque<T>::atomicque(int poolSize)
{
    arr = (T *)malloc(poolSize*sizeof(T));
    if (arr == NULL)
        errExit("malloc");
        
    size = poolSize;
    count = 0;
    start = 0;
    end = 0;
    pthread_mutex_init (&mtx, 0);
    pthread_cond_init(&nonEmpty, 0);
    pthread_cond_init(&nonFull, 0);
}



template <class T>
atomicque<T>::~atomicque()
{
    free(arr);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&nonEmpty);
    pthread_cond_destroy(&nonFull);
}



template <class T>
bool atomicque<T>::empty()
{
    return count == 0;
}



template <class T>
bool atomicque<T>::full()
{
    return count == size;
}



template <class T>
void atomicque<T>::enq(const T& item)
{
    // Lock mutex
    if (pthread_mutex_lock(&mtx))
        errExit("pthread_mutex_lock");
    
    // If full, wait
    while (full())
        if (pthread_cond_wait(&nonFull, &mtx))
            errExit("pthread_cond_wait");
            
    // When not full, enqueue
    arr[end] = item;
    end = (end + 1) % size;
    count++;
    
    // Signal
    pthread_cond_signal(&nonEmpty);
    
    // Unlock mutex
    if (pthread_mutex_unlock(&mtx))
        errExit("pthread_mutex_unlock");
}



template <class T>
T atomicque<T>::deq()
{
    // Lock mutex
    if (pthread_mutex_lock(&mtx))
        errExit("pthread_mutex_lock");
    
    // If empty, wait
    while (empty())
        if (pthread_cond_wait(&nonEmpty, &mtx))
            errExit("pthread_cond_wait");
    
    // When not empty, dequeue
    T ret = arr[start];
    start = (start + 1) % size;
    count--;
    
    // Signal
    pthread_cond_signal(&nonFull);
    
    // Unlock mutex
    if (pthread_mutex_unlock(&mtx))
        errExit("pthread_mutex_unlock");
    
    return ret; 
}



template <class T>
void atomicque<T>::print()
{
    int curr = start;
    int tmpcount = count;
    while (tmpcount--)
    {
        cout << arr[curr] << endl;
        curr = (curr + 1) % size;
    }
}



#endif  // ATOMICQUE_H