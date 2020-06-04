#ifndef PATIENT_H
#define PATIENT_H

#include <iostream>
#include <string>

#include "myDate.h"


struct patient
{ 
    string id;
    string fname;
    string lname;
    string disease;
    int age;
    myDate entryDate;
    myDate exitDate;
    
    patient();
    
    // without exit date
    patient(string prmid,
        string prmfname,
        string prmlname,
        string prmdisease,
        int prmage,
        myDate prmentryDate);
        
    // with exit date
    patient(string prmid,
        string prmfname,
        string prmlname,
        string prmdisease,
        int prmage,
        myDate prmentryDate,
        myDate prmexitDate);
        
    friend bool operator == (const patient& p1, const patient&p2);
    friend bool operator != (const patient& p1, const patient&p2);
    friend bool operator < (const patient& p1, const patient&p2);
    friend bool operator <= (const patient& p1, const patient&p2);
};

ostream& operator << (ostream &os, patient& r);

#endif  // PATIENT_H
