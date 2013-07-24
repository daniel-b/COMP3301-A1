all: clean qshell #small project so clean first

qshell: qshell.c
	gcc -g -Wall -pedantic -ansi -o $@ $^ #remove -g for sumbission
	
clean: #doesn't really do anything as -o option is ussed currently
	rm -rf *.o qshell
	 