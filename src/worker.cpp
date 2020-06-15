#include <arpa/inet.h>
#include <netinet/in.h>	     
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>	 
#include <sys/types.h>      
#include <sys/stat.h>
#include <unistd.h>        
#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <string>

#include "ageGroup.h"
#include "errExit.h"
#include "myDate.h"
#include "mySTL/myAVLTree.h"
#include "mySTL/myHashTable.h"
#include "mySTL/myList.h"
#include "patient.h"
#include "sendReceive.h"

#define ENTER       "ENTER"
#define EXIT        "EXIT"

static int flgsigint = 0;

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
  
    //cout << "inputDir = " << inputDir << endl;
    //cout << "bufferSize = " << bufferSize << endl;
    //cout << "servIP = " << servIP << endl;
    //cout << "servPort = " << servPort << endl;
    //cout << "countries = " << countries << endl;
    
    
    
    // Set signal handler
    static struct sigaction act;
    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);
    
    
    
    //
    // Connect to whoServer
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
    
    
    
    // Associative array of countries and set of dates checked so far for each country 
    myHashTable<string, myAVLTree<myDate> > countryDates;
    
    // Set of all patients this worker manages, indexed by id
    myAVLTree<patient> patients;
    
    // Read country dirs and update worker's and whoServer's data
    updateData(inputDir, bufferSize, countries, countryDates, patients, sock);

    //cout << "\nWorker " << getpid() 
      //  << " in total checked " << endl << countryDates
      //  << "and logged " << endl << patients;
    
    
    
    string id;
    patient pat;
            
    while (receive_id(sock, bufferSize, id))
    {        
        //cout << "worker received id = " << id << endl;
        
        if (flgsigint)
            break;
        
        pat.id = id;
        
        patient *ppat = patients.exists(pat); 
        
        if (ppat)
        {
            //cout << "Patient found:" << *ppat << endl;
            if (send_pat(sock, bufferSize, *ppat) == -1)
                errExit("send_pat");
        }
        else
        {
            //cout << "Not found!" << endl;
            if (send_null(sock, bufferSize) == -1)
                errExit("send_null");
        }
    }
    
    
    
    close(sock);
    
    //cout << "Worker exiting!" << endl;

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
    //int dbg = 0;
    
    
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
    if (signo == SIGINT)
        flgsigint = 1;
}
