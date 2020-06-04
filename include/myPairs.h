#ifndef MYPAIRS_H
#define MYPAIRS_H

template <class T1, class T2>
struct indexPair
{
    T1 first;
    T2 second;
};

template <class T1, class T2>
bool operator==(const indexPair<T1, T2>& lhs, const indexPair<T1, T2>& rhs)
{
    return lhs.first == rhs.first;
}

template <class T1, class T2>
bool operator != (const indexPair<T1, T2>& lhs, const indexPair<T1, T2>& rhs)
{
    return lhs.first != rhs.first;
}


template <class T1, class T2>
bool operator < (const indexPair<T1, T2>& lhs, const indexPair<T1, T2>& rhs)
{
    return lhs.first < rhs.first;
}


template <class T1, class T2>
bool operator <= (const indexPair<T1, T2>& lhs, const indexPair<T1, T2>& rhs)
{
    return lhs.first <= rhs.first;
}

template <class T1, class T2>
std::ostream& operator << (std::ostream& os, indexPair<T1, T2>& ipair)
{
    os << ipair.first << " " << ipair.second;
    return os;
}


template <class T1, class T2>
std::ostream& operator << (std::ostream& os, std::pair<T1, T2>& p)
{
    os << p.first << " " << p.second;
    return os;
}

#endif  // MYPAIRS_H