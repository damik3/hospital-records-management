#include <stdio.h>
#include <string.h>
#include <pthread.h> 
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


using namespace std;

static int err;

//
// Shared thread resources
//

// Pool for file descriptors
struct fdPool
{
    int *fd;    // Array of fds
    int size;   // Size of array
    int count;  // Current fds in array
    int start;  // Start of array
    int end;    // End of array
    pthread_mutex_t mtx;
    pthread_mutex_t printmtx;
    pthread_cond_t nonFull;
    pthread_cond_t nonEmpty;
    
    fdPool(int poolSize)
    {
        fd = (int *)malloc(poolSize*sizeof(int));
        if (fd == NULL)
            errExit("malloc");
            
        size = poolSize;
        count = 0;
        start = 0;
        end = 0;
        pthread_mutex_init (&mtx, 0);
        pthread_mutex_init (&printmtx, 0);
        pthread_cond_init(&nonEmpty, 0);
        pthread_cond_init(&nonFull, 0);
    }
    
    ~fdPool()
    {
        free(fd);
        pthread_mutex_destroy(&mtx);
        pthread_mutex_destroy(&printmtx);
        pthread_cond_destroy(&nonEmpty);
        pthread_cond_destroy(&nonFull);
    }
    
    // Non atomic
    bool empty()
    {
        return count == 0;
    }
    
    // Non atomic
    bool full()
    {
        return count == size;
    }
    
    // Atomic, waits on nonFull
    void enq(const int& pfd)
    {
        // Lock mutex
        if (err = pthread_mutex_lock(&mtx))
            errExit("pthread_mutex_lock");
        
        // If full, wait
        while (full())
            if (err = pthread_cond_wait(&nonFull, &mtx))
                errExit("pthread_cond_wait");
                
        // When not full, enqueue
        fd[end] = pfd;
        end = (end + 1) % size;
        count++;
        
        // Signal
        pthread_cond_signal(&nonEmpty);
        
        // Unlock mutex
        if (err = pthread_mutex_unlock(&mtx))
            errExit("pthread_mutex_unlock");
    }
    
    // Atomic, waits on nonEmpty
    int deq()
    {
        // Lock mutex
        if (err = pthread_mutex_lock(&mtx))
            errExit("pthread_mutex_lock");
        
        // If empty, wait
        while (empty())
            if (err = pthread_cond_wait(&nonEmpty, &mtx))
                errExit("pthread_cond_wait");
        
        // When not empty, dequeue
        int pfd = fd[start];
        start = (start + 1) % size;
        count--;
        
        // Signal
        pthread_cond_signal(&nonFull);
        
        // Unlock mutex
        if (err = pthread_mutex_unlock(&mtx))
            errExit("pthread_mutex_unlock");
        
        return pfd; 
    }
    
    // Non atomic
    void print()
    {
        int curr = start;
        int tmpcount = count;
        while (tmpcount--)
        {
            cout << fd[curr] << endl;
            curr = (curr + 1) % size;
        }
    }
} *pool;



void getCommandLineArguements(int argc, 
    char *argv[], 
    int &queryPortNum,
    int &statsPortNum,
    int &numThreads,
    int &bufferSize);

void *thread_f(void *argp);



int main(int argc, char* argv[])
{    
    int queryPortNum;
    int statsPortNum;
    int numThreads;
    int bufferSize;
    
    getCommandLineArguements(argc, argv, queryPortNum, statsPortNum, numThreads, bufferSize);
    
    //cout << "queryPortNum = " << queryPortNum << endl;
    //cout << "statsPortNum = " << statsPortNum << endl;
    //cout << "numThreads = " << numThreads << endl;
    //cout << "bufferSize = " << bufferSize << endl;
    
    
    
    // Create pool for fds
    pool = new fdPool(bufferSize);
    
    // Array for thread ids
    pthread_t *tids;
    tids = (pthread_t *)malloc(numThreads*sizeof(pthread_t));
    if (tids == NULL)
        errExit("malloc");
    
    

/*
    for (int i=0; i<5; i++)
        pool->enq(i);
    pool->print();

    for (int i=0; i<5; i++)
        cout << "Dequeued " << pool->deq() << endl;
    pool->print();
        
    for (int i=7; i<11; i++)
        pool->enq(i);  
    pool->print();
*/

    
    // Create threads
    for (int i=0; i<numThreads; i++)
        if (err = pthread_create(tids+i, NULL, thread_f, NULL))
            errExit("pthread_create");
    
    
    
    // Read fds from cin
    int input;
    while (cin >> input)
        pool->enq(input);
    pool->print();
    
    
    
    // Signal it's over
    int x=-1;
    for (int i=0; i<numThreads; i++)
        pool->enq(x);
    //
    
    
    
    // Join threads
    for (int i = 0; i < numThreads; i++)
        if (err = pthread_join(tids[i], NULL))
            errExit("pthread_join");
    

        
    cout << "About to exit, with pool looking like dis" << endl;
    pool->print();
    
    delete pool;
    free(tids);
    
    return EXIT_SUCCESS;
}




void *thread_f(void *argp){
    
    //cout << "Thread " << pthread_self() << endl;
    pthread_t tid = pthread_self();
    
    int fd;
    while ((fd = pool->deq()) != -1)
    {
        // Lock print mutex
        if (err = pthread_mutex_lock(&(pool->printmtx)))
            errExit("pthread_mutex_lock");
            
        cout << tid << " dequeued " << fd << endl;
        
        // Unlock print mutex
        if (err = pthread_mutex_unlock(&(pool->printmtx)))
            errExit("pthread_mutex_unlock");
    }   
    pthread_exit(NULL);
} 




void getCommandLineArguements(int argc, 
    char *argv[], 
    int &queryPortNum,
    int &statsPortNum,
    int &numThreads,
    int &bufferSize)
{
    string usage("Usage: whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize");
    
    if (argc != 9)
    {
        cout << usage << endl;
        exit(EXIT_FAILURE);
    }

    char q[] = "-q";
    char s[] = "-s";
    char w[] = "-w";
    char b[] = "-b";
        
    for (int i=1; i<9; i++)
    {
        if (strcmp(q, argv[i]) == 0)
        {
            i++;
            queryPortNum = atoi(argv[i]);
        }
        
        
        else if (strcmp(s, argv[i]) == 0)
        {
            i++;
            statsPortNum = atoi(argv[i]);
        }
        
        else if (strcmp(w, argv[i]) == 0)
        {
            i++;
            numThreads = atoi(argv[i]);
        }
        
        else if (strcmp(b, argv[i]) == 0)
        {
            i++;
            bufferSize = atoi(argv[i]);
        }
        
        else
        {
            cout << usage << endl;
            exit(EXIT_FAILURE);
        }
    }
}

