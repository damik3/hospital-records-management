#include "myLowLvlIO.h"




/*
 * Read up to count bytes from file descriptor fd into the buffer starting at arr.
 * While reading, split the message into segments of size bufsiz each 
 * (except maybe for the last segment).
 */
ssize_t read_data(int fd, char *arr, size_t count, size_t bufsiz)
{
	// Minimum bufsiz = sizeof(int)
	if (bufsiz < sizeof(int))
		bufsiz = sizeof(int);



	char *buff = (char *)malloc(bufsiz);
    if (buff == NULL)
    {
        return -1;
    }
    
	char *buff_ini = buff;
	ssize_t bread;				// bytes read
	size_t msgsiz = 0;			// message size
	int i;						// For debugging purposes
	
	
	
	// Read actual message size
	
	msgsiz = sizeof(int);
	
	while(msgsiz)
	{
		bread = read(fd, buff, msgsiz);
		
		if (bread <= 0)
			return bread;
			
		msgsiz -= bread;
		
		buff += bread;
	}
	
	buff = buff_ini;
	
	memcpy(&msgsiz, buff, sizeof(int));
		
	//printf("msgsiz %ld.", msgsiz);
	
	
	
	// Read actual message
	
	size_t msgsiz_ini = msgsiz;
	
	i = 0;
	
	while (msgsiz)
	{
		bread = read(fd, buff, (msgsiz < bufsiz ? msgsiz : bufsiz));
			
		if (bread <= 0)
			return bread;
				
		memcpy(arr, buff, bread);
		
		msgsiz -= bread;
		
		arr += bread;
		
		i++;
	}

	//printf(" iterations %d.\n", i);
	
	
	
	free(buff_ini);
	
	return msgsiz_ini;
}




/*
 * Write count bytes from buffer starting at arr into file descriptor fd.
 * While writing, split the message into segments of size bufsiz each 
 * (except maybe for the last segment).
 */
ssize_t write_data(int fd, char *arr, size_t count, size_t bufsiz)
{
	// Minimum bufsiz = sizeof(int)
	if (bufsiz < sizeof(int))
		bufsiz = sizeof(int);
		
		

	char *buff = (char *)malloc(bufsiz);
    if (buff == NULL)
    {
        return (-1);
    }       
	char *buff_ini = buff;
	size_t bwrit;
	size_t count_ini = count;
	int i;				// For debugging purposes
	
	
	
	// Send message size first
	
	memcpy(buff, &count_ini, sizeof(int));
	
	count = sizeof(int); 
	
	i=0;
	
	while (count)
	{
		bwrit = write(fd, buff, count);
		
		if (bwrit <= 0)
			return bwrit;
		
		count -= bwrit;
		
		buff += bwrit;
		
		i++;
	}
	
	buff = buff_ini;
	
	count = count_ini;
		
	//printf("iterations %d.\n", i);
	
	
		
	// Read actual data	
		
	i = 0;
		
	while (count)
	{
		memcpy(buff, arr, (count > bufsiz ? bufsiz : count));
		
		bwrit = write(fd, buff, (count > bufsiz ? bufsiz : count));
		
		if (bwrit <= 0)
			return bwrit;
		
		count -= bwrit;
		
		arr += bwrit;
	
		i++;
	}
	
	//printf("iterations %d.\n", i);
	
	
	
	free(buff_ini);
	
	return count_ini;
}