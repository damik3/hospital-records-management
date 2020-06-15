#include "sendReceive.h"



ssize_t send_null(int fd, unsigned int bufferSize)
{
    size_t count = sizeof(char);
    
    char c = '\0';
    
    return write_data(fd, &c, count, bufferSize);
}




ssize_t send_data(int fd, unsigned int bufferSize, myDate &date, string &en, ageGroup &group, string &country, string &disease)
{
    size_t count = 3*sizeof(int) + 3*sizeof(char) + NUMGRPS*sizeof(int) + country.length() + 1 + disease.length() + 1;
    
    char *buffer;
    buffer = (char *)malloc(count);
    
    // Keep initial address of buffer
    char *buffer_ini;
    buffer_ini = buffer;
    
    // First the date
    memcpy(buffer, &(date.day), sizeof(int));
    buffer += sizeof(int);
    memcpy(buffer, &(date.month), sizeof(int));
    buffer += sizeof(int);
    memcpy(buffer, &(date.year), sizeof(int));
    buffer += sizeof(int);
    
    // Then the EN or EX for entered or exited patients respectively
    memcpy(buffer, en.c_str(), 3*sizeof(char));
    buffer += 3*sizeof(char);
    
    int age;
    
    // Then the age group statistics
    for (int i=0; i<NUMGRPS; i++)
    {
        memcpy(buffer, &(age = group[i]), sizeof(int));
        buffer += sizeof(int);
    }
    
    // Then the country string
    memcpy(buffer, country.c_str(), country.length() + 1);
    buffer += country.length() + 1;
    
    // Then the disease string
    memcpy(buffer, disease.c_str(), disease.length() + 1);
    buffer += disease.length() + 1;
    
    ssize_t ret = write_data(fd, buffer_ini, count, bufferSize);
        
    free(buffer_ini);
    
    return ret;
}




ssize_t send_id(int fd, unsigned int bufferSize, string id)
{
    size_t count = (id.length() + 1)*sizeof(char);
    
    char buffer[count];
    
    memcpy(buffer, id.c_str(), count);
    
    return write_data(fd, buffer, count, (size_t)bufferSize);
}



ssize_t send_pat(int fd, unsigned int bufferSize, patient pat)
{
    size_t count = pat.id.length() + 1 
        + pat.fname.length() + 1
        + pat.lname.length() + 1
        + pat.disease.length() + 1
        + sizeof(int) 
        + 6*sizeof(int);

    char *buffer = (char *)malloc(count);
    
    char *buffer_ini = buffer;
    
    // First the id
    memcpy(buffer, pat.id.c_str(), pat.id.length() + 1 );
    buffer += pat.id.length() + 1 ;
    
    // Then the first name
    memcpy(buffer, pat.fname.c_str(), pat.fname.length() + 1);
    buffer += pat.fname.length() + 1;
    
    // Then the last name
    memcpy(buffer, pat.lname.c_str(), pat.lname.length() + 1);
    buffer += pat.lname.length() + 1;
    
    // Then the disease
    memcpy(buffer, pat.disease.c_str(), pat.disease.length() + 1);
    buffer += pat.disease.length() + 1;
    
    // Then the age
    memcpy(buffer, &(pat.age), sizeof(int));
    buffer += sizeof(int);
    
    // Then the entry date day
    memcpy(buffer, &(pat.entryDate.day), sizeof(int));
    buffer += sizeof(int);
    
    // Then the entry date month
    memcpy(buffer, &(pat.entryDate.month), sizeof(int));
    buffer += sizeof(int);
    
    // Then the entry date year
    memcpy(buffer, &(pat.entryDate.year), sizeof(int));
    buffer += sizeof(int);
    
    // Then the exit date day
    memcpy(buffer, &(pat.exitDate.day), sizeof(int));
    buffer += sizeof(int);
    
    // Then the exit date month
    memcpy(buffer, &(pat.exitDate.month), sizeof(int));
    buffer += sizeof(int);
    
    // Then the exit date year
    memcpy(buffer, &(pat.exitDate.year), sizeof(int));
    buffer += sizeof(int);
    
    ssize_t ret = write_data(fd, buffer_ini, count, bufferSize);
    
    free(buffer_ini);
    
    return ret;
}



ssize_t receive_data(int fd, unsigned int bufferSize, myDate &date, string &en, ageGroup &group, string &country, string &disease)
{
    // Total bytes to be read 
    size_t count = 3*sizeof(int) + 3*sizeof(char) + NUMGRPS*sizeof(int) + 63 + 1 + 63 + 1;
    
    // Max string length for country or disease is 63 + 1
    char s[64];
    
    char *buffer;
    buffer = (char *)malloc(count);
    
    // Keep initial address of buffer
    char *buffer_ini;
    buffer_ini = buffer;
    
    //cout << "before read_data with fd " << fd << endl;
    
    ssize_t ret = read_data(fd, buffer, count, bufferSize);
    
    if (ret <= sizeof(char))
    {
        free(buffer_ini);
        return ret;
    }
        
    
    //cout << "after read_data with ret " << ret << endl;
    
    // First the date
    memcpy(&(date.day), buffer, sizeof(int));
    buffer += sizeof(int);
    memcpy(&(date.month), buffer, sizeof(int));
    buffer += sizeof(int);
    memcpy(&(date.year), buffer, sizeof(int));
    buffer += sizeof(int);
    
    // Then the EN or EX for entered or exited patients respectively
    memcpy(s, buffer, 3*sizeof(char));
    buffer += 3;
    en = s;
    
    int age;
    
    // Then the age group statistics
    for (int i=0; i<NUMGRPS; i++)
    {
        memcpy(&age, buffer, sizeof(int));
        buffer += sizeof(int);
        group[i] = age;
    }
    group.setTotal();
    
    // Then the country string
    strcpy(s, buffer);
    country = s;
    buffer += country.length() + 1;
    
    // Then the disease string
    strcpy(s, buffer);
    disease = s;
    buffer += disease.length() + 1;
    
    free(buffer_ini);
    
    return ret;
}



ssize_t receive_id(int fd, unsigned int bufferSize, string& id)
{
    // Maximum length of id = 64
    char s[64];

    size_t count = 64*sizeof(char);

    ssize_t ret = read_data(fd, s, count, bufferSize);
    
    if (ret <= 0)
        return ret;
    
    id = s;
    
    return ret;
}




ssize_t receive_pat(int fd, unsigned int bufferSize, patient& pat)
{
    size_t count = 63 + 1 
        + 63 + 1
        + 63 + 1
        + 63 + 1
        + sizeof(int) 
        + 6*sizeof(int);

    char *buffer = (char *)malloc(count);
    
    char *buffer_ini = buffer;
    
    ssize_t ret = read_data(fd, buffer, count, bufferSize);
    
    if (ret <= sizeof(char))
    {
        free(buffer_ini);
        return ret;
    }
    
    char s[64];
    
    strcpy(s, buffer);
    pat.id = s;
    buffer += pat.id.length() + 1;
    
    strcpy(s, buffer);
    pat.fname = s;
    buffer += pat.fname.length() + 1;
    
    strcpy(s, buffer);
    pat.lname = s;
    buffer += pat.lname.length() + 1;
    
    strcpy(s, buffer);
    pat.disease = s;
    buffer += pat.disease.length() + 1;
    
    memcpy(&(pat.age), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.entryDate.day), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.entryDate.month), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.entryDate.year), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.exitDate.day), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.exitDate.month), buffer, sizeof(int));
    buffer += sizeof(int);
    
    memcpy(&(pat.exitDate.year), buffer, sizeof(int));
    buffer += sizeof(int);
    
    free(buffer_ini);
    
    return ret;
}

