#ifndef MYPRIORITYQUEUE_H
#define MYPRIORITYQUEUE_H

#include <iostream>
#include <cmath>

using namespace std;

template <class T>
class myPriorityQueue
{
    
    
    struct node 
    {
        T data;
        node *left;
        node *right;
        unsigned int height;
        
        
        
        node(T obj) : data(obj),
            left(NULL),
            right(NULL),
            height(0) 
            {}
            
          
  
        ~node() 
        {    
            if (this->left)
                delete this->left;
            
            if (this->right)
                delete this->right;
        }
        
        
        
        void setHeight() 
        {    
            if ( (this->right == NULL) && (this->left == NULL) )
                this->height = 0;
            else 
            {
                int leftHeight = ( this->left ? this->left->height : 0 );
                int rightHeight = ( this->right ? this->right->height : 0 );
                this->height =  max(leftHeight, rightHeight) + 1;
            
            }
        }
        
        
        
        void print(ostream& os) 
        {    
            if (this->left)
                this->left->print(os);
                
            os << "Data: " << this->data << ", height: " << this->height << endl;
            
            if (this->right)
                this->right->print(os);
        }
        
        
        
        void push(T &obj, int count)
        {            
            node *nodeToCompare = NULL;
                        
            // Not supposed to happen
            if (count < 0)
            {
                cerr << "<myPriorityQueue: Something went wrong!" << endl;
                return;
            }
            
            // Father of 0 - Insert
            else if (count == 1)
            {
                left = new node(obj);
                nodeToCompare = left;
            }
            
            // Father of 1 - Insert
            else if (count == 2)
            {
                right = new node(obj);
                nodeToCompare = right;
            }
            
            // Last row is not full and must go left
            else if (count < pow(2, height) + pow(2, height-1) - 1)
            {
                left->push(obj, count - pow(2, height-1));
                nodeToCompare = left;
            }
            
            // Last row is not full and must go right
            else if (count < pow(2, height+1) - 1)
            {
                right->push(obj, count - pow(2, height));
                nodeToCompare = right;
            }
            
            // Last row is full and must go left
            else if (count == pow(2, height+1) - 1)
            {
                left->push(obj, pow(2, height) - 1);
                nodeToCompare = left;
            }
            
            // Not supposed to happen
            else 
            {
                cerr << "myPriorityQueue: Something went wrong!" << endl;
                return;
            }
            
            // Set height
            this->setHeight();
            
            // Compare and swap if needed
            if (data < nodeToCompare->data)
                swap(data, nodeToCompare->data);
                
        }
        
        
        
        T getFarthestRight(int count)
        {   
            T ret;
            
            // Not supposed to happen
            if (count <= 1)
            {
                cerr << "myPriorityQueue: Something went wrong!" << endl;
            }
            // Case: we arrived at father node
            else if (count == 2)
            {
                ret = left->data;
                delete left;
                left = NULL;
            }
            // Case: we arrived at father node
            else if (count == 3)
            {
                ret = right->data;
                delete right;
                right = NULL;
            }
            // Case: go left
            else if (count <= pow(2, height) + pow(2, height-1) - 1)
            {
                ret = left->getFarthestRight(count - pow(2, height-1));
            }
            // Case: go right
            else if (count <= pow(2, height+1) - 1)
            {
                ret = right->getFarthestRight(count - pow(2, height));
            }
            // Not supposed to happen
            else
            {
                cerr << "myPriorityQueue: Something went wrong!" << endl;
            }
            
            setHeight();
            
            return ret;
        }
        
        
        
        void heapify()
        {
            // Case: Father of none
            if ( (left == NULL) && (right == NULL) )
            {
                return;
            }
            
            // Case: Father of left 
            else if ( (left) && (right == NULL) )
            {
                if (data < left->data)
                {
                    swap(data, left->data);
                    left->heapify();
                }
            }
            
            // Case: Father of right
            else if ( (left == NULL) && (right) )
            {
                if (data < right->data){
                    swap(data, right->data);
                    right->heapify();
                }
            }
            
            // Case: Father of left and right
            else
            {
                if (data < max(left->data, right->data))
                {
                    if (left->data < right->data)
                    {
                        swap(data, right->data);
                        right->heapify();
                    }
                    else
                    {
                        swap(data, left->data);
                        left->heapify();
                    }
                }
            }
        }
        
    };
    
    
    
    node* root;
    unsigned int count;
    
public:
    myPriorityQueue();
    ~myPriorityQueue();
    bool empty();
    void push(T& obj);
    T pop();
    void print(ostream &os);
};



template <class T>
myPriorityQueue<T>::myPriorityQueue()
{
    root = NULL;
    count = 0;
}



template <class T>
myPriorityQueue<T>::~myPriorityQueue()
{
    if (root)
        delete root;
}



template <class T>
bool myPriorityQueue<T>::empty()
{
    return count == 0;
}


template <class T>
void myPriorityQueue<T>::push(T &obj) 
{
    if (root)
        root->push(obj, count);
    else
        root = new node(obj);
        
    count++;
}



template <class T>
T myPriorityQueue<T>::pop()
{
    T ret;
    
    // Not supposed to happen
    if (count < 0)
    {
        cerr << "myPriorityQueue: Something went wrong!" << endl;
    }
    // Heap is empty, do nothing
    else if (count == 0)
    {
        
    }
    // Only root exists
    else if (count == 1)
    {
        ret = root->data;
        root = NULL;
        count--;
    }
    // More than 1 nodes
    else 
    {
        ret = root->data;
        root->data = root->getFarthestRight(count);
        count--;
        root->heapify();
    }
    
    return ret;
}



template <class T>
void myPriorityQueue<T>::print(ostream &os)
{
    if (root)
        root->print(os);
}

#endif // MYPRIORITYQUEUE_H
