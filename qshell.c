/*
Written by Daniel Bowden - 4203648
Contact: daniel.bowden@uqconnect.edu.au
COMP3301 - Assignment 1
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Function Prototypes */
int printprompt(void);
char** getcommand(void);
int parsecommand(char**);
int getlength(char** array);

int main(void)
{
	while(1){
		printprompt();
		if(parsecommand(getcommand()) == 1){
			/*Kill background processes !!!!!!!!!*/
			return 0;
		}
	}
	return 0;
}

int printprompt(void)
{
	char cwd[1024];
	if(getcwd(cwd, sizeof(cwd)) != NULL)
	{
		printf("%s? ", cwd);
		return 0;
	}
	else
		printf("getcwd() error? ");
	return -1;
}

/*getcommand() gets the command line from the shell. It Returns a char*[] with each element being an argument
following the initial command with the final element being NULL. It Returns NULL if there was an error or if 
nothing was entered. The returned array must be freed once it is no longer needed.*/
char** getcommand(void)
{
	int bytes_read;
	size_t no_bytes = 128;/*maximum number of characters that will be read(can type more)*/
	char *command_line;
	char **input_split = NULL;/*The array we will be returning. To be freed in calling function*/
	char *tmp = NULL;
	int no_spaces = 0;
	int i = 0;
	
	/*allocate memory for string. + 1 for null char*/
	command_line = (char *) malloc (no_bytes + 1);
	bytes_read = getline(&command_line, &no_bytes, stdin);
	
	/*If more than no_bytes is read no_bytes value will change, hence number used not variable*/
	if(bytes_read > 129){
		printf("\nInvalid command line! Command lines may be up to and including 128 characters.");
		free(command_line);
		return NULL;
	}
	else if(bytes_read < 1){
		printf("\nError reading command line.\n");
		free(command_line);
		return NULL;
	}
	else if(bytes_read == 1){
		/*User just hit enter */
		free(command_line);
		return NULL;
	}
	else{
		tmp = strtok(command_line, " ");
		while(tmp){
			/*Reallocate memory for Array*/
			input_split = realloc(input_split, sizeof(char*) * ++no_spaces);
			if(input_split == NULL){
				/*memory allocation failed*/
				printf("\nError reading command line.\n");
				free(command_line);
				free (input_split);
				return NULL;
			}

			/*Remove \n from last argument.*/
			i= strlen(tmp);
			/*If the \n character is the last character(other then the end of string null character)*/
			if(tmp[i-1] == '\n'){
				/*Make it the NULL char*/
				tmp[i-1] = 0;
				/*If the string only contained the \n char then we are done. As there was trailing white space*/
				i = strlen(tmp);
				if(i == 0){;
					/*Decrement space count as no more are being added*/
					no_spaces--;
					tmp = NULL;
				}
				else{
					input_split[no_spaces-1] = tmp;
					tmp = NULL;
				}
			}
			else{
				input_split[no_spaces-1] = tmp;
				tmp = strtok (NULL, " ");
			}
		}
		
		/*Reallocate an extra element for final NULL*/
		input_split = realloc(input_split, sizeof(char*) * (no_spaces+1));
		input_split[no_spaces] = 0;

		
		/*If command + max of 20 arguments i.e. 21*/
		if(no_spaces > 21){
			printf("\nInvalid command line! Command lines may have at most 20 Arguments.\n");
			free(command_line);
			free (input_split);
			return NULL;
		}
		/*
		for (i=0; i< (no_spaces+1); i++){
			printf("input_split[%d] = .%s.\n", i, input_split[i]);
		}
		printf("bytes_read: %d\n", bytes_read);/**/
	}
	return input_split;
}

/*parsecommand() takes a char** (array of strings) with the first string the command to execute followed by any additional
arguments. Returns 1 if exit was entered. Returns -1 if no input was entered (or if input was just spaces).
*/
int parsecommand(char** input)
{
	if(input == NULL || input[0] == NULL){
		return -1;/*no userinput*/
	}
	else if(strcmp(input[0], "cd") == 0){
		if(getlength(input) > 1){
			if(chdir(input[1]) != 0){
				printf("\nError changing directory.\n");
			}
		}
	}
	else if(strcmp(input[0], "exit") == 0){
		return 1;
	}
	return 0;
}

int getlength(char** array)
{
	int count = 0;
	while(array[count] != NULL) count++;
	return count;
}