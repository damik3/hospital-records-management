#ifndef FREQPAIR_H
#define FREQPAIR_H



template <class T>
struct freqPair
{
    T data;
    unsigned int count;
    
    
    
    freqPair()
    {}
    
    
    
    freqPair(T &obj) : data(obj),
        count(1)
    {}
};



template <class T>
bool operator< (freqPair<T> fp1, freqPair<T> fp2)
{
    return fp1.count < fp2.count;
}



template <class T>
bool operator== (freqPair<T> fp1, freqPair<T> fp2)
{
    return fp1.count == fp2.count;
}



template <class T>
bool operator<= (freqPair<T> fp1, freqPair<T> fp2)
{
    return fp1.count <= fp2.count;
}



template <class T>
ostream& operator<< (ostream& os, freqPair<T> fp)
{
    os << fp.data << " " << fp.count;
    return os;
}


#endif  // FREQPAIR_H