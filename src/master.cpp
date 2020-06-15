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
    int &servPort,
    int workerPid[]);
    
void sigHandler(int signo);




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
    
    
    
    // Slot i contains pid of i-th worker
    int workerPid[numWorkers];
    
    // Read and assign inputDir to workers
    readAndAssign(numWorkers, bufferSize, inputDir, servIP, servPort, workerPid);
    
    
    
    // Set signal handler
    static struct sigaction act;
    sigfillset(&(act.sa_mask));
    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    
    
    
    // Wait for SIGINT of SIGSTP to terminate children and self
    while (1)
    {
        pause();
        
        if (dflgsigint)
        {
            //cout << "gonna kill my children!" << endl;
            
            for (int i=0; i<numWorkers; i++)
                kill(workerPid[i], SIGINT);
                
            break;
        }
    }
    
    
    
    // Wait for all workers to finish
    int status;
    while (wait(&status) > 0)
        ;;
        
    //cout << "master: All children finished" << endl;
    
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
    int &servPort,
    int workerPid[])
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
}