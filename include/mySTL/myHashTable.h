#ifndef MYHASHTABLE_H
#define MYHASHTABLE_H

#include <utility>
#include <iostream>

#include "myHashFunc.h"

#define CONST_N 16      // Number of hashTable entries
#define CONST_B 128     // Size of bucket in bytes

using namespace std;



class myHashTableExcpetion;



template <class Key, class T, class H = myHashFunc>
class myHashTable {
    
    struct bucket {
        
        pair<Key, T> *array;
        int maxPairs;
        int count;
        bucket *next;
        
        
        
        /*** Generic functions ***/
        
        
        
        bucket() : array(NULL), 
            maxPairs(0), 
            count(0), 
            next(NULL) 
            {}
        
        
        
        ~bucket() {
            
            if (next)
                delete this->next;
                
            delete[] this->array;
        }
        
        
        
        T& at(const Key &key) {
            
            // Search in this bucket
            
            int i;
            
            for (i=0; i<this->count; i++) {
                
                if (this->array[i].first == key) 
                    return this->array[i].second;
            }
            
            // If not found in this bucket
                
            // If bucket has enough space, insert here
                
            if (i < this->maxPairs) {
                
                count ++;
                this->array[i].first = key;
                return this->array[i].second;
            }
                
            // If bucket has not enough space
                
            else {
                  
                // If there is not an overflow bucket, create it
                
                if (this->next == NULL) {
                    
                    next = new bucket;
                    next->array = new pair<Key, T>[this->maxPairs];
                    next->count = 0;
                    next->maxPairs = this->maxPairs;
                    next->next = NULL;
                }
                
                // Search in overflow bucket
                
                return this->next->at(key);
            }
            
        }
        
        
        
        void print(ostream &os) {
            
            for (int i=0; i<this->count; i++)
                os << this->array[i].first << ", " << this->array[i].second << endl;
                
            if (this->next)
                this->next->print(os);
        }
    
    };
    
    
    
    /* Private members */

    bucket *hashTable;
    int n;              // Number of hashTable entries
    int bucketSize;     // Size of bucket in bytes
    H hashfunc;         // Hash function passed as a template 
    
public:


    
    struct iterator
    {
        int i_curr;             // Current index for hashTable
        int b_i_curr;           // Current index for bucket
        bucket *b_curr;         // Current bucket
        bucket **hashTablePtr;  // Ptr to hashTable
        int n;                  // Number of hashTable entries
        
        
        
        iterator() 
            : i_curr(-1),
            b_i_curr(-1), 
            b_curr(NULL), 
            hashTablePtr(NULL),
            n(0)
            {}
        
        
        
        bool isValid()
        {
            return b_curr != NULL;
        }
        
        
        
        void operator++ ()
        {
            
            /* Check if operator is valid */
            
            if (!isValid())
                return;
                
            /* If there is a next record in curr bucket, point to it and return */
            
            if (b_i_curr < b_curr->count - 1)
            {
                b_i_curr++;
                return;
            }   
            
            /* Else, if there is an overflow bucket, point to its first record and return*/
            
            else if (b_curr->next)
            {
                b_curr = b_curr->next;
                b_i_curr = 0;
                return;
            }
            
            /* Else, search in all remaining hashTable entries */
            
            else 
            {
                do 
                {
                    /* Check next hashTable entry */
                    i_curr++;
                    
                    /* At 1st record */
                    b_i_curr = 0;
                    
                    /* Check validity of i_curr */
                    b_curr = (i_curr < n) ? &((*hashTablePtr)[i_curr]) : NULL;
                    
                    /* If i_curr is valid and bucket is not empty, point to its first record and return */
                    if (b_curr && b_curr->count)
                        return;
                
                } while (b_curr);
                
            }
            
        }
        
        
        
        pair<Key, T>* operator-> ()
        {
            if (isValid())
                return &(b_curr->array[b_i_curr]);
            else
                throw myHashTableExcpetion("Invalid iterator!");
        }
        
        
        
        pair<Key, T>& operator* ()
        {
            if (isValid())
                return b_curr->array[b_i_curr];
            else
                throw myHashTableExcpetion("Invalid iterator!");
        }
        
    };
    
    

    /*** Generic functions ***/
        
    myHashTable (const int &n = CONST_N, const int &bucketSize = CONST_B);
    ~myHashTable ();
    T& operator[] (const Key &key);
    void print(ostream &os);
    iterator begin();
};



/*** Generic functions ***/



template <class Key, class T, class H>
myHashTable<Key, T, H>::myHashTable (const int &n, const int &bucketSize) {
    
    this->hashTable = new bucket[n];
    this->n = n;
    this->bucketSize = bucketSize;
    
    // Initialize each bucket
    
    int maxPairs = bucketSize / (sizeof(Key) + sizeof(T));
    
    
    //cout << "sizeof(Key): " << sizeof(Key)
      //  << ", sizeof(T): " << sizeof(T)
      //  << ", max number of pairs per bucket: " << maxPairs << endl;
        
    for (int i=0; i<n; i++) {
        
        hashTable[i].array = new pair<Key, T>[maxPairs];
        hashTable[i].maxPairs = maxPairs;
        hashTable[i].count = 0;
        hashTable[i].next = NULL;
    }
    
}



template <class Key, class T, class H>
myHashTable<Key, T, H>::~myHashTable () {
    
    delete[] this->hashTable;
}



template <class Key, class T, class H>
T& myHashTable<Key, T, H>::operator[] (const Key &key) {
    
    unsigned int b = hashfunc(key, n);
    return hashTable[b].at(key);
}



template <class Key, class T, class H>
void myHashTable<Key, T, H>::print(ostream &os) {
    
    for (int i=0; i<this->n; i++)
        hashTable[i].print(os);
}



template <class Key, class T, class H>
inline ostream& operator<< (ostream& os, myHashTable<Key, T, H> &h) {
    
    h.print(os);
    return os;
}



template <class Key, class T, class H>
typename myHashTable<Key, T, H>::iterator myHashTable<Key, T, H>::begin()
{
    iterator it;
    
    /* For each bucket list */
    
    for (int i=0; i<n; i++) 
    {
        
        if (hashTable[i].count > 0) 
        {
            it.i_curr = i;
            it.b_i_curr = 0;
            it.b_curr = &hashTable[i];
            it.hashTablePtr = &hashTable;
            it.n = n;
            return it;
        }
        
    }
    
    return it;
}



class myHashTableExcpetion
{
    string s;
    
public:

    myHashTableExcpetion() : s("myHashTableExcpetion: Something unexpected happened!") 
        {}
        
    myHashTableExcpetion(string prms) : s("myHashTableExcpetion: " + prms) 
        {}
        
    const string what()
    {
        return s;
    }
    
};

#endif // MYHASHTABLE_H
