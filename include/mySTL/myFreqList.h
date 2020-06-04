#ifndef MYFREQLIST_H
#define MYFREQLIST_H

#include <iostream>

#include "myFreqPair.h"



template <class T>
class myFreqList
{
    
    
    struct node 
    {
        freqPair<T> fpair;
        node *next;
        
        
        
        node (T &obj) : fpair(obj),
            next(NULL)
        {}
            
        
        
        ~node()
        {
            if (next)
                delete next;
        }
        
        
        
        void print(ostream &os)
        {
            cout << "Data: " << fpair.data << ", count: " << fpair.count << endl;
            
            if (next)
                next->print(os);
        }
        
        
        
        void insert(T &obj)
        {
            if (fpair.data == obj)
                fpair.count++;
            else if (next)
            {
                next->insert(obj);
            }
            else 
            {
                next = new node(obj);
            }
        }
        
    };
    
    
    
    node *head;
    
public:


    
    struct iterator
    {
        node *curr;
        
        
        
        iterator() : curr(NULL) 
        {}
        
        
        
        bool isValid()
        {
            return curr != NULL;
        }
        
        
        
        void operator++ ()
        {
            if (curr)
                curr = curr->next;
        }
        
        
        
        freqPair<T>* operator-> ()
        {
            if (curr)
                return &(curr->fpair);
            else
                throw ("myFreqList: Invalid iterator!");
        }
        
        
        
        freqPair<T>& operator* ()
        {
            if (curr)
                return curr->fpair;
            else
                throw ("myFreqList: Invalid iterator!");
        }
        
    };

    myFreqList();
    ~myFreqList();
    void print(ostream &os);
    void insert(T &obj);
    iterator begin();
};



template <class T>
myFreqList<T>::myFreqList() : head(NULL)
{}



template <class T>
myFreqList<T>::~myFreqList()
{
    if (head)
        delete head;
}



template <class T>
void myFreqList<T>::print(ostream &os)
{
    if (head)
        head->print(os);
}



template <class T>
void myFreqList<T>::insert(T &obj)
{
    if (head)
        head->insert(obj);
    else
        head = new node(obj);
}



template <class T>
typename myFreqList<T>::iterator myFreqList<T>::begin()
{
    iterator it;
    it.curr = head;
    return it;
}



template <class T>
inline ostream& operator << (ostream& os, myFreqList<T> &fl) 
{
    fl.print(os);
    return os;
}

#endif // MYFREQLIST_H
