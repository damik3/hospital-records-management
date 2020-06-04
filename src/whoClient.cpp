#include <stdio.h>
#include <string.h>
#include <pthread.h> 

#include <iostream>
#include <fstream>
#include <string>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


using namespace std;




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
    int err;
    
    string queryFile;
    int numThreads;
    int servPort;
    string servIP;
    
    // Read command line arguements
    getCommandLineArguements(argc, argv, queryFile, numThreads, servPort, servIP);
    
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
        if (err = pthread_join(tids[i], NULL))
            errExit("pthread_join");
    
    
    
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
        
        // Create dynamically allocated query string
        query = (char *)malloc((s.length()+1)*sizeof(char));
        strcpy(query, s.c_str());
        
        cout << query << " <------> ";
        
        // Assign to thread
        err = pthread_create(tids+qcount-1, NULL, thread_f, (void *)query);
        if (err)
            errExit("pthread_create");
            
        cout << tids[qcount-1] << endl;
    }
}



void *thread_f(void *argp)
{
    char *query = (char *)argp;
    printf("Thread %ld got %s!\n", pthread_self(), query);;
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

