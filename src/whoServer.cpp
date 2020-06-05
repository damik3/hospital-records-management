#include <stdio.h>
#include <string.h>
#include <pthread.h> 
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


using namespace std;

static int err;



int main(int argc, char* argv[])
{
    cout << "whoServer here!" << endl;
    
    return EXIT_SUCCESS;
}