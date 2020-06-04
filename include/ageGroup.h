#ifndef AGEGROUP_H
#define AGEGROUP_H

#include <iostream>

#include "myPairs.h"
#include "mySTL/myPriorityQueue.h"

#define NUMGRPS 4    // Number of different age groups

class ageGroup 
{
    int group[NUMGRPS];
    int total;
    
public:
    ageGroup();
    bool ingroup(int g, int a);                 // Returns true if age belongs in group g, false otherwise
    void insert(int a);                         // Insert age a into appropriate group
    int& operator[] (int g);                    // Returns number of people in age group i
    int getTotal();                             // Returns total number of ages currently in all groups
    void setTotal();                            // Sets private member total to its appopriate count    
    void printGroup(std::ostream& os, int g);   // Prints group g into ostream os
    void print(std::ostream& os);               // Prints all groups into ostream os
    void printTopk(std::ostream& os, int k);    // Prints topk group categories (0 < k < NUMGRPS)
};

std::ostream& operator << (std::ostream& os, ageGroup g);
ageGroup operator+ (ageGroup& lhs, ageGroup& rhs);   // Adds count for each group as well as total

#endif  // AGEGROUP_H