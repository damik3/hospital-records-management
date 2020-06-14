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
#include <sstream>

#include "ageGroup.h"
#include "atomicque.h"
#include "errExit.h"
#include "sendReceive.h"
#include "myLowLvlIO.h"
#include "mySTL/myHashTable.h"
#include "mySTL/myAVLTree.h"
#include "myPairs.h"

using namespace std;

static int flgsigint = 0;
static int bufferSize = 128;

//
// Shared thread resources
//

// Pool for file descriptors
static atomicque<pair<int, char> > *pool;

// Mutex for printing        
static pthread_mutex_t print = PTHREAD_MUTEX_INITIALIZER;

// Number of workers
static int numWorkers;

// Sockets for communication with workers
static int* workerSock;
static int iworkerSock = 0;
static pthread_mutex_t mtxworkerSock = PTHREAD_MUTEX_INITIALIZER;

// Set for countries
myAVLTree<string> countries;

// 2d associative array (rows are diseases, columns are countries).
// [d][c] element is a tree sorted by myDate containing ageGroups.
// 1 struct for patients(records) that entered and 1 for those that exited
static myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > > rec_enter;
static myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > > rec_exit;
static pthread_mutex_t recmtx = PTHREAD_MUTEX_INITIALIZER;




void getCommandLineArguements(int argc, 
    char *argv[], 
    int &queryPortNum,
    int &statsPortNum,
    int &numThreads,
    int &bufferSize);

void *thread_f(void *argp);

void sigHandler(int signo);

string procQuery(const char *q);



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
        
    //cout << "numWorkers = " << numWorkers << endl;
    
    workerSock = (int *)malloc(numWorkers*sizeof(int));
    for (int i=0; i<numWorkers; i++)
        workerSock[i] = -1;
    
    
    
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
        
        
        
        // If connection came in statsSock
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
    
    
    
    //cout << "/n/n***rec_enter***" << endl << rec_enter << endl;
    //cout << "/n/n***rec_exit***" << endl << rec_exit << endl;
    
    
    
    // Signal it's over
    p.first = -1;
    for (int i=0; i<numThreads; i++)
        pool->enq(p);
        
    
    
    // Join threads
    for (int i = 0; i < numThreads; i++)
        if (pthread_join(tids[i], NULL))
            errExit("pthread_join");
    
    
    
    // Destroy mutexes
    if (pthread_mutex_destroy(&print))
        errExit("pthread_mutex_destroy");
    
    if (pthread_mutex_destroy(&recmtx))
        errExit("pthread_mutex_destroy");
    
    if (pthread_mutex_destroy(&mtxworkerSock))
        errExit("pthread_mutex_destroy");
    
    
    //cout << "Exiting with pool lookin like dis" << endl;
    //pool->print();
    
    close(querySock);
    close(statsSock);
    delete pool;
    free(workerSock);
    free(tids);
    
    return EXIT_SUCCESS;
}




string procQuery(const char *q)
{
    string result = "";
    
    string s(q);
    stringstream ss(s);
    
    string command;
    ss >> command;
    
    
    
    if (command == "/diseaseFrequency")
    {
        string disease, country;
        myDate d1, d2;
        
        ss >> disease >> d1 >> d2;
        
        string line;
        getline(ss, line);
        
        // If the country arguement exists, read it 
        if (!line.empty()) 
        {
            stringstream sline;
            sline.str(line);
            sline >> country;
        }
                    
        int count;
        
        if (country.empty())
        {
            count = 0;
            myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > >::iterator it1;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it2;
            
            for (it1 = rec_enter[disease].begin(); it1.isValid(); ++it1)
                for (it2 = it1->second.begin(); it2.isValid(); ++it2)
                    if ((d1 < it2->first) && (it2->first < d2))
                        count += it2->second.getTotal();
                        
            result += to_string(count);
            result += "\n";
        }
        
        else
        {
            count = 0;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
            
            if (countries.exists(country))
                for (it = rec_enter[disease][country].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        count += it->second.getTotal();
                        
            result += to_string(count);
            result += "\n";
        }
    
    }   // End if (command == "/diseaseFrequency")
    
    
    
    else if (command == "/topk-AgeRanges")
    {
        int k;
        string country;
        string disease;
        myDate d1;
        myDate d2;
        
        ss >> k >> country >> disease >> d1 >> d2;
        
        ageGroup g;
        myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
    
        if (countries.exists(country))
            for (it = rec_enter[disease][country].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        g = g + it->second;
             
        stringstream ssout;
        g.printTopk(ssout, k);
        result = ssout.str();
    
    }   // End else if (command == "/topk-AgeRanges")
    
    
    
    else if (command == "/searchPatientRecord")
    {
        bool found = false;
        patient pat;
        patient patfound;
        ssize_t ret;
        
        // Read patient id
        ss >> pat.id;
        
        
        
        // Send it to workers
        for (int i=0; i<iworkerSock; i++)
            if (send_pat(workerSock[i], bufferSize, pat) == -1)
                errExit("send_pat");
        
        
        // Wait for their response
        for (int i=0; i<iworkerSock; i++)
        {
            // Listen for i-th worker's response
            ret = receive_pat(workerSock[i], bufferSize, pat);
            if (ret == -1)
                errExit("receive_pat");
            
            // If patient was found
            if (ret > sizeof(char))
            {
                found = true;
                patfound = pat;
            }
        }
        
        
        
        // Write into result
        if (found)
        {
            result += patfound.str();
            result += "\n";
        }
        else
            result += "Patient not found!\n";
    }   // End if (command == "/searchPatientRecord")
    
    
    
    else if (command == "/numPatientAdmissions")
    {
        string disease, country;
        myDate d1, d2;
        
        ss >> disease >> d1 >> d2;
        
        string line;
        getline(ss, line);
        
        // If the country arguement exists, read it 
        if (!line.empty()) 
        {
            stringstream sline;
            sline.str(line);
            sline >> country;
        }
             

   
        int count;
        
        if (country.empty())
        { 
            myAVLTree<string>::iterator itCountries;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
            
            for (itCountries = countries.begin(); itCountries.isValid(); ++itCountries)
            {
                count = 0;
                
                for (it = rec_enter[disease][*itCountries].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        count += it->second.getTotal();
                        
                result += *itCountries;
                result += " ";
                result += to_string(count);
                result += "\n";
            }
        }
        
        else
        {
            count = 0;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
            
            if (countries.exists(country))
                for (it = rec_enter[disease][country].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        count += it->second.getTotal();
            
            result += to_string(count);
            result += "\n";
        }
    }
    
    
    
    else if (command == "/numPatientDischarges")
    {
        string disease, country;
        myDate d1, d2;
        
        ss >> disease >> d1 >> d2;
        
        string line;
        getline(ss, line);
        
        // If the country arguement exists, read it 
        if (!line.empty()) 
        {
            stringstream sline;
            sline.str(line);
            sline >> country;
        }
              

  
        int count;
        
        if (country.empty())
        {
            myAVLTree<string>::iterator itCountries;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
            
            for (itCountries = countries.begin(); itCountries.isValid(); ++itCountries)
            {
                count = 0;
                
                for (it = rec_exit[disease][*itCountries].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        count += it->second.getTotal();
                
                result += *itCountries;
                result += " ";
                result += to_string(count);
                result += "\n";
            }
        }
        
        else
        {
            count = 0;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
            
            if (countries.exists(country))
                for (it = rec_exit[disease][country].begin(); it.isValid(); ++it)
                    if ((d1 < it->first) && (it->first < d2))
                        count += it->second.getTotal();
            
            result += to_string(count);
            result += "\n";
        }
    }
    
    
    
    // Wrong query
    else
        result = "You have an error in your query syntax.\n";
    
    return result;
}




void *thread_f(void *argp)
{    
    // Maximum query length = 128
    char buff[128];
    
    
    
    //
    // Wait to get a file descriptor (-1 means exit)
    //
    
    pair<int, char> p;
    
    while ((p = pool->deq()).first != -1)
    {    
        //cout << "Just deq'd " << p << endl;
        
        
        //
        // If read from queryPort
        //
        if (p.second == 'q'){

            // Read query
            if (read_data(p.first, buff, 128, bufferSize) == -1)
                errExit("read");
                
            // Just to be safe
            buff[127] = '\0'; 
            
            
            
            // Lock mutex
            if (pthread_mutex_lock(&recmtx))
                errExit("pthread_mutex_lock");
            
            // Process query
            string result = procQuery(buff);
            
            cout << buff << endl << result << endl;;
            
            // Unock mutex
            if (pthread_mutex_unlock(&recmtx))
                errExit("pthread_mutex_unlock");
               
               
               
            // Send answer back
            strncpy(buff, result.c_str(), 127);
            buff[127] = '\0';
            if (write_data(p.first, buff, (strlen(buff)+1)*sizeof(char), bufferSize) == -1)
                errExit("write");
                
            close(p.first);
        
        }   // End if read from queryPort
        
        
        //
        // If read from statsPort
        //
        else if (p.second == 's')
        {
            
            int ret;
            string s;
            string country;
            string disease;
            myDate d;
            ageGroup group;
            indexPair<myDate, ageGroup> ipair;
            
            
            
            //
            // Read all statistics sent from worker
            //
            do {
                
                ret = receive_data(p.first, bufferSize, d, s, group, country, disease);
                
                // In case of error
                if (ret == -1)
                    errExit("receive_data");
                    
                // In case worker is done
                else if (ret == sizeof(char))
                    break;
                  
                // In case worker sent data
                else
                {
                    //cout << "\nwhoServer received " 
                      //      << country << endl
                      //      << disease << endl
                      //      << d << endl
                      //      << s << endl
                      //      << group;
                            
                    ipair.first = d;
                    ipair.second = group;
                    
                    // Lock mutex
                    if (pthread_mutex_lock(&recmtx))
                        errExit("pthread_mutex_lock");
                    
                    // Insert into appropriate data structure
                    if (s == "EN")
                        rec_enter[disease][country].insert(ipair);
                    else if (s == "EX")
                        rec_exit[disease][country].insert(ipair);
                    else
                        cerr << "whoServer: something unexpected happened!" << endl;
                        
                    // Also, insert country in countries set
                    if (!countries.exists(country))
                        countries.insert(country);
                        
                    // Unlock mutex
                    if (pthread_mutex_unlock(&recmtx))
                        errExit("pthread_mutex_unlock");
                } 
            
            } while (1);    // Exit reading statistics sent from worker
            
            
            
            //
            // Save socket for communication with worker
            //
            
            // Lock mutex
            if (pthread_mutex_lock(&mtxworkerSock))
                errExit("pthread_mutex_lock");
            
            workerSock[iworkerSock] = p.first;
            iworkerSock++;
            
            //cout << "\nworkerSock: " << endl;
            //for (int i=0; i<numWorkers; i++)
            //    cout << workerSock[i] << endl;
            //cout << endl;
            
            // Unlock mutex
            if (pthread_mutex_unlock(&mtxworkerSock))
                errExit("pthread_mutex_unlock");
        
        }   // End if read from statsPort
    
    }   // End handling file descriptor
   
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

