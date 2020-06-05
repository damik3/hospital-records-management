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

// Shared thread resources


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
    
    
    
    // Create 
    
    
    // Array for thread ids
    pthread_t *tids;
    tids = (pthread_t *)malloc(numThreads*sizeof(pthread_t));
    if (tids == NULL)
        errExit("malloc");
        
        
    
    // Create threads
    for (int i=0; i<numThreads; i++)
        if (err = pthread_create(tids+i, NULL, thread_f, NULL))
            errExit("pthread_create");
    //
    
    
    
    // Join threads
    for (int i = 0; i < numThreads; i++)
        if (err = pthread_join(tids[i], NULL))
            errExit("pthread_join");
            
            
    
    free(tids);
    
    return EXIT_SUCCESS;
}




void *thread_f(void *argp){
    
    cout << "Thread " << pthread_self() << endl;
    
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

