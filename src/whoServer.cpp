#include <errno.h>
#include <netinet/in.h>	     
#include <netdb.h> 
#include <poll.h>
#include <pthread.h> 
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>	     
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "atomicque.h"
#include "errExit.h"
#include "myLowLvlIO.h"
#include "myPairs.h"

using namespace std;

static int flgsigint = 0;

//
// Shared thread resources
//

// Pool for file descriptors
static atomicque<pair<int, char> > *pool;

// Mutex for printing        
static pthread_mutex_t print = PTHREAD_MUTEX_INITIALIZER;

// Number of workers
static int numWorkers;

// Number of workeres done sending summary statistics
static int workersDone;




void getCommandLineArguements(int argc, 
    char *argv[], 
    int &queryPortNum,
    int &statsPortNum,
    int &numThreads,
    int &bufferSize);

void *thread_f(void *argp);

void sigHandler(int signo);



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
    
    
    
    // Set signal handler
    static struct sigaction act;
    sigfillset(&(act.sa_mask));
    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    
    

    //
    // Set up sockets
    //
    
    int querySock;
    int statsSock;
    struct sockaddr_in server;
    
    // Create socket for whoClient
    if ((querySock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errExit("socket");
        
    // Create socket for workers
    if ((statsSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errExit("socket");
        
    // Avoid TIME_WAIT after closing querySock
    int yes=1;
    if (setsockopt(querySock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        errExit("setsockopt");
        
    // Avoid TIME_WAIT after closing statsSock
    if (setsockopt(statsSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        errExit("setsockopt");
        
    // Bind querySock
    server.sin_family = AF_INET;       
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(queryPortNum);      
    if (bind(querySock, (struct sockaddr *)&server, sizeof(server)) < 0)
        errExit("bind");
        
    // Bind statsSock
    server.sin_port = htons(statsPortNum);  
    if (bind(statsSock, (struct sockaddr *)&server, sizeof(server)) < 0)
        errExit("bind");
     
    // Listen
    if (listen(querySock, 5) < 0) 
        errExit("listen");
        
    if (listen(statsSock, 5) < 0) 
        errExit("listen");
        
    printf("Listening for connections on ports %d and %d\n", queryPortNum, statsPortNum);
    
    
    
    //
    // Read numWorkers from master
    //
    
    int newsock;
    
    if ((newsock = accept(statsSock, NULL, NULL)) < 0) 
        errExit("accept");
        
    if (read_data(newsock, (char *)&numWorkers, sizeof(int), sizeof(int)) < 0)
        errExit("read_data");
        
    cout << "numWorkers = " << numWorkers << endl;
    
    
    
    //
    // Create poll struct
    //
    
    struct pollfd *pfds;
    pfds = (struct pollfd *)calloc(2, sizeof(struct pollfd));
    if (pfds == NULL)
    {
        perror("malloc");
        return EXIT_FAILURE;
    }
    
    pfds[0].fd = querySock;
    pfds[0].events = POLLIN;
    pfds[1].fd = statsSock;
    pfds[1].events = POLLIN;
    
    
    
    //
    // Create threads
    //
    
    // Create pool for fds
    pool = new atomicque<pair<int, char> >(bufferSize);
    
    // Array for thread ids
    pthread_t *tids;
    tids = (pthread_t *)malloc(numThreads*sizeof(pthread_t));
    if (tids == NULL)
        errExit("malloc");


    
    // Create threads
    for (int i=0; i<numThreads; i++)
        if (pthread_create(tids+i, NULL, thread_f, NULL))
            errExit("pthread_create");
    
    
    
    //
    // Enter server mode
    //
    pair<int, char> p;
    
    while (1)
    {
        // Wait for new connection to arrive
        if (poll(pfds, 2, -1) == -1)
            if (errno != EINTR)
                errExit("poll");
            
        // Exit on SIGINT
        if (flgsigint)
            break;
        
        
        
        // If connection came in querySock
        if ((pfds[0].revents != 0) && (pfds[0].revents & POLLIN))
        {
            // Accept new connection
            if ((newsock = accept(querySock, NULL, NULL)) < 0) 
                if (errno != EINTR)
                    errExit("accept");
                    
            p.first = newsock;
            p.second = 'q';
            pool->enq(p);
        }
        
        
        
        // If connection came in querySock
        else if ((pfds[1].revents != 0) && (pfds[1].revents & POLLIN))
        {
            // Accept new connection
            if ((newsock = accept(statsSock, NULL, NULL)) < 0) 
                if (errno != EINTR)
                    errExit("accept");
            
            p.first = newsock;
            p.second = 's';
            pool->enq(p);
        }
        
        
        
        // Not supposed to happen
        else
            errExit("whoServer: something unexpected happened!");
    }
    
    
    
    // Signal it's over
    p.first = -1;
    for (int i=0; i<numThreads; i++)
        pool->enq(p);
        
    
    
    // Join threads
    for (int i = 0; i < numThreads; i++)
        if (pthread_join(tids[i], NULL))
            errExit("pthread_join");
    
    
    
    // Destroy mutex
    if (pthread_mutex_destroy(&print))
        errExit("pthread_mutex_destroy");
    
    
    
    cout << "Exiting with pool lookin like dis" << endl;
    pool->print();
    
    close(querySock);
    close(statsSock);
    delete pool;
    free(tids);
    
    return EXIT_SUCCESS;
}




void *thread_f(void *argp){
    
    //cout << "Thread " << pthread_self() << endl;
    //pthread_t tid = pthread_self();
    
    // Maximum query length = 128
    char buff[128];
    
    pair<int, char> p;
    
    
    
    //
    // Wait to get a file descriptor (-1 means exit)
    //
    
    while ((p = pool->deq()).first != -1)
    {    
        cout << "Just deq'd " << p << endl;
        
        
        // If read from queryPort
        if (p.second == 'q'){

            // Read query
            if (read(p.first, buff, 128) == -1)
                errExit("read");
                
            // Just to be safe
            buff[128] = '\0'; 
            
            
            
            // Lock mutex
            if (pthread_mutex_lock(&print))
                errExit("pthread_mutex_lock");
            
            cout << "Received " << buff << endl;
            
            // Unlock mutex
            if (pthread_mutex_unlock(&print))
                errExit("pthread_mutex_unlock");
                
                
            
            //
            // TODO: Process query
            //
            
            
            
            // Send answer back
            strcpy(buff, "okay");
            if (write(p.first, buff, (strlen(buff)+1)*sizeof(char)) == -1)
                errExit("write");
                 
        }
        
        else if (p.second == 's')
        {
            
        }
        
        close(p.first);
    }
   
    pthread_exit(NULL);
} 




void sigHandler(int signo)
{
    if ((signo == SIGINT) || (signo == SIGQUIT))
        flgsigint = 1;
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

