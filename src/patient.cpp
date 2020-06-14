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
            
string patient::str()
{
    string s("");
    s += id;
    s += " ";
    s += fname;
    s += " ";
    s += lname;
    s += " ";
    s += disease;
    s += " ";
    s += to_string(age);
    s += " ";
    s += to_string(entryDate.day);
    s += "-";
    s += to_string(entryDate.month);
    s += "-";
    s += to_string(entryDate.year);
    if (exitDate.isNull())
    {
        s += " ";
        s += "-";
    }
    
    else
    {
        s += " ";
        s += to_string(exitDate.day);
        s += "-";
        s += to_string(exitDate.month);
        s += "-";
        s += to_string(exitDate.year);
    }
    
    return s;
}
            
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
