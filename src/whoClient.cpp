#include <stdio.h>
#include <string.h>
#include <pthread.h> 
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "errExit.h"


using namespace std;

//
// Shared thread resources
//
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ready  = PTHREAD_COND_INITIALIZER;
static int gnumReady = 0;
static int gnumThreads = 0;


void getCommandLineArguements(int argc, 
    char *argv[], 
    string &queryFile,
    int &numThreads,
    int &servPort,
    string &servIP); 

void *thread_f(void *argp);

void readAndAssign(ifstream &squeryFile,
    int &qcount,
    int numThreads,
    pthread_t *tids);


int main(int argc, char *argv[])
{   
    // For various uses
    string s;
    
    string queryFile;
    int numThreads;
    int servPort;
    string servIP;
    
    // Read command line arguements
    getCommandLineArguements(argc, argv, queryFile, numThreads, servPort, servIP);
    
    gnumThreads = numThreads;
    
    //cout << "queryFile: " << queryFile
      //  << "\nnumThreads: " << numThreads
      //  << "\nservPort: " << servPort
      //  << "\nservIP: " << servIP 
      //  << endl;
    
    
    
    // Array for thread ids
    pthread_t *tids;
    tids = (pthread_t *)malloc(numThreads*sizeof(pthread_t));
    if (tids == NULL)
        errExit("malloc");
        
        
    
    // Open queryFile
    ifstream squeryFile;
    squeryFile.open(queryFile);
    if (!squeryFile.is_open())
        errExit("open");
        
        
    
    // Number of queries in queryFile
    int qcount;
    
    // Read queryFile and assign each line to a thread
    readAndAssign(squeryFile, qcount, numThreads, tids);
    
    
    
    // Join threads
    for (int i = 0; i < (numThreads < qcount ? numThreads : qcount); i++)
        if (pthread_join(tids[i], NULL))
            errExit("pthread_join");
    
    
    
    // Destroy mutex
    if (pthread_mutex_destroy(&mtx))
        errExit("pthread_mutex_destroy");
        
        
        
    // Destroy condition variable
    if (pthread_cond_destroy(&ready))
        errExit("pthread_cond_destroy");
    
    
    
    free(tids);
    
    
    
    return EXIT_SUCCESS;
} 




void readAndAssign(ifstream &squeryFile,
    int &qcount,
    int numThreads,
    pthread_t *tids)
{
    string s;
    int err;
    char *query;
    
    qcount = 0;
    
    // Read queryFile and assign to threads
    // while (squeryFile >> s)
    while (getline(squeryFile, s))
    {
        if (s.empty())
            continue;
        
        // Read up to numThreads queries
        qcount++;
        if (qcount > numThreads)
            continue;
        
        // Create dynamically allocated string for query
        query = (char *)malloc((s.length()+1)*sizeof(char));
        strcpy(query, s.c_str());
                
        // Assign to thread
        if (pthread_create(tids+qcount-1, NULL, thread_f, (void *)query))
            errExit("pthread_create");
            
        //cout << s << " <------> " << tids[qcount-1] << endl;
    }
}



void *thread_f(void *argp)
{
    char *query = (char *)argp;
    
    //int sleepTime = query[0] - '0';
    //printf("Thread %ld got %s and gonna sleep for %d seconds!\n", pthread_self(), query, sleepTime);
    //sleep(sleepTime);  
    //cout << "\nThread " << pthread_self() << " woke up, " << flush;



    //
    // Wait for all threads to get ready
    //
    
    // Lock mutex
    if (pthread_mutex_lock(&mtx))
        errExit("pthread_mutex_lock");
        
    //cout << "locked mutex, " << flush;
    
    // State that current thread is ready
    gnumReady++;
    
    // If all ready, broadcast
    if (gnumReady == gnumThreads)
    {   
        //cout << "broadcasts,";
        if (pthread_cond_broadcast(&ready))
            errExit("pthread_cond_broadcast");
    
    } 
    
    // If not all ready, wait on condition variable
    else
    {
        //cout << "waits on condition variable, " << flush;
        while (gnumReady != gnumThreads)
            if (pthread_cond_wait(&ready, &mtx))
                errExit("pthread_cond_wait");
    }
    
    //cout << "wakes up from cond_wait, " << flush;
    
    // Unlock mutex
    if (pthread_mutex_unlock(&mtx))
        errExit("pthread_mutex_unlock");
    
    //cout << "unlocks mutex and starts doing stuff!" << endl;
    
    // Do stuff
    
    free(query);
    pthread_exit(NULL);
}




void getCommandLineArguements(int argc, 
    char *argv[], 
    string &queryFile,
    int &numThreads,
    int &servPort,
    string &servIP)
{
    string usage("Usage: whoClient –q queryFile -w numThreads –sp servPort –sip servIP");
    
    if (argc != 9)
    {
        cout << usage << endl;
        exit(EXIT_FAILURE);
    }

    char q[] = "-q";
    char w[] = "-w";
    char sp[] = "-sp";
    char sip[] = "-sip";
        
    for (int i=1; i<9; i++)
    {
        if (strcmp(q, argv[i]) == 0)
        {
            i++;
            queryFile = argv[i];
        }
        
        else if (strcmp(w, argv[i]) == 0)
        {
            i++;
            numThreads = atoi(argv[i]);
        }
        
        else if (strcmp(sp, argv[i]) == 0)
        {
            i++;
            servPort = atoi(argv[i]);
        }
        
        else if (strcmp(sip, argv[i]) == 0)
        {
            i++;
            servIP = argv[i];
        }
        
        else
        {
            cout << usage << endl;
            exit(EXIT_FAILURE);
        }
    }
}

