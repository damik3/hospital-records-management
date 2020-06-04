#ifndef MYHASHFUNC_H
#define MYHASHFUNC_H

#include <string> 
#include <cmath>

using namespace std;

class myHashFunc {

public:
    
    /**
     * @brief Computes hash of string s modulo n
     * @param s
     * @param n
     * @return unsigned int in the range of [0, n-1]
     */
    unsigned int operator () (const string &s, const unsigned int &n) {
        
        long sum = 0, mul = 1;
        
        for (unsigned int i = 0; i < s.length(); i++) {
            
            mul = (i % 4 == 0) ? 1 : mul * 256;
            sum += s[i] * mul;
        }
        
        return (unsigned int)(abs(sum) % n);
    }
    
    
    
    /**
     * @brief Computes hash of int i modulo n
     * @param i
     * @param n
     * @return unsigned int in the range of [0, n-1]
     */
    unsigned int operator() (const int &i, const unsigned int &n) {
        
        return (unsigned int )(abs(i) % n);
    }

};

#endif // MYHASHFUNC_H
