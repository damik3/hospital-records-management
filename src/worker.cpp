#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>      
#include <sys/stat.h>
#include <unistd.h>        
#include <fcntl.h>
#include <dirent.h>

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

#define PIPEDIR     ".pipes"
#define ENTER       "ENTER"
#define EXIT        "EXIT"

#define MAX_BACKOFF 3600    // 1 hour
#define INITIAL_BACKOFF 1   // 1 sec

static int backoff = INITIAL_BACKOFF;
static int flgsigint = 0;
static int flgsigusr1 = 0;
static int flgsigusr2 = 0;

using namespace std; 




void getCommandLineArguements(int argc, 
    char *argv[], 
    string &inputDir, 
    unsigned int &bufferSize, 
    string &servIP,
    int &servPort,
    myList<string>& countries);

void updateData(string inputDir, 
    unsigned int bufferSize, 
    myList<string> &countries, 
    myHashTable<string, myAVLTree<myDate> > &countryDates,
    myAVLTree<patient> &patients,
    int fdpipe);

void sigHandler(int signo);

void printLog(myList<string> &countries, unsigned int qsucc, unsigned int qfail);




int main (int argc, char *argv[])
{
    
    // Gonna need it in different places
    string s;
    
    
    
    // Read command line args 
    
    string inputDir;    
    unsigned int bufferSize;
    string servIP;
    int servPort;
    myList<string> countries;   
    
    getCommandLineArguements(argc, argv, inputDir, bufferSize, servIP, servPort, countries);
  
    cout << "inputDir = " << inputDir << endl;
    cout << "bufferSize = " << bufferSize << endl;
    cout << "servIP = " << servIP << endl;
    cout << "servPort = " << servPort << endl;
    cout << "countries = " << countries << endl;
    
    
     
    return 0; 
    
/************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************/

    
    pid_t parentpid;


    
    // Set signal handler
    
    static struct sigaction act;
    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);

    
    
    // Open pipe to diseaseAggregator
    
    s = PIPEDIR;
    s += "/pipe";
    s += to_string(getpid());
    
    int fdpipe;
    
    // Wait for diseaseAggregator to create pipes 
    
    while ((fdpipe = open(s.c_str(), O_RDWR)) == -1)
        ;;
        
    s = PIPEDIR;
    s += "/2pipe";
    s += to_string(getpid());
    
    int fromfdpipe;
    
    while ((fromfdpipe = open(s.c_str(), O_RDWR)) == -1)
        ;;
            
            
    
    // Associative array of countries and set of dates checked so far for each country 
    myHashTable<string, myAVLTree<myDate> > countryDates;
    
    // Set of all patients this worker manages, indexed by id
    myAVLTree<patient> patients;
    
    // Number of succesfull queries
    unsigned int qsucc = 0;
    
    // Number of failed queries
    unsigned int qfail = 0;
    
    
    
    // Initialize data
    
    updateData(inputDir, bufferSize, countries, countryDates, patients, fdpipe);
    
    //cout << "\nWorker " << getpid() 
      //  << " in total checked " << endl << countryDates
      //  << "and logged " << endl << patients;
    


    while (1)
    {
        //printf("worker: gonna sleep for %d seconds\n", backoff);
        sleep(backoff);
        backoff = backoff << 2;
        
        if (backoff > MAX_BACKOFF)
            backoff = MAX_BACKOFF;
        
    
            
        // Check for signal flags
        
        if (flgsigint) 
        {
            //printf("worker: gonna do some SIGINT stuff\n");
            flgsigint = 0;
            printLog(countries, qsucc, qfail);
        }
        
        
        
        if (flgsigusr1)
        {
            //printf("main: gonna do some SIGUSR1 stuff\n");
            flgsigusr1 = 0;
            updateData(inputDir, bufferSize, countries, countryDates, patients, fdpipe);
            kill(parentpid, SIGUSR1);
        }
        
        
        
        if (flgsigusr2)
        {
            //printf("main: gonna do some SIGUSR2 stuff\n");
            flgsigusr2 = 0;
            
            string id;
            
            receive_id(fromfdpipe, bufferSize, id);
                        
            patient pat;
            pat.id = id;
            
            patient *ppat = patients.exists(pat);
            
            if (ppat)
                send_pat(fdpipe, bufferSize, *ppat);
            else
                send_null(fdpipe, bufferSize);
        }
    }
    
    
    
    close(fdpipe);
    close(fromfdpipe);
    
    return 0;
}




void getCommandLineArguements(int argc, 
    char *argv[], 
    string &inputDir, 
    unsigned int &bufferSize, 
    string &servIP,
    int &servPort,
    myList<string>& countries)
{
    string usage("Usage: worker inputDir bufferSize servIP servPort country1 country2 country3 ... ");
    
    if (argc < 5)
    {
        cerr << usage << endl;
        exit(1);
    }
    
    inputDir = argv[1];
    bufferSize = atoi(argv[2]);
    servIP = argv[3];
    servPort = atoi(argv[4]);
    
    string s;
    
    for (int i=5; i<argc; i++)
        countries.insert(s = argv[i]);
}




void updateData(string inputDir, 
    unsigned int bufferSize, 
    myList<string> &countries, 
    myHashTable<string, myAVLTree<myDate> > &countryDates,
    myAVLTree<patient> &patients,
    int fdpipe)
{
    // If no countries assigned to this worker, send_null and return
    if (countries.empty())
    {
        send_null(fdpipe, bufferSize);
        return;
    }
        
    
    string s;
    
    // For iterating over countries
    myList<string>::iterator itCountries;
    
    // For iterating over input directory
    DIR *inputDirPtr;
    struct dirent *direntP;
    
    // For reading date files
    ifstream dateFile;
    
    // For reading patient records
    patient pat;
    patient *ppat;
    string enter;
    
    // For debugging purposes;
    int dbg = 0;
    
    
    // For each country assigned
    
    for (itCountries = countries.begin(); itCountries.isValid(); ++itCountries)
    {
        s = inputDir;
        s += "/";
        s += *itCountries;
        
        if ( (inputDirPtr = opendir(s.c_str())) == NULL)
        {
            perror("opendir");
            exit(1);
        }
        
        // Ordered set of newly added date files in current countryDir
        myAVLTree<myDate> newDates;
        
        
        
        // For each date file in current country assigned
        
        while ( (direntP = readdir(inputDirPtr)) != NULL)
        {
            // Ignore . and .. directories
            if (!strcmp(direntP->d_name, ".") || !strcmp(direntP->d_name, ".."))
                continue;
            
            myDate d(s = direntP->d_name);
                        
            // If date has been already checked, continue
            if (countryDates[*itCountries].exists(d))
                continue;
                                
            // Else, mark checked
            countryDates[*itCountries].insert(d);
            
            // And temporarily add it into sorted set so that we can read them sorted
            newDates.insert(d);
            
        }
                        
        myAVLTree<myDate>::iterator itnewDates;
        
        // For each date not already stored (in a sorted manner)
        
        for (itnewDates = newDates.begin(); itnewDates.isValid(); ++itnewDates)
        {
            
            // Open date file
            
            s = inputDir;
            s += "/";
            s += *itCountries;
            s += "/";
            s += itnewDates->toString();
            dateFile.open(s);
            
            //cout << "Gonna check " << s << endl;
            
            if(!dateFile.is_open()){
                perror("open");
                continue;
            }
            
            
             
            // For each country, the statistics of each group for patients ENTERED
            myHashTable<string, ageGroup> patenterd;
            
            // For each country, the statistics of each group for patients EXITED
            myHashTable<string, ageGroup> patexitd;
            
            myHashTable<string, ageGroup>::iterator itpatnx;
   
            // For each patient in dateFile
            
            while (dateFile >> pat.id)
            {
                dateFile >> enter >> pat.fname >> pat.lname >> pat.disease >> pat.age;
                
                
                
                // Case: patient exists (based on its id)
                if (ppat = patients.exists(pat))
                {
                    // Only acceptable case is exiting with entry date < exit date
                    if ((enter == EXIT) && ppat->exitDate.isNull() && (ppat->entryDate < *itnewDates)) 
                    {
                        ppat->exitDate = *itnewDates;
                        
                        patexitd[ppat->disease].insert(ppat->age);
                    }
                    
                    else
                        cerr << "ERROR" << endl;
                }
                
                
                
                // Case: patient does not exist and is entering
                else if (enter == ENTER)
                {
                    pat.entryDate = *itnewDates;
                    patients.insert(pat);
                    
                    patenterd[pat.disease].insert(pat.age);
                }
                
                
                
                // Case: not supposed to happen
                else
                    cerr << "ERROR" << endl;
            }
            
            //cout << "\n****Simulating pipe data***\n" << endl;
            
            for (itpatnx = patenterd.begin(); itpatnx.isValid(); ++itpatnx)
            {
                /*
                cout << *itCountries << endl;
                cout << itpatnx->first << endl;
                cout << *itnewDates << endl;
                cout << "EN" << endl;
                for (int i=0; i<NUMGRPS; i++)
                    cout << itpatnx->second[i] << endl;
                cout << endl;
                 */ 
                 
                send_data(fdpipe, bufferSize, *itnewDates, s = "EN", itpatnx->second, *itCountries, itpatnx->first);
            }
            
            for (itpatnx = patexitd.begin(); itpatnx.isValid(); ++itpatnx)
            {
                /*
                cout << *itCountries << endl;
                cout << itpatnx->first << endl;
                cout << *itnewDates << endl;
                cout << "EX" << endl;
                for (int i=0; i<NUMGRPS; i++)
                    cout << itpatnx->second[i] << endl;
                cout << endl;
                 */
                
                send_data(fdpipe, bufferSize, *itnewDates, s = "EX", itpatnx->second, *itCountries, itpatnx->first);
            }
            
            // Close date file
            dateFile.close();
            
        }
            
        
        closedir(inputDirPtr);  
    }
    
    send_null(fdpipe, bufferSize);
}




void sigHandler(int signo)
{
    if ( (signo == SIGINT) || (signo == SIGQUIT) )
        flgsigint = 1;
    else if (signo == SIGUSR1)
        flgsigusr1 = 1;
    else if (signo == SIGUSR2)
        flgsigusr2 = 1;
        
    backoff = INITIAL_BACKOFF;
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

