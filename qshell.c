#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* Function Prototypes */
int printprompt(void);

int main(void)
{
	//printf("Test");
	printprompt();
	return 0;
}

int printprompt(void)
{
	char cwd[1024];
	if(getcwd(cwd, sizeof(cwd)) != NULL)
		printf("%s?", cwd);
		return 0;
	else
		printf("getcwd() error?");
	return -1;
}