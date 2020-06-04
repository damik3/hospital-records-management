#ifndef SENDRECEIVE_H
#define SENDRECEIVE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>      

#include <iostream>
#include <string>

#include "myDate.h"
#include "ageGroup.h"
#include "myLowLvlIO.h"
#include "patient.h"


/*
 * Send formatted data into file descriptor fd, using buffer of size bufferSize 
 */
ssize_t send_data(int fd, unsigned int bufferSize, myDate &date, string &en, ageGroup &group, string &country, string &disease);


/*
 * Send a message of size 1b containing only the '\0' charachter, meaning the sequence of packets has ended
 */
ssize_t send_null(int fd, unsigned int bufferSize);


/*
 * Send id of patient 
 */
ssize_t send_id(int fd, unsigned int bufferSize, string id);

/*
 * Send patient record
 */
ssize_t send_pat(int fd, unsigned int bufferSize, patient pat);


/*
 * Receive formatted data from file descriptor fd, using buffer of size bufferSize 
 */
ssize_t receive_data(int fd, unsigned int bufferSize, myDate &date, string &en, ageGroup &group, string &country, string &disease);


/*
 * Send id of patient 
 */
ssize_t receive_id(int fd, unsigned int bufferSize, string& id);


/*
 * Receive patient record
 */
ssize_t receive_pat(int fd, unsigned int bufferSize, patient& pat);


#endif  // SENDRECEIVE_H