#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>	     
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>	 
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "ageGroup.h"
#include "errExit.h"
#include "patient.h"
#include "myDate.h"
#include "myPairs.h" 
#include "mySTL/myAVLTree.h"
#include "mySTL/myHashTable.h"
#include "mySTL/myList.h"
#include "sendReceive.h"

#define WORKERPATH  "./bin/worker"
#define PIPEDIR     ".pipes"

static int dflgsigint = 0;
static int dflgsigusr1 = 0;

using namespace std;

void getCommandLineArguements(int argc, 
    char *argv[], 
    unsigned int *numWorkers, 
    unsigned int* bufferSize, 
    char *inputDir, 
    int &servPort,
    string &servIP);

void readAndAssign(unsigned int &numWorkers, 
    unsigned int &bufferSize,
    char *inputDir, 
    string servIP,
    int &servPort);
void sigHandler(int signo);

void printLog(myList<string> &countries, 
    unsigned int qsucc, 
    unsigned int qfail);
    
void receiveFromWorkers(myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > >& rec_enter, 
    myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > >& rec_exit,
    int numWorkers,
    int workerPipe[],
    int bufferSize
    );




int main (int argc, char *argv[])
{    
    // Gonna need it in different places
    string s;
    
    
    
    // Read command line arguements 
    unsigned int numWorkers;
    unsigned int bufferSize;
    int servPort;
    string servIP;
    char inputDir[128];
    getCommandLineArguements(argc, argv, &numWorkers, &bufferSize, inputDir, servPort, servIP);
    
    //cout << "numWorkers = " << numWorkers << endl;
    //cout << "bufferSize = " << bufferSize<< endl;
    //cout << "servPort = " << servPort << endl;
    //cout << "servIP = " << servIP << endl;
    //cout << "inputDir = " << inputDir << endl;
    
    
    
    //
    // Send number of workers to whoServer
    //
    
    // IP dot-number into binary form (network byte order)
    struct in_addr addr;
    inet_aton(servIP.c_str(), &addr);
    
    // Get host
    struct hostent* host;  
    host = gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
    if (host == NULL)
        errExit("gethostbyaddr");
    //printf("servIP:%s Resolved to: %s\n", servIP.c_str(),host->h_name);
    
    // Get server     
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, host->h_addr, host->h_length);
    server.sin_port = htons(servPort); 
    
    // Create socket for conncection with server
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errExit("socket");
        
    // Connect to server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
        errExit("connect");
    //printf("Connected successfully!\n");
    
    // Send the numWorkers variable
    if (write_data(sock, (char *)&numWorkers, sizeof(int), bufferSize) < 0)
        errExit("write_data");
        
    close(sock);
    
    
    
    
    // Read and assign inputDir to workers
    readAndAssign(numWorkers, bufferSize, inputDir, servIP, servPort);
    
    
    
    // Wait for all workers to finish
    int status;
    while (wait(&status) > 0)
        ;;
    cout << "master: All children finished" << endl;
    
    
    
    return 0; 
    


/************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************/



    //
    // Data structures used by master
    // 
    
    int numCountries = 0;
    
    // List of all countries 
    myList<string> countries;
    
    // Slot i contains a list of countries assigned to worker i 
    myList<string> workerCountries[numWorkers];
    
    // Slot i contains pid of i-th worker
    int workerPid[numWorkers];
    
    // Slot i contains fd for reading data from i-th worker 
    int workerPipe[numWorkers];
    
    // Slot i contains fd for sending data to i-th worker
    int toworkerPipe[numWorkers];
    
    
    // 2d associative array (rows are diseases, columns are countries).
    // [d][c] element is a tree sorted by myDate containing ageGroups.
    // 1 struct for patients(records) that entered and 1 for those that exited
    myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > > rec_enter;
    myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > > rec_exit;
 
    // Number of succesfull queries
    unsigned int qsucc = 0;
    
    // Number of failed queries
    unsigned int qfail = 0;
 
    
    
    // Create directory for fifos 
    int pipeDir = mkdir(PIPEDIR, 0777);
    
    if (pipeDir == -1)
    {
        perror("mkdir");
        exit(1);
    }
    

        
    // Set signal handler
    static struct sigaction act;
    sigfillset(&(act.sa_mask));
    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
  
    
    
    // Receive data from all workers
    receiveFromWorkers(rec_enter, rec_exit, numWorkers, workerPipe, bufferSize);
    
    
    
    // Create poll structs
    struct pollfd *pfds;
    pfds = (struct pollfd *)calloc(numWorkers, sizeof(struct pollfd));
    if (pfds == NULL)
    {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (int i=0; i<numWorkers; i++)
    {
        pfds[i].fd = workerPipe[i];
        pfds[i].events = POLLIN;
    }
    
    
    
    //
    // Read commands from input
    //
    
    string command("");
    char c[64];
    
    while ( command != "/exit" ) 
    {
        //cin >> command;   // For some weird reason it does not work after receiving SIGUSR1
        
        scanf("%63s", c);
        
        command = c;
        
        //if (cin.eof())
         //   break;
            
        // Check for signals
        if (dflgsigint)
        {   
            dflgsigint = 0;
            printLog(countries, qsucc, qfail);   
            break;
        }
        
        if (dflgsigusr1)
        {
            //cout << "Parent got SIGUSR1" << endl;
            dflgsigusr1 = 0;
            

            int ret;
            string s;
            string country;
            string disease;
            myDate d;
            ageGroup group;
            indexPair<myDate, ageGroup> ipair;
            
            if (poll(pfds, numWorkers, -1) == -1)
            {
                perror("poll");
                return EXIT_FAILURE;
            }
            
            // Search for worker that sent the signal
            for (int i=0; i<numWorkers; i++)
            {    
                if (pfds[i].revents != 0)
                {
                    if (pfds[i].revents & POLLIN)
                    {
                    
                        //cout << "Worker that sent data is " << workerPid[i] << endl;
                        
                        // Receive data
                        while (1)
                        {
                            ret = receive_data(workerPipe[i], bufferSize, d, s, group, country, disease);
                            
                            //cout << "da: ret = " << ret << endl;
                            
                            // If end of data, break
                            if (ret == sizeof(char))
                                break;
                              
                            
                            //cout << "\ndiseaseAggregator received " 
                              //  << country << endl
                              //  << disease << endl
                              //  << d << endl
                              //  << s << endl
                              //  << group;
                             
                             
                            // Insert data into data struct
                            
                            ipair.first = d;
                            ipair.second = group;
                            
                            if (s == "EN")
                                rec_enter[disease][country].insert(ipair);
                            else if (s == "EX")
                                rec_exit[disease][country].insert(ipair);
                            else
                                cerr << "diseaseAggregator: something unexpected happened!" << endl;
                         
                        }
                    }
                }
            }
            
            continue;
        }
        
        
        
        // By default asssume succesfull query, unless stated otherwise
        qsucc++;
        
        
        
        if (command == "/listCountries")
        {
            myList<string>::iterator it;
            
            for (int i=0; i<numWorkers; i++)
                for (it = workerCountries[i].begin(); it.isValid(); ++it)
                    cout << *it << " " << workerPid[i] << endl;
        }
        
        
        
        else if (command == "/diseaseFrequency")
        {
            string disease, country;
            myDate d1, d2;
            
            cin >> disease >> d1 >> d2;
            
            string line;
            getline(cin, line);
            
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
                            
                cout << count << endl;
            }
            
            else
            {
                count = 0;
                myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
                
                if (countries.exists(country))
                    for (it = rec_enter[disease][country].begin(); it.isValid(); ++it)
                        if ((d1 < it->first) && (it->first < d2))
                            count += it->second.getTotal();
                            
                cout << count << endl;
            }
        }
        
        
        
        else if (command == "/topk-AgeRanges")
        {
            int k;
            string country;
            string disease;
            myDate d1;
            myDate d2;
            
            cin >> k >> country >> disease >> d1 >> d2;
            
            ageGroup g;
            myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
        
            if (countries.exists(country))
                for (it = rec_enter[disease][country].begin(); it.isValid(); ++it)
                        if ((d1 < it->first) && (it->first < d2))
                            g = g + it->second;
                           
            //cout << "\nPrinting all groups" << endl << g << endl;
 
            g.printTopk(cout, k);
        }
        
        
        
        else if (command == "/searchPatientRecord")
        {
            string id;
            cin >> id;
                        
            // Signal workers to search for patient
            for (int i=0; i<numWorkers; i++)
            {
                send_id(toworkerPipe[i], bufferSize, id);
                kill(workerPid[i], SIGUSR2);
            }
            
            
            
            bool found = false;
            patient pat;
            patient patfound;
            ssize_t ret;
            
            for (int i=0; i<numWorkers; i++)
            {
                // Listen for i-th worker's response
                do 
                {
                    ret = receive_pat(workerPipe[i], bufferSize, pat);
                    
                } while (ret <= 0);
                
                // If patient was found
                if (ret > sizeof(char))
                {
                    found = true;
                    patfound = pat;
                }
            }
            
            
            
            if (found)
                cout << patfound << endl;
            else
                cout << "Patient not found!" << endl;
        }
        
        
        
        else if (command == "/numPatientAdmissions")
        {
            string disease, country;
            myDate d1, d2;
            
            cin >> disease >> d1 >> d2;
            
            string line;
            getline(cin, line);
            
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
                myList<string>::iterator itCountries;
                myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
                
                for (itCountries = countries.begin(); itCountries.isValid(); ++itCountries)
                {
                    count = 0;
                    
                    for (it = rec_enter[disease][*itCountries].begin(); it.isValid(); ++it)
                        if ((d1 < it->first) && (it->first < d2))
                            count += it->second.getTotal();
                            
                    cout << *itCountries << " " << count << endl;
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
                
                cout << count << endl;
            }
        }
        
        
        
        else if (command == "/numPatientDischarges")
        {
            string disease, country;
            myDate d1, d2;
            
            cin >> disease >> d1 >> d2;
            
            string line;
            getline(cin, line);
            
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
                myList<string>::iterator itCountries;
                myAVLTree<indexPair<myDate, ageGroup > >::iterator it;
                
                for (itCountries = countries.begin(); itCountries.isValid(); ++itCountries)
                {
                    count = 0;
                    
                    for (it = rec_exit[disease][*itCountries].begin(); it.isValid(); ++it)
                        if ((d1 < it->first) && (it->first < d2))
                            count += it->second.getTotal();
                    
                    cout << *itCountries << " " << count << endl;
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
                
                cout << count << endl;
            }
        }
        
        
        
        // Wrong query
        else
        {
            qsucc--;
            qfail++;
        }
    }

    
    
    // Kill children
    for (int i=0; i<numWorkers; i++)
        kill(workerPid[i], SIGKILL);
    
    
    
    // Close pipes
    for (int i=0; i<numWorkers; i++)
        close(workerPipe[i]);
        
    for (int i=0; i<numWorkers; i++)
        close(toworkerPipe[i]);
    
    
    
    // Recursively remove PIPEDIR 
    s = "rm -rf ./";
    s += PIPEDIR;
    system(s.c_str());
    
    
    
    free(pfds);
    
    
    return 0;
}




void getCommandLineArguements(int argc, 
    char *argv[], 
    unsigned int *numWorkers, 
    unsigned int* bufferSize, 
    char *inputDir, 
    int &servPort,
    string &servIP)
{
    string usage("Usage: master –w numWorkers -b bufferSize –s serverIP  –p serverPort -i input_dir");
    
    if (argc != 11)
    {
        cerr << usage << endl;
        exit(1);
    }
    
    string w("-w"), b("-b"), s("-s"), p("-p"), i("-i");
    
    for (int j=1; j<11; j++)
    {
        if (w.compare(argv[j]) == 0)
            *numWorkers = atoi(argv[++j]);
        else if (b.compare(argv[j]) == 0)
            *bufferSize = atoi(argv[++j]);
        else if (s.compare(argv[j]) == 0)
            servIP = argv[++j];
        else if (p.compare(argv[j]) == 0)
            servPort = atoi(argv[++j]);
        else if (i.compare(argv[j]) == 0)
            strcpy(inputDir, argv[++j]);
        else
        {
            cerr << usage << endl;
            exit(1);
        }
    }
}




void readAndAssign(unsigned int &numWorkers, 
    unsigned int &bufferSize,
    char *inputDir, 
    string servIP,
    int &servPort)
{
    
    // Open inputDir 
    DIR *inputDirPtr;
    if ( (inputDirPtr = opendir(inputDir)) == NULL)
    {
        perror("opendir");
        exit(1);
    }
    

    //
    // Read inputDir
    //
    
    int numCountries = 0;
    myList<string> workerCountries[numWorkers];
    struct dirent *direntP;
    string s;
    
    while ( (direntP = readdir(inputDirPtr)) != NULL)
    {
        // Ignore . and .. directories
        if (!strcmp(direntP->d_name, ".") || !strcmp(direntP->d_name, ".."))
            continue;
  
        // Assign country to worker 
        workerCountries[numCountries % numWorkers].insert(s = direntP->d_name);
        
        numCountries++;
    }
    
    
    
    // Close inputDir
    closedir(inputDirPtr);
    
    
    //
    // Fork workers 
    //
    
    myList<string>::iterator itWorkerCountries;
    
    for (int i=0; i<numWorkers; i++)
    {
        
        int workerPid = fork();
        
        
        
        // Case: fork failed
        if (workerPid < 0)
        {
            perror("fork");
            exit(1);
        }
          
          
          
        // Case: forked child
        else if (workerPid == 0)
        { 
            // Build argv of i-th worker 
            
            int n = workerCountries[i].getCount();
            
            // Array of n + 5 char pointers 
            char **cmd = (char **)malloc((n + 6) * sizeof(char *));
            
            cmd[0] = (char *)malloc( (strlen(WORKERPATH) + 1) * sizeof(char) );
            strcpy(cmd[0], WORKERPATH);
            
            // inputDir
            cmd[1] = (char *)malloc ( (strlen(inputDir) + 1) * sizeof(char));
            strcpy(cmd[1], inputDir);
            
            // bufferSize (suppose 63 digits is enough for bufferSize)
            cmd[2] = (char *)malloc ( 64 * sizeof(char));
            sprintf(cmd[2], "%63d", bufferSize);
            
            // servIP
            cmd[3] = (char *)malloc( (servIP.length() + 1) *sizeof(char) );
            strcpy(cmd[3], servIP.c_str());
            
            // servPort (suppose 32 digits is enough for servPort)
            cmd[4] = (char *)malloc( 64 *sizeof(char) );
            sprintf(cmd[4], "%63d", servPort);
            
            // For each country 
            
            itWorkerCountries = workerCountries[i].begin();
            
            for (int j=4; j<n+4; j++)
            {
                cmd[j+1] = (char *)malloc((itWorkerCountries->length() + 1) * sizeof(char));
                strcpy(cmd[j+1], itWorkerCountries->c_str());
                ++itWorkerCountries;
            }
            
            
            cmd[n+5] = NULL;
            
            execv(cmd[0], cmd);
            
            
            
            // Not supposed to reach here 
            
            perror("execv");
            exit(1);
        }
        
        
        
        // Case: father process
        else 
            ;;
    }
}




void sigHandler(int signo)
{
    if ( (signo == SIGINT) || (signo == SIGQUIT) )
        dflgsigint = 1;
    else if (signo == SIGUSR1)
        dflgsigusr1 = 1;
}




void printLog(myList<string> &countries, unsigned int qsucc, unsigned int qfail)
{
    string s;
    s = "./logs/";
    s += "log_file.";
    s += to_string(getpid());
    //cout << "Gonna write to " << s << endl;
    
    ofstream logFile(s);
    
    if (!logFile.is_open())
    {
        perror("open");
    }
    
    else
    {
        logFile << countries;
        logFile << "TOTAL " << qsucc + qfail << endl;
        logFile << "SUCCESS " << qsucc << endl;
        logFile << "FAIL " << qfail << endl;
        
        logFile.close();
    }
}




void receiveFromWorkers(myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > >& rec_enter, 
    myHashTable<string, myHashTable<string, myAVLTree<indexPair<myDate, ageGroup > > > >& rec_exit,
    int numWorkers,
    int workerPipe[],
    int bufferSize)
{
    
    // Slot i is true if i-th worker is done sending data
    bool workerDone[numWorkers];
    // Initialize to not done
    for (int i=0; i< numWorkers; i++)
        workerDone[i] = false;
    
    bool allDone;
    int ret;
    string s;
    string country;
    string disease;
    myDate d;
    ageGroup group;
    indexPair<myDate, ageGroup> ipair;
    
    
    
    // While not all workers are done sending data
    
    do
    {
        // Assume all done unless someone says otherwise
        allDone = true;
        
        // Receive a packet from each worker 
        for (int i=0; i<numWorkers; i++)
        {
            // If worker is not done sending
            if (!workerDone[i])
            {
                ret = receive_data(workerPipe[i], bufferSize, d, s, group, country, disease);
                
                if (ret < 0)
                {
                    allDone = false;
                    //cout << "da: ret < 0" << endl;
                }
                
                else if (ret == 0)
                {
                    allDone = false;
                    //cout << "da: ret == 0" << endl;
                }
                
                // Not supposed to happen
                else if ((0 < ret ) && (ret < sizeof(char)))
                    cout << "WUT" << endl;
                    
                // If this is the last packet of i-th worker, mark him done
                else if (ret == sizeof(char))
                    workerDone[i] = true;
                    
                // If worker sent data
                else
                {
                    allDone = false;
                    
                    /*
                    cout << "\ndiseaseAggregator received " 
                        << country << endl
                        << disease << endl
                        << d << endl
                        << s << endl
                        << group;
                    */
                    
                    ipair.first = d;
                    ipair.second = group;
                    
                    if (s == "EN")
                        rec_enter[disease][country].insert(ipair);
                    else if (s == "EX")
                        rec_exit[disease][country].insert(ipair);
                    else
                        cerr << "master: something unexpected happened!" << endl;
                }
            }
        }
        
    } while (!allDone);
}


