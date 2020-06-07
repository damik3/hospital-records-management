#include <netinet/in.h>	     
#include <netdb.h> 
#include <pthread.h> 
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>	     
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "errExit.h"
#include "atomicque.h"


using namespace std;

//
// Shared thread resources
//

atomicque<int> *pool;        // Pool for file descriptors



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
    pool = new atomicque<int>(bufferSize);
    
    // Array for thread ids
    pthread_t *tids;
    tids = (pthread_t *)malloc(numThreads*sizeof(pthread_t));
    if (tids == NULL)
        errExit("malloc");


    
    // Create threads
    for (int i=0; i<numThreads; i++)
        if (pthread_create(tids+i, NULL, thread_f, NULL))
            errExit("pthread_create");
    
    
/*
    // Read fds from cin
    int input;
    while (cin >> input)
        pool->enq(input);
    pool->print();
  */
  

    //
    // Set up sockets
    //
    
    int sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen;
    struct hostent *rem;
    
    // Create socket for whoClient
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errExit("socket");
        
    // Avoid TIME_WAIT after closing socket
    int yes=1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        errExit("setsockopt");
        
    // Bind socket
    server.sin_family = AF_INET;       
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(queryPortNum);      
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        errExit("bind");
        
    // Listen
    if (listen(sock, 5) < 0) 
        errExit("listen");
        
    printf("Listening for connections to port %d\n", queryPortNum);
    //
    
    
    
    // Signal it's over
    int x=-1;
    for (int i=0; i<numThreads; i++)
        pool->enq(x);
        
    
    
    // Join threads
    for (int i = 0; i < numThreads; i++)
        if (pthread_join(tids[i], NULL))
            errExit("pthread_join");
    

        
    cout << "About to exit, with pool looking like dis" << endl;
    pool->print();
    
    close(sock);
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
        if (pthread_mutex_lock(&(pool->mtx)))
            errExit("pthread_mutex_lock");
            
        cout << tid << " dequeued " << fd << endl;
        
        // Unlock print mutex
        if (pthread_mutex_unlock(&(pool->mtx)))
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

