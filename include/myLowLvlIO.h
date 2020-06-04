#ifndef MYLOWLVLIO_H
#define MYLOWLVLIO_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * Read up to count bytes from file descriptor fd into the buffer starting at arr.
 * While reading, split the message into segments of size bufsiz each 
 * (except maybe for the last segment).
 */
ssize_t read_data(int fd, char *arr, size_t count, size_t bufsiz);


/*
 * Write count bytes from buffer starting at arr into file descriptor fd.
 * While writing, split the message into segments of size bufsiz each 
 * (except maybe for the last segment).
 */
ssize_t write_data(int fd, char *arr, size_t count, size_t bufsiz);


#endif  // MYLOWLVLIO_H