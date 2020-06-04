#ifndef MYSTACK_H
#define MYSTACK_H

#include <iostream>



template <class T>
class myStack 
{
    
    
    
    struct node {
     
        T data;
        node* next;
            
            
            
        node(T prmdata, node *prmnext = NULL)
            : data(prmdata),
            next(prmnext)
            {}
        
        
        
        void print(std::ostream& os) 
        {    
            os << this->data << std::endl;
            
            if (this->next)
                this->next->print(os);
        }
        
    };
    
    
    
    node *top;
    int count;

public:
    myStack();
    myStack(myStack<T> &s2);
    ~myStack();
    bool empty();
    unsigned int getCount();
    void push(const T& obj);
    T pop();
    void print(std::ostream &os);
    myStack& operator= (const myStack& rhs);
};



template<class T>
myStack<T>::myStack()
    : top(NULL), 
    count(0)
    {}
    
    

template<class T>
myStack<T>::myStack(myStack<T> &s2)
{
    top = NULL;
    count = 0;
    *this = s2;
}


    
template<class T>
myStack<T>::~myStack()
{
    node *tmp;
    
    while (top)
    {
        tmp = top;
        top = top->next;
        delete tmp;
    }
}



template<class T>
bool myStack<T>::empty()
{
    return count == 0;
}



template<class T>
unsigned int myStack<T>::getCount()
{
    return count;
}



template<class T>
void myStack<T>::push(const T &obj)
{
    if (top)
        top = new node(obj, top);
    else
        top = new node(obj);
        
    count++;
}



template<class T>
T myStack<T>::pop()
{
    T ret;
    
    if (top)
    {
        ret = top->data;
        node* tmp = top;
        top = top->next;
        delete tmp;
        count--;
    }
    
    return ret;
}


template<class T>
void myStack<T>::print(std::ostream &os)
{
    if (top)
        top->print(os);
}



template<class T>
inline std::ostream& operator << (std::ostream& os, myStack<T> &st)
{
    st.print(os);
    return os;
} 



template <class T>
myStack<T>& myStack<T>::operator= (const myStack& rhs)
{
    if (this == &rhs)
        return *this;
        
        
    
    // Delete current stack
    
    node *tmp;
    
    while (top)
    {
        tmp = top;
        top = top->next;
        delete tmp;
    }
    
    top = NULL;
    count = 0;
    

    
    // If rhs empty, return empty stack
    if (rhs.count == 0)
        return *this;
    
    // If rhs not empty, copy its top element
    else
        top = new node(rhs.top->data);
        
        
    
    node *rhscurr = rhs.top->next;
    node *thiscurr = top;
    
    // And then copy its remaining elements
    while (rhscurr)
    {
        thiscurr->next = new node(rhscurr->data);
        thiscurr = thiscurr->next;
        rhscurr = rhscurr->next;
    }
    
    // Set count 
    count = rhs.count;
    
    // And return
    return *this;
}


#endif  // MYSTACK_H
