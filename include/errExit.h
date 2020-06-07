#ifndef ERREXIT_H
#define ERREXIT_H

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)
                                   
#endif  // ERREXIT_H