#include "ageGroup.h"

ageGroup::ageGroup()
{
    for (int i=0; i<NUMGRPS; i++)
        group[i] = 0;
    total = 0;
}

bool ageGroup::ingroup(int g, int a)
{
    switch (g)
    {
        case 0: 
            return (0 <= a) && (a <= 20);
            
        case 1:
            return (21 <= a) && (a <= 40);
            
        case 2:
            return (41 <= a) && (a <= 60);
            
        case 3:
            return (61 <= a);
            
        default:
            return false;
    }
}

void ageGroup::insert(int a)
{
    for (int g=0; g<NUMGRPS; g++)
        if (ingroup(g, a))
        {
            group[g]++;
            total++;
        }
}

int& ageGroup::operator [](int g)
{
    if ((g < 0) || (NUMGRPS <= g))
        throw "Not supposed to happen";
    else
        return group[g];
}

int ageGroup::getTotal()
{
    return total;
}

void ageGroup::setTotal()
{
    total = 0;
    for (int i=0; i<NUMGRPS; i++)
        total += group[i];
}

void ageGroup::printTopk(std::ostream& os, int k)
{
    if (k <= 0)
        return;
    
    if (NUMGRPS < k)
        k = NUMGRPS;
    
    indexPair<int, int> ipair;
    
    myPriorityQueue<indexPair<int, int> > qu;
    
    for (int i=0; i<NUMGRPS; i++)
    {
        ipair.first = group[i];
        ipair.second = i;
        qu.push(ipair);
    }
    
    for (int i=0; i<k; i++)
        printGroup(os, qu.pop().second);
}

void ageGroup::printGroup(std::ostream& os, int g)
{
    switch (g)
    {
        case 0: 
            os << "0-20: " << group[g] << std::endl;
            break;
            
        case 1:
            os << "20-40: " << group[g] << std::endl;
            break;
            
        case 2:
            os << "40-60: " << group[g] << std::endl;
            break;
        
        case 3:
            os << "60+: " << group[g] << std::endl;
            break;
            
        default:
            break;
    }
}


void ageGroup::print(std::ostream& os)
{
    for (int i=0; i<NUMGRPS; i++)
        printGroup(os, i);
    os << "Total: " << total << std::endl;
}

std::ostream& operator << (std::ostream& os, ageGroup g)
{
    g.print(os);
    return os;
}

ageGroup operator+ (ageGroup& lhs, ageGroup& rhs)
{
    ageGroup ag;
    
    for (int i=0; i<NUMGRPS; i++)
        ag[i] = lhs[i] + rhs[i];
        
    ag.setTotal();
    
    return ag;
}