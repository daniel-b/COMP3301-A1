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
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* Function Prototypes */
int printprompt(void);
char** getcommand(void);
pid_t parsecommand(char**);
int getlength(char** array);
int basiccommand(char** command_array);
int validcommand(char** command_array);
int getindex(char** command_array, char search_item);
static void sig_handler(int sig);

/*Global Variables*/
static volatile sig_atomic_t got_signal = 0;
static volatile int foreground_pid = 0; /* = 0 when no command is running in foreground.*/

int main(void)
{
	pid_t current_command;
	pid_t finished_pid;
	int wait_stat;
	struct sigaction action;
	action.sa_handler = sig_handler;
	action.sa_flags = 0;
	
	sigaction(SIGINT, &action, NULL);
	while(1){
		while((finished_pid = waitpid(WAIT_ANY, &wait_stat, WNOHANG)) > 0){
			printf("\nProcess with PID: %d has finished with exit status: %d\n", finished_pid, wait_stat);
		}
		printprompt();
		current_command = parsecommand(getcommand());
		if(current_command == -1){
			/*Kill background processes !!!!!!!!!*/
			return 0;
		}
		else if(current_command > 0){/*background command pid*/
			/*May not need*/
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

	/*if stdin is in error state(sometimes after SIGINT caught) open stdin again*/
	if(ferror(stdin)){
		stdin = fopen("/dev/tty", "r");
	}
	
	bytes_read = getline(&command_line, &no_bytes, stdin);
	/*If ctrl+c pressed*/
	if(bytes_read < 1 && errno ==EINTR){
		printf("\n");/*Print a newline to keep formatting nice*/
		free(command_line);
		return NULL;
	}
	/*If more than no_bytes is read no_bytes value will change, hence number used not variable*/
	else if(bytes_read > 129){
		printf("\nInvalid command line! Command lines may be up to and including 128 characters.\n");
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
Returns the pid of the process started(if it was a background command).
*/
pid_t parsecommand(char** input)
{
	pid_t pid;
	int wait_status;
	char command_buf[129];
	int stdin_fd;
	int stdout_fd;
	int symbol_index;
	
	if(input == NULL || input[0] == NULL){
		return 0;/*no userinput*/
	}
	else if(strcmp(input[0], "cd") == 0){
		if(getlength(input) > 1){
			if(chdir(input[1]) != 0){
				printf("\nError changing directory.\n");
			}
		}else{
			if(chdir(getenv("HOME")) != 0){
				printf("\nError changing directory.\n");
			}
		}
	}
	else if(strcmp(input[0], "exit") == 0){
		return -1;
	}
	else if(input[0][0] == '#'){
		return 0;/*Return as it is a comment line.*/
	}
	/*Non Built in commands are parsed below*/
	else{
		
		if(basiccommand(input) == 0){
			strncpy(command_buf, input[0], 129);
			pid = fork();
			if(pid < 0){
				printf("\nError executing command.\n");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){/*Child process*/
				execvp(command_buf, input);
				printf("\nFailed to run command: %s\n", command_buf);/*only run if exec fails*/
				exit(EXIT_FAILURE);
			}
			
			/*Parent Process*/
			foreground_pid = pid;
			pid = waitpid(pid, &wait_status, 0);
			foreground_pid = 0;/*Set back to 0 immediately once foreground child terminates.*/
			if(pid < 0){
				printf("\nCommand cancelled.\n");
			}
		}
		else if(validcommand(input) == 0){
			printf("\nValid command\n");
			
			/*When a process has to be run in the background*/
			if(strcmp(input[getlength(input)-2], "&") == 0){
				strncpy(command_buf, input[0], 129);
				input[getlength(input)-2] = NULL;
				pid = fork();
				if(pid < 0){
					printf("\nError executing command.\n");
					exit(EXIT_FAILURE);
				}
				else if(pid == 0){/*Child process*/
					symbol_index = getindex(input, '<');
					if(symbol_index > 0){/*Input redirection. Can't be element 0*/
						stdin_fd = open(input[symbol_index + 1], O_RDONLY);
						if(stdin_fd < 0){
							printf("\nFailed to run command. File %s could not be opened.\n", input[symbol_index + 1]);
							exit(EXIT_FAILURE);
						}
						dup2(stdin_fd, STDIN_FILENO);
						close(stdin_fd);
						
						/*Remove < and filename from command argument array*/
					}
					else{
						/*If no input redirect entered.*/
						stdin_fd = open("/dev/null", O_RDONLY);
						dup2(stdin_fd, STDIN_FILENO);
						close(stdin_fd);
					}
					symbol_index = getindex(input, '>');
					if(symbol_index > 0){/*Output redirection. Can't be element 0*/
						stdout_fd = open(input[symbol_index + 1], O_WRONLY | O_CREAT | O_TRUNC);
						if(stdout_fd < 0){
							printf("\nFailed to run command. File %s could not be opened.\n", input[symbol_index + 1]);
							exit(EXIT_FAILURE);
						}
						dup2(stdout_fd, STDOUT_FILENO);
						close(stdout_fd);
					}
					
					execvp(command_buf, input);/*input[1], (char*)0);*/
					printf("\nFailed to run command: %s\n", command_buf);/*only run if exec fails*/
					exit(EXIT_FAILURE);
				}
				
				/*Parent Process*/
				/*Doesn't need to do anything (other than return child pid) check is made before prompt printing*/
				printf("\nCommand %s started with PID: %d\n", input[0], pid);
				return pid;
			}
		}
		else{
			printf("\nInvalid command line syntax.\n");
		}
	}
	return 0;
}

/*validcommand() checks the syntax of the command line.

Returns 0 if the command is a valid qshell command. Returns -1 otherwise.
*/
int validcommand(char** command_array)
{
	int in_count =0;
	int out_count =0;
	int pipe_count =0;
	int bg_count =0;
	int i =0;
	int filename_check = 0;
	
	/*Start at element[1] as element[0] is the command and not an argument.*/
	for(i=1; i < getlength(command_array); i++){
		/*< or > must be followed by a filename*/
		if(filename_check == 1){
			/*i.e. there is no next item.*/
			if(command_array[i] == NULL){
				return -1;
			}/*Anything other than <, >, | or & is considered a filename*/
			else if(strcmp(command_array[i], "<") == 0 || strcmp(command_array[i], ">") == 0 || 
			strcmp(command_array[i], "|") == 0 || strcmp(command_array[i], "&") == 0)
			{
				return -1;
			}
			filename_check = 0;
		}
		else if(command_array[i] == NULL){
			break;
		}
		else if(strcmp(command_array[i], "<") == 0){
			in_count++;
			filename_check =1;
			/*command on the right side of a pipe will not also have input redirection.*/
			if(pipe_count > 0){
				return -1;
			}
		}
		else if(strcmp(command_array[i], ">") == 0){
			out_count++;
			filename_check =1;
		}
		else if(strcmp(command_array[i], "|") == 0){
			pipe_count++;
			/*command on the left side of a pipe will not also have output redirection.*/
			if(out_count > 0){
				return -1;
			}
		}
		else if(strcmp(command_array[i], "&") == 0){
			bg_count++;
			/*you do not have to worry about piped commands running in the background.*/
			if(pipe_count > 0){
				return -1;
			}/*If the command is to be executed in the background, the last word must be &*/
			else if(command_array[i+1] != NULL){
				return -1;
			}
		}
	}
	if(in_count > 1 || out_count > 1 || pipe_count > 1 || bg_count > 1){
		return -1;
	}
	else{
		return 0;
	}
}

/*basiccommand() returns 0 if none of the arguments given is:
<, > (redirects), | (pipe) or &(run proccess in background) arguments. 

Returns -1 if an argument is one of the above listed.
*/
int basiccommand(char** command_array)
{
	int i = 0;
	
	/*Start at element[1] as element[0] is the command and not an argument.*/
	for(i=1; i < getlength(command_array); i++){
		if(command_array[i] == NULL){
			return 0;
		}
		else if(strcmp(command_array[i], "<") == 0 || strcmp(command_array[i], ">") == 0 || 
			strcmp(command_array[i], "|") == 0 || strcmp(command_array[i], "&") == 0)
		{
			return -1;
		}
	}
	return 0;
}

/*getindex() takes a null terminated char** and searches it for the FIRST occurrence of
search_item as the whole value contained within an element.

Returns the index of the element containing search_item. Returns -1 if no element found matching.
*/
int getindex(char** command_array, char search_item)
{
	int i;
	/*Start at element[1] as element[0] is the command and not an argument.*/
	for(i=1; i < getlength(command_array); i++){
		if(command_array[i] == NULL){
			return -1;/*search_item not found*/
		}
		else if(strcmp(command_array[i], &search_item) == 0){
			return i;
		}
	}
	return -1;
}

static void sig_handler(int sig)
{
	/*Shouldn't be needed but on the side of caution :) */
	if(sig == SIGINT){
		if(foreground_pid > 0){
			if(kill(foreground_pid, SIGTERM) == -1){
				kill(foreground_pid, SIGKILL);
			}
		}
	}
}

int getlength(char** array)
{
	int count = 0;
	while(array[count] != NULL) count++;
	return count + 1;/*+ 1 as the NULL element is an element.*/
}


