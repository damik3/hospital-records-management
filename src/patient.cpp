#include "patient.h"

patient::patient()
    {}
    
patient::patient(string prmid,
        string prmfname,
        string prmlname,
        string prmdisease,
        int prmage,
        myDate prmentryDate)
            : id(prmid), 
            fname(prmfname), 
            lname(prmlname),
            disease(prmdisease),
            age(prmage),
            entryDate(prmentryDate),
            exitDate()
            {}
            
patient::patient(string prmid,
        string prmfname,
        string prmlname,
        string prmdisease,
        int prmage,
        myDate prmentryDate,
        myDate prmexitDate)
            : id(prmid), 
            fname(prmfname), 
            lname(prmlname),
            disease(prmdisease),
            age(prmage),
            entryDate(prmentryDate),
            exitDate(prmexitDate)
            {}
            
bool operator == (const patient& p1, const patient&p2)
{
    return p1.id == p2.id;
}

bool operator != (const patient& p1, const patient&p2)
{
    return p1.id != p2.id;
}

bool operator < (const patient& p1, const patient&p2)
{
    return p1.id < p2.id;
}

bool operator <= (const patient& p1, const patient&p2)
{
    return p1.id <= p2.id;
}

ostream& operator << (ostream &os, patient& p)
{
    cout << p.id << " "
        << p.fname << " "
        << p.lname << " "
        << p.disease << " "
        << p.age << " "
        << p.entryDate << " "
        << p.exitDate;
        
    return os;
}
