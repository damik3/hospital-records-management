#include "myDate.h"

#include <iostream>

using namespace std;



myDate::myDate() {
    
    this->day = MYDATE_NULL;
    this->month = MYDATE_NULL;
    this->year = MYDATE_NULL;
}



myDate::myDate(int day, int month, int year) {
    
    this->day = day;
    this->month = month;
    this->year = year;
}



myDate::myDate(string s)
{
    if (s == "-")
    {
        setNull();
    }
    else
    {
        char dash;
        stringstream ss;
        ss.str(s);
        ss >> day >> dash >> month >> dash >> year;
    }
}



void myDate::setNull() {
        
    this->day = MYDATE_NULL;
    this->month = MYDATE_NULL;
    this->year = MYDATE_NULL;
}



bool myDate::isNull() {
    
    return ( (this->day == MYDATE_NULL) && (this->month == MYDATE_NULL) && (this->year == MYDATE_NULL) );
}



bool operator == (const myDate &d1, const myDate &d2) {
    
    return ( (d1.day == d2.day) && (d1.month == d2.month) && (d1.year == d2.year) ); 
}



bool operator != (const myDate &d1, const myDate &d2) {
    
    return !(d1 == d2);
}



bool operator < (const myDate &d1, const myDate &d2) {
    
    if (d1.year < d2.year) 
        return true;
    else if ( (d1.year == d2.year) && (d1.month < d2.month) )
        return true;
    else if ( (d1.year == d2.year) && (d1.month == d2.month) && (d1.day < d2.day) )
        return true;
    else
        return false;
}



bool operator <= (const myDate &d1, const myDate &d2) {
    
    return ( (d1 < d2) || (d1 == d2) );
}



string myDate::toString()
{
    string s = "";
    
    #ifdef MYDATETYPE1
    
    s += (day < 10 ? "0" : "");
    s += std::to_string(day);
    s += "-";
    s += (month < 10 ? "0" : "");
    s += std::to_string(month);
    s += "-";
    s += std::to_string(year);

    #endif
    
    #ifdef MYDATETYPE2
    
    s += std::to_string(day);
    s += "-";
    s += std::to_string(month);
    s += "-";
    s += std::to_string(year);
    
    #endif
    
    return s;
}



ostream& operator << (ostream &os, myDate& d) {
    
    if (d.isNull())
    {
        os << "-";
    }
    else 
    {
        os << d.toString();
    }
        
    return os;
}



istream& operator >> (istream &is, myDate& d) {

    string s;
    
    is >> s;
    
    if (s == "-")
    {
        d.setNull();
    }
    else
    {
        char dash;
        stringstream ss;
        ss.str(s);
        ss >> d.day >> dash >> d.month >> dash >> d.year;
    }
    
    return is;
}