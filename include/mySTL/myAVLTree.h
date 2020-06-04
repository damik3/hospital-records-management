#ifndef MYAVLTREE_H
#define MYAVLTREE_H

#include "myStack.hpp"

#include <iostream>

using namespace std;



template <class T>
class myAVLTree {
    
    
    
    struct node {
        
        T data;
        node* left;
        node* right;
        int height;
        
        
        
        /*** Generic functions ***/
        
        
        
        node(T obj) : data(obj),
            left(NULL),
            right(NULL),
            height(0) 
            {}
            
          
  
        ~node() {
            
            if (this->left)
                delete this->left;
            
            if (this->right)
                delete this->right;
        }
        
        
        
        void setHeight() {
            
            if ( (this->right == NULL) && (this->left == NULL) )
                this->height = 0;
            else {
            
                int leftHeight = ( this->left ? this->left->height : 0 );
                int rightHeight = ( this->right ? this->right->height : 0 );
                this->height =  max(leftHeight, rightHeight) + 1;
            
            }
        }
        
        
        
        int getBalance() {
            
            return (this->right ? this->right->height : -1) - (this->left ? this->left->height : -1);
        }
        
        
        
        void insert (T& obj) {
            
            // Perform usual binary tree insertion
            
            if (obj < this->data){
                
                if (this->left == NULL) 
                    this->left = new node(obj);
                else 
                    this->left->insert(obj);
            }
            
            else {
                
                if (this->right == NULL) 
                    this->right = new node(obj);
                else 
                    this->right->insert(obj);
            }
            
            this->setHeight();
            
            // Check for possible rotation going up the tree 
            
            int balance = this->getBalance();
            
            // Left left case
            if ( (balance < -1) && (obj < this->left->data) )
                this->rightRotate();
            
            // Left right case
            else if ( (balance < -1) && (this->left->data < obj) ) {
                
                this->left->leftRotate();
                this->rightRotate();
            }
            
            // Right left case
            else if ( (1 < balance) && (obj < this->right->data) ) {
                
                this->right->rightRotate();
                this->leftRotate();
            }
            
            // Right right case
            else if ( (1 < balance) && (this->right->data < obj) )
                this->leftRotate();
                
        }
        
        
        
        void print(ostream& os) {
            
            if (this->left)
                this->left->print(os);
                
            //os << "Data: " << this->data << ", height: " << this->height << ", balance: " << this->getBalance() << endl;
            
            os << this->data << endl;
            
            if (this->right)
                this->right->print(os);
        }
        
        
        
        void rightRotate() {
            
            // Pointer magic
            
            swap(data, left->data);
            
            node *rtemp = this->right;
            
            this->right = this->left;
            
            this->left = this->right->left;
            
            this->right->left = this->right->right;
            
            this->right->right = rtemp;
            
            // Set appropriate heights
            
            this->right->setHeight();
            
            this->setHeight();
        }
        
        
        
        void leftRotate() {
            
            // Pointer magic
            
            swap(data, right->data);
            
            node *ltemp = this->left;
            
            this->left = this->right;
            
            this->right = this->left->right;
            
            this->left->right = this->left->left;
            
            this->left->left = ltemp;
            
            // Set appropriate heights
            
            this->left->setHeight();
            
            this->setHeight();
        }
        
        
        
        T* exists(T &obj)
        {
            T* ret = NULL;
            
            if (data == obj)
                ret = &data;
            else
            {
                if (left && (ret = left->exists(obj)))
                    return ret;
                if (right && (ret = right->exists(obj)))
                    return ret;
            }
            
            return ret;
        }
        
    };
    
    
    
    /* Private members */
    
    node* root;
    unsigned int count;
    
public:


    
    struct iterator
    {
        node *curr;
        myStack<node*> nodeStack;
        
        
        
        iterator() 
            : curr(NULL)
        {}
        
        
        
        bool isValid()
        {
            return curr != NULL;
        }
        
        
        
        void operator++ ()
        {
            // If it.curr == NULL, I can't magically produce the next element 
            if (!isValid())
                return;
                
            // If nodeStack is empty, check if there is a curr->right not yet into stack
            if (nodeStack.empty())
            {
                // If there is, push it in
                if (curr->right)
                    nodeStack.push(curr->right);
                // Else, if stack is empty and there is no curr->right, we're done
                else {
                    curr = NULL;
                    return;
                }
            }
            
            // Self explanatory command is self explanatory
            curr = nodeStack.pop();
            
            // If curr->right exists, push dat in and all its lefts
            if (curr->right)
            {
                // Dat. In.
                nodeStack.push(curr->right);
                
                node *tmp = curr->right;
                
                // And all its lefts I said
                while (tmp->left)
                {
                    nodeStack.push(tmp->left);
                    tmp = tmp->left;
                }    
            }
        }
        
        
        
        T* operator-> ()
        {
            if (curr)
                return &(curr->data);
            else
                throw ("myAVLTree: Invalid iterator!");
        }
        
        
        
        T& operator* ()
        {
            if (curr)
                return curr->data;
            else
                throw ("myAVLTree: Invalid iterator!");
        }
        
    };
    
    

    /*** Generic functions ***/
    
    myAVLTree();
    ~myAVLTree();
    bool empty();
    unsigned int getCount();
    void insert(T& obj);
    T* exists(T& obj);
    void print(ostream& os);
    iterator begin();
};



/*** Generic functions ***/



template <class T>
myAVLTree<T>::myAVLTree() {
    
    this->root = NULL;
    this->count = 0;
}



template <class T>
myAVLTree<T>::~myAVLTree() {
    
    if (this->root)
        delete this->root;
}



template <class T>
bool myAVLTree<T>::empty()
{
    return count == 0;
}



template <class T>
unsigned int myAVLTree<T>::getCount()
{
    return count;
}


template <class T>
void myAVLTree<T>::insert(T& obj) {
    
    // If tree is empty
    if (this->root == NULL) 
        this->root = new node(obj);
        
    // If tree is not empty
    else 
        this->root->insert(obj);
        
    this->count++;
}



template <class T>
void myAVLTree<T>::print(ostream& os) {
    
    if (this->root)
        this->root->print(os);

    //os << "Count: " << this->count << endl;
}



template <class T>
inline ostream& operator << (ostream& os, myAVLTree<T> &t) {
    
    t.print(os);
    return os;
}



template <class T>
T* myAVLTree<T>::exists(T& obj)
{
    if (root)
        return root->exists(obj);
    else
        return NULL;
}



template <class T>
typename myAVLTree<T>::iterator myAVLTree<T>::begin()
{
    iterator it;
    
    if (root == NULL)
        return it;
    
    it.curr = root;
    
    // Go as far left as possibru
    
    while (it.curr->left)
    {
        it.nodeStack.push(it.curr);
        it.curr = it.curr->left;
    }
    
    // And thus, return time has come
    
    return it;
}

#endif // MYAVLTREE_H
