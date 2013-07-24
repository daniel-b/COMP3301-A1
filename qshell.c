#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* Function Prototypes */
int printprompt(void);
int getcommand(void);

int main(void)
{
	/*printf("Test");*/
	printprompt();
	getcommand();
	return 0;
}

int printprompt(void)
{
	char cwd[1024];
	if(getcwd(cwd, sizeof(cwd)) != NULL)
	{
		printf("%s?", cwd);
		return 0;
	}
	else
		printf("getcwd() error?");
	return -1;
}

/*char*[] getcommand(void)*/
int getcommand(void)
{
	int bytes_read;
	int no_bytes = 128;/*maximum number of characters that will be read(can type more)*/
	char *command;
	
	/*allocate memory for string. + 1 for null char*/
	command = (char *) malloc (no_bytes + 1);
	bytes_read = getline(&command, &no_bytes, stdin);
	
	if(bytes_read > 128)
		printf("bytes_read > 128- No Read: %d\n", bytes_read);
	else if(bytes_read < 1)
		printf("bytes_read < 1- No Read: %d\n", bytes_read);
	else
		printf("bytes_read: %d\n", bytes_read);
	return 0;
}
