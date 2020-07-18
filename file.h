#define BUFSIZE		1024
#define OPEN_MAX	20	
#define myEOF 		-1

typedef struct myFILE {
	int myfd;        //file descriptor
	int mycnt;       //characters left
	char *myptr;     //next character position 
	char *mybase;    //location of buffer
	int myflag; 
	int count;     //mode of file access
}myFILE;

typedef struct fpos {
	int p;
}fpos;

myFILE file_no[OPEN_MAX];

#define mystdin		(&file_no[0])
#define mystdout	(&file_no[1])
#define mystderr	(&file_no[2])

enum _flags {
	READ = 01,
	WRITE = 02,
	UNBUF = 04,
	Eof = 010,
	ERR = 020
};

myFILE file_no[OPEN_MAX] ={		/* stdin, stdout, stderr:*/
	{0, 0, (char *)0, (char *)0, READ}, 
	{1, 0, (char *)0, (char *)0, WRITE}, 
	{2, 0, (char *)0, (char *)0, WRITE | UNBUF}
};

#define mySEEK_SET	0
#define mySEEK_CUR	1
#define mySEEK_END	2

myFILE *myfopen(char *filename, char *mode);

int myfread(char *, int size, int nmemb, myFILE *fp);

int myfwrite(char *, int size, int nmemb, myFILE *fp);

int myfclose(myFILE *fp);

int myfillbuf(myFILE *fp);

int myflushbuf(int c, myFILE *fp);

int myfgetpos(myFILE *fp, fpos *pos);

int myfsetpos(myFILE *fp, fpos *pos);

int myfeof(myFILE *fp);

long myftell(myFILE *fp);

int myfseek(myFILE *fp, long offset, int whence);
