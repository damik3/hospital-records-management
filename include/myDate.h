#ifndef MYDATE_H
#define MYDATE_H

#include <iostream>
#include <string>
#include <sstream>

#define MYDATE_NULL 0

//#define MYDATETYPE1        // For dates in format dd-mm-yyyy
#define MYDATETYPE2      // For dates in format (d)d-(m)m-yyyy

using namespace std;

struct myDate {

    int day;
    int month;
    int year;

    myDate();
    myDate(int day, int month, int year);
    myDate(string s);       // string s in the form of (d)d-(m)m-yyyy
    
    void setNull();
    bool isNull();
    
    friend bool operator == (const myDate &d1, const myDate &d2);
    friend bool operator != (const myDate &d1, const myDate &d2);
    friend bool operator < (const myDate &d1, const myDate &d2);
    friend bool operator <= (const myDate &d1, const myDate &d2);
    
    string toString();
};

ostream& operator << (ostream &os, myDate& d);
istream& operator >> (istream &is, myDate& d);

#endif // MYDATE_H