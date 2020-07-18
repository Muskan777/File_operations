#include<stdio.h>
#define PERMS 0666
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<errno.h>
#include"file.h" 

 
//myfopen opens the file in the required mode for he various operations to be performed on the file
// it even creates the file if required permisssion are given
myFILE *myfopen(char *filename, char *mode) {
	int fd;
	myFILE *fp;		//declaration of file pointer and descriptor
	
	//checking the validity of modes entered it should be either of "r" "w" "a" "w+" "r+" "a+"	
	if(*mode != 'r' && *mode != 'w' && *mode != 'a' && strcmp(mode, "r+") && strcmp(mode, "w+") && strcmp(mode, "a+")) 
		return NULL;
		
	//here, the maximum number of files to be opened simultaneously is MAX
	for(fp = file_no; fp < file_no + OPEN_MAX; fp++)
		if((fp -> myflag & (READ | WRITE)) == 0)
			break;		//found free slot
	if(fp >= file_no + OPEN_MAX)
		return NULL;		//no free slots
	
	//opening the file in read mode and reading data from it	
	if(*mode == 'r') {
		(fp -> myfd) = open(filename, O_RDONLY, 0);
		(fp -> myflag) = READ;
		lseek((fp -> myfd), 0L, 0);
	}
	
	//opening the file in "w" mode or creating it if it doesn't exist
	else if(*mode == 'w') {
		(fp -> myfd) = creat(filename, PERMS);
		(fp -> myflag) = WRITE;
		lseek((fp -> myfd), 0L, 0);
	}
	
	//opening the file in append mode to append the data at its end it creates the file if it doesn't exist
	else if(*mode == 'a') {
		if(((fp -> myfd) = open(filename, O_WRONLY, 0)) == -1)
			(fp -> myfd) = creat(filename, PERMS);
		(fp -> myflag) = WRITE; 
		lseek((fp -> myfd), 0L, 2);
	}
	
	//opening the file in "r+" mode i.e. read and write mode it doesn't create the file if it doesn't exist
	else if(!strcmp(mode,"r+")) {
		(fp -> myfd) = open(filename, O_RDWR, S_IRUSR | S_IWUSR);
		(fp -> myflag) = READ | WRITE;
		lseek((fp -> myfd), 0L, 0);
	}
	
	//opening the file in "w+" mode i.e. write + read mode it creates the file if it doesn't exist
	else if(!strcmp(mode,"w+")) {
		(fp -> myfd) = creat(filename, PERMS);
		(fp -> myfd) = open(filename, O_RDWR, 0);
		(fp -> myflag) = READ | WRITE;
		lseek((fp -> myfd), 0L, 0);
	}
	
	//opens the file in "a+" mode i.e. append + read mode it creates the file if it doesn't exist
	else if(!strcmp(mode,"a+")) {
		(fp -> myfd) = creat(filename, PERMS);
		(fp -> myfd) = open(filename, O_RDWR, 0);
		(fp -> myflag) = READ | WRITE;
		lseek((fp -> myfd), 0L, 2);
	}	
	
	//if the file can't be opened by any reason then fp returns null pointer and 
	//each member of structure is initialized to 0 and pointers to NULL 
	if((fp -> myfd) == -1)
		return NULL;
	(fp -> mycnt) = 0;
	(fp -> mybase) = NULL;
	(fp -> myptr) = NULL;
	return fp;	
}


//myfclose function closes the file and frees the malloced memory for buffer 
//it makes the file pointer as NULL and initializes each of the member of myFILE to 0
int myfclose(myFILE *fp) {
	if(fp == NULL) {
		return -1;		//if file isn't opened then fclose function fails
	}
	int fd = (fp -> myfd);
	free(fp -> mybase);
	fp -> myfd = 0;
	fp -> mycnt = 0;
	fp -> myptr = NULL;
	fp -> mybase = NULL;
	fp -> myflag = 0;  
	return close(fd);		//file is closed
}


//this function is used to fill the buffer by reading more than required amount of data
//so as to increase the time effieciency of myfread function by reducing the number of calls reuired for read function
int myfillbuf(myFILE *fp) {
	int bufsize;
	if(((fp -> myflag & (READ | ERR | Eof)) != READ) && (fp -> myflag & (READ | ERR | Eof) != (READ | WRITE)))
		return myEOF;		//checking the flags
	bufsize = (fp -> myflag & UNBUF) ? 1 : BUFSIZE;
	if(fp -> mybase == NULL) {
		if((fp -> mybase = (char *)malloc(bufsize)) == NULL)
			return myEOF;			//if malloc function doesn't get the memory then use unbuffered mode
		(fp -> myptr) = (fp -> mybase);
	}
	
	else {
		if(((char *)realloc((fp -> mybase), bufsize)) == NULL)
			return myEOF;		//if malloced memory is full and still data is to be read
	}
	
	(fp -> mycnt) = read((fp -> myfd), (fp -> myptr), bufsize);
	
	if(--fp -> mycnt < 0) {		
	
		if(fp -> mycnt == -1) {
			(fp -> myflag) |= Eof;		//end of file is reached
			(fp -> mycnt) = 0;
			return myEOF;
		}
	
		else 
			(fp -> myflag) |= ERR;
		(fp -> mycnt) = 0;				//error condition occurs
		return myEOF;
	}
	
	return (unsigned char) *(fp -> myptr);
}


//myfread function read the data from the file opened to buffer
int myfread(char *buf, int size, int nmemb, myFILE *fp) {
	int no_of_req_bytes = size * nmemb;
	
	if((fp -> mybase) == NULL)
		myfillbuf(fp);			//in this case firstly memory is allocated 
		
	if((fp -> myflag) & UNBUF) {
		int i = 0;				//in case if memory space can't be malloced then unbuffered reading takes place
	
		while(i != no_of_req_bytes) {
			read(fp -> myfd, &(fp -> mybase), 1);		
			buf[i] = *(fp -> mybase); 
			i++;
		}
	
		return nmemb;
	}
	if(no_of_req_bytes == (fp -> mycnt)) {
		memcpy(buf, fp -> myptr, no_of_req_bytes);
		(fp -> myptr) = (fp -> myptr) + no_of_req_bytes;		
		(fp -> mycnt) = (fp -> mycnt) - no_of_req_bytes;
	}
	
	else if(no_of_req_bytes < (fp -> mycnt)) {
		memcpy(buf, fp -> myptr, no_of_req_bytes);				//writing the number of bytes required  from buffer to character pointer
		(fp -> myptr) = (fp -> myptr) + no_of_req_bytes;
		(fp -> mycnt) = (fp -> mycnt) - no_of_req_bytes;
	}
	
	else {
		int i = no_of_req_bytes - (fp -> mycnt);
		memcpy(buf, (fp -> myptr), (fp -> mycnt));
		(fp -> myptr) = (fp -> myptr) + (fp -> mycnt);
		int count = (fp -> mycnt);
		(fp -> mycnt) = 0;
		myfillbuf(fp);
	
		if((fp -> mycnt) < i) {
			memcpy((buf + count), (fp -> myptr), (fp) -> mycnt);
			(fp -> myptr) = (fp -> myptr) + (fp -> mycnt);				
			(fp -> mycnt) = (fp -> mycnt) - (fp -> mycnt);
			return ((count + (fp -> mycnt))/size); 
		}
	
		else {
			memcpy((buf + count), (fp-> myptr), i);
			(fp -> myptr) = (fp -> myptr) + i;
			(fp -> mycnt) = (fp -> mycnt) - i;
		}
	}
	
	return nmemb;
}


//function flushbuf is used to write the data from buffer to file using write function
//it also checke the flags
int myflushbuf(int c, myFILE *fp) {

	if(fp -> myflag & (WRITE | ERR | Eof) != WRITE && (fp -> myflag & (WRITE | ERR | Eof) != READ | WRITE))
		return myEOF;
		
		write((fp -> myfd), (fp -> myptr), c);
		(fp -> myptr) = (fp -> myptr) + c; 
		return c;	
}


//myfwrite function writes more data than required from the buf to the malloced buffer so that calls for write function are reduced
//if memory space can't be malloced then it writs the data byte by byte in memory
int myfwrite(char *buf, int size, int nmemb, myFILE *fp) {
	int bufsize;
	
	int no_of_req_bytes = size * nmemb;
	
	if(fp -> mybase == NULL) {
	
		if(((fp -> mybase) = (char *)malloc(BUFSIZE)) == NULL) {		//mallocs the memory space 
			fp -> myflag |= UNBUF;
			return myEOF;
		} 
		
		fp -> mycnt = BUFSIZE;
		(fp -> myptr) = (fp -> mybase);
	}
	
	else if(((fp -> mybase) != NULL) && ((fp -> mycnt) < no_of_req_bytes)) {
	
		if((char *)realloc((fp -> mybase), BUFSIZE) == NULL) {			//it reallocs the memory if malloced memory is full
			fp -> myflag |= UNBUF;
			return myEOF;
		}
		
		fp -> mycnt = BUFSIZE;
	}
	
	bufsize = ((fp -> myflag) | UNBUF) ? 1 : BUFSIZE;		//flag checking
	
		if((fp -> myflag) & UNBUF) {	//if flag is set to UNBUF then unbuffered writing of data takes place
		int i = 0;
		
		while(i != no_of_req_bytes) {
			write(fp -> myfd, &(fp -> mybase), 1);
			buf[i] = *(fp -> mybase); 
			i++;
		}
		
		return nmemb;
	}
	
	if((fp -> mycnt) >= no_of_req_bytes) {
		memcpy((fp -> myptr), buf, no_of_req_bytes);
		(fp -> mycnt) = (fp -> mycnt) - no_of_req_bytes;
		(fp -> myptr) = (fp -> myptr) + no_of_req_bytes;
	}
	
	else if(no_of_req_bytes > (fp -> mycnt)) {
		int k = no_of_req_bytes;
		memcpy((fp -> myptr), buf, (fp -> mycnt));
		k = k - (fp -> mycnt);
		(fp -> myptr) = (fp -> myptr) + (fp -> mycnt);
		(fp -> mycnt) = 0;
		void *ptr = buf + (fp -> mycnt);
		
		while(k > BUFSIZE) {
		
			if((realloc(fp -> mybase, k)) == NULL)
				return myEOF;
				
			memcpy((fp -> myptr), ptr, BUFSIZE);
			ptr = ptr + BUFSIZE;
			(fp -> myptr) = (fp -> myptr) + BUFSIZE;
			k = k - BUFSIZE;
		}

		if((realloc(fp -> mybase, k)) == NULL)
			return myEOF;
			
		memcpy((fp -> myptr), ptr, k);
	}
	
	(fp -> myptr) = (fp -> myptr) - no_of_req_bytes;
	myflushbuf(no_of_req_bytes, fp);
	
	return nmemb;
}


//mygetpos function takes the position of file pointer wrt the beginnig of the file 
//on successful execution it returns 0
int myfgetpos(myFILE *fp, fpos *pos) {

	(pos -> p) = ((fp -> myptr) - (fp ->mybase));
	return 0;
	
}


//myfsetpos function uses the value of position as given by myfgetpos and then it 
//sets it position to that place and continues execution
//on successful execution it returns 0
int myfsetpos(myFILE *fp, fpos *pos) {

	myfseek(fp, -(pos -> p), 0);
	return 0;

}


//myfeof functions checks if the end of the file is reached or not
//if the end is reahed, it returns 1, else it returns 0

int myfeof(myFILE *fp) {

	int c = 1;
	char r;

	if(!(c = read(fp -> myfd, &r, 1))) 

		return 1;

	else {

		lseek(fp -> myfd, 0, SEEK_CUR);
		return 0;
	}	
}


//myftell function tells the current position of the file pointer with respect to the beginning
//on successful execution, it returns distance from the beginning

long myftell(myFILE *fp) {
	return ((fp -> myptr) - (fp -> mybase));
}


//this function seek a particular position in file 
//either the beginning or end or the current cursor position considering the required offset from that position
//it returns 0 on successful execution and -1 on failure
int myfseek(myFILE *fp, long offset, int whence) {

	if(whence == mySEEK_SET) {
	
		if(offset < 0) {
			return -1;
		}
		
		(fp -> myptr) = (fp -> mybase);
		fp -> myptr = fp -> myptr + offset;
		return 0; 
	}
	
	else if(whence == mySEEK_CUR) {
		fp -> myptr = fp -> myptr + offset;
		return 0;
	}
	
	else if(whence == mySEEK_END) {
	
		if(offset > 0) {
			return -1;
		}
		lseek(fp -> myfd, 0, SEEK_END);
		fp -> myptr = fp -> myptr + offset;
		return 0;
	}
	
	else {
		fp -> myflag = ERR;
		return -1;
	}
}


int main() {
	//TEST SUITE
	myFILE *fp1, *fp2, *fp3, *fp4, *fp5, *fp6;
	FILE *f1, *f2, *f3, *f4, *f5, *f6;
	
	fpos pos1, pos2, pos3, pos4,pos5;
	fpos_t p1,p2, p3, p4, p5;
	
	char a[10] = "abcdefhgij";
	char b[10] = "pqrstuvwxy";
	char c[10] = "lmnoplkabc";
	char d[20];
	
	//check for stdout
	//myfunctions
	
	printf("writing using stdout\n\n");
	printf("using myfwrite\n\n");
	myfwrite(a, 10, 1, mystdout);
	
	//built-in  stdout and fwrite
	
	printf("\n\nusing fwrite\n\n");
	fwrite(a, 10, 1, stdout);
	
	//check for stdin
	//myfunctions
	
	printf("\n\nreading using mystdin\n\n");
	int i = myfread(d, 1, 10, mystdin);
	
	//printing the data read by myfread
	
	printf("data read by myfread:");
	int j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	bzero(d, 10);
	
	printf("\n\nreading using stdin\n\n");
	i = fread(d, 1, 10, stdin);
	
	//prinitng data read by fread
	
	printf("data read by fread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}	
	
	printf("\n\nchecking myfread function");
	fp1 = myfopen("filer.txt", "r");
	
	//reading the data using myfread
	
	i = myfread(d, 1, 10, fp1);
	printf("\n\ndata read by myfread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	
	myfclose(fp1);
	
	bzero(d, 10);
	printf("\n\nchecking with fread function");
	
	//check opening in read mode with an already created file
	
	f1 = fopen("filer.txt", "r");
	
	//reading the data using myfread
	
	i = fread(d, 1, 10, f1);
	printf("\n\ndata read by fread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	
	fclose(f1);
	
	//WRITE MODE CHECK
	
	printf("\n\nchecking the W mode by my defined functions\n\n");
	
	//creating a file in "w" mode using myfwrite
	
	fp2 = myfopen("filew.txt", "w");
	
	//writing the data into the file
	
	myfwrite(a, 10, 1, fp2);
	
	myfclose(fp2);
	
	printf("Data successfully written in the file");	
	printf("\n\nchecking the W mode by built-in functions\n\n");
	
	//creating a file in "w" mode using fwrite
	
	f2 = fopen("FILEw.txt", "w");
	
	//writing the data into the file
	
	fwrite(a, 10, 1, f2);
	
	fclose(f2);
	
	printf("Data successfully written in the file\n\n");
		
		//APPEND MODE CHECK BY MYFILE FUNCTIONS
	//writing into a newly created file and closing it
	
	printf("Checking the append mode by my_functions");
	fp3 = myfopen("filea.txt", "w");
	myfwrite(a, 1, 10, fp3);
	printf("\n\nwriting in the newly created file\n\n");
	
	myfclose(fp3);
	
	//reopening the already created file in "a" mode
	
	fp3 = myfopen("filea.txt", "a");
	
	//appending the piece of data into it
	
	printf("Appending data in already created file\n\n");
	myfwrite(b, 1, 10, fp3);
	myfclose(fp3);
	printf("Data successfully appended by my functions\n\n");
	
	
	//APPEND MODE CHECK BY BUILT-IN FUNCTIONS
	//writing into a newly created file and closing it
	
	
	printf("Checking the append mode by my_functions");
	f3 = fopen("FILEa.txt", "w");
	fwrite(a, 1, 10, f3);
	printf("\n\nwriting in the newly created file\n\n");
	
	fclose(f3);
	
	//reopening the already created file in "a" mode
	
	f3 = fopen("FILEa.txt", "a");
	
	//appending the piece of data into it
	
	printf("Appending data in already created file\n\n");
	fwrite(b, 1, 10, f3);
	printf("Data successfully appended by built_in functions\n\n");
	
	fclose(f3);
	
		//WRITE + READ MODE CHECK
	
		//open a new file and write in it
		
	printf("Checking the W+ mode by my_functions\n\n");
	fp4 = myfopen("filew+.txt", "w+");
	myfwrite(c, 10, 1, fp4);
	
	//seek the beginnig and read the file in a string and print it 
	 
	myfseek(fp4, 0, 0);
	bzero(d, 20);
	i = myfread(d, 1, 10, fp4);
	printf("\n\ndata read by myfread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	
	myfclose(fp4);
	
	printf("\n\nData successfully read and write(W+) by my functions\n\n");
	
		//open a new file and write in it
		
	printf("Checking the W+ mode by built-in_functions\n\n");
	f4 = fopen("FILEw+.txt", "w+");
	fwrite(c, 1, 10, f4);
	
	//seek the beginnig and read the file in a string and print it 
	 
	fseek(f4, 0, 0);
	bzero(d, 20);
	i = fread(d, 1, 10, f4);
	printf("\n\ndata read by fread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	
	fclose(f4);
	
	printf("\n\nData successfully read and write (W+) by built-in functions\n\n");

		//READ + WRITE MODE CHECK
		
	printf("Checking the R+ mode by my_functions\n\n");
	bzero(d, 20);
	
	//open a new file and write in it
	
	fp5 = myfopen("filer+.txt", "r+");
	
	myfwrite(b, 1, 10, fp5);
	
	//seek the beginning and read the file in a string and print it 
	 
	myfseek(fp5, 0, 0);
	bzero(d, 20);
	i = myfread(d, 1, 10, fp5);
	printf("\n\ndata read by myfread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	
	myfclose(fp5);

	printf("\n\nData successfully read and write(R+) by my functions\n\n");

	//open a new file and write in it

	printf("Checking the R+ mode by my_functions\n\n");
	
	f5 = fopen("FILEr+.txt", "r+");
	
	fwrite(b, 1, 10, f5);
	
	//seek the beginnig and read the file in a string and print it  
	
	fseek(f5, 0, 0);
	
	bzero(d, 20);
	
	i = fread(d, 1, 10, f5);
	
	printf("\n\ndata read by fread:");
	
	j = 0;
	
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}
	fclose(f5);
	
	printf("\n\nData successfully read and write (R+) by built-in functions\n\n");

		//APPEND + READ MODE CHECK
	
	//open a new file and write in it
	
	printf("\n\nopening file by my_functions for writing\n\n");

	fp6 = myfopen("filea+.txt", "w");

	myfwrite(a, 1, 10, fp6);

	//close and open it in "a+" append data in it
	
	myfclose(fp6);

	myfopen("filea+.txt", "a+");

	printf("appending data in filea+");

	myfwrite(b, 1, 10, fp6);

	myfclose(fp6);
	
		//open a new file and write in it
		
	printf("\n\nopening file for writing built_in functions");
	
	f6 = fopen("FILEa+.txt", "w");
	
	fwrite(a, 1, 10, f6);

	//close and open it in "a+" append data in it
	
	fclose(f6);
		
	fopen("FILEa+.txt", "a+");
	
	printf("\n\nappending data in FILEa+\n\n");
	
	fwrite(b, 1, 10, f6);
	
	fclose(f6);
	
	//opening the file in "w" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'w' mode and checking for fsetpos and fgetpos operations\n\n");
	fp1 = myfopen("filew1", "w");
	myfwrite(a, 5, 1, fp1);
	myfwrite(b, 1, 6, fp1);
	myfwrite(c, 1, 8, fp1);
	
	myfgetpos(fp1, &pos1);			//getting location of pointer
	
	myfwrite(a, 5, 1, fp1);
	myfwrite(c, 1, 5, fp1);
	myfwrite(c, 1, 6, fp1);
	
	myfsetpos(fp1, &pos1);			//setting locaion to pos1
	
	myfwrite(c, 5, 1, fp1);
	myfwrite(c, 1, 5, fp1);
	myfclose(fp1);
	
	//opening the file in "w" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'w' mode and checking for fsetpos and fgetpos operations\n\n");
	
	f1 = fopen("FILEw2", "w");
	fwrite(a, 5, 1, f1);
	fwrite(b, 1, 6, f1);
	fwrite(c, 1, 8, f1);
	
	fgetpos(f1, &p1);			//getting location of pointer
	
	fwrite(a, 5, 1, f1);
	fwrite(c, 1, 5, f1);
	fwrite(c, 1, 6, f1);
	
	fsetpos(f1, &p1);			//setting location to p1
	
	fwrite(c, 5, 1, f1);
	fwrite(c, 1, 6, f1);
	fclose(f1);	
	
	//opening the file in "a" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'a' mode and checking for fsetpos and fgetpos operations\n\n");
	fp2 = myfopen("filea1", "a");
	myfwrite(a, 5, 1, fp2);
	myfwrite(b, 1, 6, fp2);
	myfwrite(c, 1, 8, fp2);
	
	myfgetpos(fp2, &pos2);			//getting location of pointer
	
	myfwrite(a, 5, 1, fp2);
	myfwrite(c, 1, 5, fp2);
	myfwrite(c, 1, 6, fp2);
	
	myfsetpos(fp2, &pos2);			//setting locaion to pos2
	
	myfwrite(c, 5, 1, fp2);
	myfwrite(c, 1, 5, fp2);
	myfclose(fp2);
	
	//opening the file in "a" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'a' mode and checking for fsetpos and fgetpos operations\n\n");
	
	f2 = fopen("FILEa2", "a");
	fwrite(a, 5, 1, f2);
	fwrite(b, 1, 6, f2);
	fwrite(c, 1, 8, f2);
	
	fgetpos(f2, &p2);			//getting location of pointer
	
	fwrite(a, 5, 1, f2);
	fwrite(c, 1, 5, f2);
	fwrite(c, 1, 6, f2);
	
	fsetpos(f2, &p2);			//setting location to p1
	
	fwrite(c, 5, 1, f2);
	fwrite(c, 1, 6, f2);
	fclose(f2);	
	
	//opening the file in "a" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'w+' mode and checking for fsetpos and fgetpos operations\n\n");
	fp3 = myfopen("filew+1", "w+");
	myfwrite(a, 5, 1, fp3);
	myfwrite(b, 1, 6, fp3);
	myfwrite(c, 1, 8, fp3);
	
	myfgetpos(fp3, &pos3);			//getting location of pointer
	
	myfwrite(a, 5, 1, fp3);
	myfwrite(c, 1, 5, fp3);
	myfwrite(c, 1, 6, fp3);
	
	myfsetpos(fp3, &pos3);			//setting locaion to pos2
	
	myfwrite(c, 5, 1, fp3);
	myfwrite(c, 1, 5, fp3);
	myfclose(fp3);
	
	//opening the file in "a" mode and checking for fsetpos and fgetpos operations

	printf("opening the file in 'w+' mode and checking for fsetpos and fgetpos operations\n\n");
	
	f3 = fopen("FILEw+2", "a");
	fwrite(a, 5, 1, f3);
	fwrite(b, 1, 6, f3);
	fwrite(c, 1, 8, f3);
	
	fgetpos(f3, &p3);			//getting location of pointer
	
	fwrite(a, 5, 1, f3);
	fwrite(c, 1, 5, f3);
	fwrite(c, 1, 6, f3);
	
	fsetpos(f3, &p3);			//setting location to p1
	
	fwrite(c, 5, 1, f3);
	fwrite(c, 1, 6, f3);
	
	fclose(f3);
	bzero(d, 20);
	//opening the file in "w" mode and checking for myfseek and myftell operation
	
	printf("opening the file in 'w' mode and checking forfseek operation\n\n");
	fp4 = myfopen("fileseekw", "w");
	myfwrite(a, 1, 10, fp4);
	
	myfseek(fp4, 0, 1);					//checking for myfseek at SEEK_CUR
	
	myfwrite(b, 1, 10, fp4);
	printf("Data written in fileseekw\n\n");
	
	printf("%ld\n\n", myftell(fp4));			//checking for myftell
	
	myfclose(fp4);

	//opening the file in "w" mode and checking for fseek and ftell function
	
	printf("opening the file in 'w' mode and checking forfseek operation\n\n");
	f4 = fopen("FILEseekw", "w");
	fwrite(a, 1, 10, f4);
	
	fseek(f4, 0, 1);				//checking for fseek at SEEK_CUR
	
	fwrite(b, 1, 10, f4);
	printf("Data written in in FILEseekw\n\n");
	printf("%ld\n\n", ftell(f4));				//checking for ftell 	

	fclose(f4);
	
	//opening the file in "w+" mode and checking for myfseek and myftell operation
	
	printf("opening the file in 'w+' mode and checking for fseek operation\n\n");
	fp5 = myfopen("fileseekw+", "w+");
	myfwrite(a, 1, 10, fp5);
	
	myfseek(fp5, 0, 0);					//checking for myfseek at SEEK_SET
	bzero(d, 20);
	i = myfread(d, 1, 10, fp5);
	printf("Data written in fileseekw\n\n");
	printf("data read by myfread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}

	printf("\n\n%ld\n\n", myftell(fp5));			//checking for myftell	

	myfclose(fp5);	
	
		
	//opening the file in "w+" mode and checking for myfseek and myftell operation
	
	printf("opening the file in 'w+' mode and checking for fseek operation\n\n");
	f5 = fopen("FILEseekw+", "w+");
	fwrite(a, 1, 10, f5);
	
	fseek(f5, 0, 0);					//checking for myfseek at SEEK_SET
	bzero(d, 20);
	i = fread(d, 1, 10, f5);
	printf("Data written in FILEseekw\n\n");
	printf("data read by myfread:");
	j = 0;
	while(i) {
		printf("%c", d[j]);
		j++;
		i--;
	}

	printf("\n\n%ld\n\n", ftell(f5));			//checking for myftell	

	fclose(f5);	
	
		//opening the file in "r" mode and checking for myfseek and myftell operation
		
	printf("opening the file in 'w' mode and checking for myfseek and myftell operation\n\n");
	fp5 = myfopen("fileseekw1", "w");
	myfwrite(a, 1, 10, fp5);
	
	myfseek(fp5, 0, 2);					//checking for myfseek at SEEK_END
	bzero(d, 20);
	i = myfwrite(b, 1, 10, fp5);
	printf("Data written in fileseekw\n\n");

	printf("\n\n%ld\n\n", myftell(fp5));			//checking for myftell	

	myfclose(fp5);	
		
	//opening the file in "w" mode and checking for myfseek and myftell operation
	
	printf("\n\nopening the file in 'w' mode and checking for fseek and operation\n\n");
	f5 = fopen("FILEseekw2", "w");
	fwrite(a, 1, 10, f5);
	
	fseek(f5, 0, 2);	
					//checking for myfseek at SEEK_END
	bzero(d, 20);
	i = fwrite(b, 1, 10, f5);
	printf("Data written in FILEseekw\n\n");

	printf("\n\n%ld\n\n", ftell(f5));			//checking for myftell	

	fclose(f5);		

	return 0;
}
