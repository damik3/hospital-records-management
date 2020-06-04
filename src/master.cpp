#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "myDate.h"
#include "patient.h"
#include "ageGroup.h"
#include "mySTL/myList.h"
#include "mySTL/myAVLTree.h"
#include "mySTL/myHashTable.h"
#include "sendReceive.h"
#include "myPairs.h" 

#define WORKERPATH  "./bin/worker"
#define PIPEDIR     ".pipes"

extern int errno;

static int dflgsigint = 0;
static int dflgsigusr1 = 0;

using namespace std;

void getCommandLineArguements(int argc, 
    char *argv[], 
    unsigned int *numWorkers, 
    unsigned int* bufferSize, 
    char *inputDir);

void readAndAssign(char *inputDir, 
    unsigned int &bufferSize,
    unsigned int &numWorkers, 
    int &numCountries, 
    myList<string> &countries, 
    pid_t workerPid[], 
    myList<string> workerCountries[],
    int workerPipe[],
    int toworkerPipe[]);

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
    char inputDir[128];
    getCommandLineArguements(argc, argv, &numWorkers, &bufferSize, inputDir);
    
    
    
    // Create directory for fifos 
    int pipeDir = mkdir(PIPEDIR, 0777);
    
    if (pipeDir == -1)
    {
        perror("mkdir");
        exit(1);
    }



    //
    // Data structures used by diseaseAggregator
    //
    
    int numCountries = 0;
    
    // List of all countries 
    myList<string> countries; 
    
    // Slot i contains pid of worker i 
    pid_t workerPid[numWorkers];
    
    // Slot i contains a list of countries assigned to worker i 
    myList<string> workerCountries[numWorkers];
    
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
    
    
    
    // Read and assign inputDir to workers
    readAndAssign(inputDir, bufferSize, numWorkers, numCountries, countries, workerPid, workerCountries, workerPipe, toworkerPipe);
    

        
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
                              
                            /*
                            cout << "\ndiseaseAggregator received " 
                                << country << endl
                                << disease << endl
                                << d << endl
                                << s << endl
                                << group;
                             */
                             
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




void getCommandLineArguements(int argc, char *argv[], unsigned int *numWorkers, unsigned int* bufferSize, char *inputDir)
{
    string usage("Usage: diseaseAggregator –w numWorkers -b bufferSize -i input_dir");
    
    if (argc != 7)
    {
        cerr << usage << endl;
        exit(1);
    }
    
    string w("-w"), b("-b"), i("-i");
    
    for (int j=1; j<7; j++)
    {
        if (w.compare(argv[j]) == 0)
            *numWorkers = atoi(argv[++j]);
        else if (b.compare(argv[j]) == 0)
            *bufferSize = atoi(argv[++j]);
        else if (i.compare(argv[j]) == 0)
            strcpy(inputDir, argv[++j]);
        else
        {
            cerr << usage << endl;
            exit(1);
        }
    }
}




void readAndAssign(char *inputDir, 
    unsigned int &bufferSize,
    unsigned int &numWorkers, 
    int &numCountries, 
    myList<string> &countries,  
    pid_t workerPid[], 
    myList<string> workerCountries[],
    int workerPipe[],
    int toworkerPipe[])
{
    
    // Open inputDir 
    
    DIR *inputDirPtr;
    
    if ( (inputDirPtr = opendir(inputDir)) == NULL)
    {
        perror("opendir");
        exit(1);
    }
    

    
    // Read inputDir
    
    struct dirent *direntP;
    string s;
    
    while ( (direntP = readdir(inputDirPtr)) != NULL)
    {
        // Ignore . and .. directories
        if (!strcmp(direntP->d_name, ".") || !strcmp(direntP->d_name, ".."))
            continue;
  
        // Assign country to worker 
        workerCountries[numCountries % numWorkers].insert(s = direntP->d_name);
        
        // Save country in country list
        countries.insert(s);
        
        numCountries++;
    }
    
    
    
    // Close inputDir
    
    closedir(inputDirPtr);
    
    
    
    // Fork workers 
    
    pid_t ppid = getpid();
    
    myList<string>::iterator itWorkerCountries;
    
    for (int i=0; i<numWorkers; i++)
    {
        
        workerPid[i] = fork();
        
        
        
        // Case: fork failed
        if (workerPid[i] < 0)
        {
            perror("fork");
            exit(1);
        }
          
          
          
        // Case: forked child
        else if (workerPid[i] == 0)
        { 
            // Build argv of i-th worker 
            
            int n = workerCountries[i].getCount();
            
            // Array of n + 5 char pointers 
            char **cmd = (char **)malloc((n + 5) * sizeof(char *));
            
            cmd[0] = (char *)malloc( (strlen(WORKERPATH) + 1) * sizeof(char) );
            strcpy(cmd[0], WORKERPATH);
            
            // Supose 16 digits is enough for a process id
            cmd[1] = (char *)malloc( 16 * sizeof(char) );
            sprintf(cmd[1], "%15d", ppid);
            
            cmd[2] = (char *)malloc ( (strlen(inputDir) + 1) * sizeof(char));
            strcpy(cmd[2], inputDir);
            
            // Suppose 63 digits is enough for bufferSize
            cmd[3] = (char *)malloc ( 64 * sizeof(char));
            sprintf(cmd[3], "%63d", bufferSize);
            
            
            // For each country 
            
            itWorkerCountries = workerCountries[i].begin();
            
            for (int j=3; j<n+3; j++)
            {
                cmd[j+1] = (char *)malloc((itWorkerCountries->length() + 1) * sizeof(char));
                strcpy(cmd[j+1], itWorkerCountries->c_str());
                ++itWorkerCountries;
            }
            
            
            cmd[n+4] = NULL;
            
            execv(cmd[0], cmd);
            
            
            
            // Not supposed to reach here 
            
            perror("execv");
            exit(1);
        }
        
        
        
        // Case: father process
        else 
        {
            // Create and open pipe for current worker
        
            s = PIPEDIR;
            s += "/pipe";
            s += to_string(workerPid[i]);
            
            if (mkfifo(s.c_str(), 0666) == -1 )
            {
                perror("mkfifo");
                exit(1);
            }
            
            if ((workerPipe[i] = open(s.c_str(), O_RDWR)) == -1)
            {
                perror("diseaseAggregator: open worker pipe");
                exit(1);
            }
            
            s = PIPEDIR;
            s += "/2pipe";
            s += to_string((workerPid[i]));
            
            if (mkfifo(s.c_str(), 0666) == -1 )
            {
                perror("mkfifo");
                exit(1);
            }
            
            if ((toworkerPipe[i] = open(s.c_str(), O_RDWR)) == -1)
            {
                perror("diseaseAggregator: open worker pipe");
                exit(1);
            }
        }
    }
}




void sigHandler(int signo)
{
    if ( (signo == SIGINT) || (signo == SIGQUIT) )
        dflgsigint = 1;
    else if (signo == SIGUSR1);
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
                        cerr << "diseaseAggregator: something unexpected happened!" << endl;
                }
            }
        }
        
    } while (!allDone);
}


