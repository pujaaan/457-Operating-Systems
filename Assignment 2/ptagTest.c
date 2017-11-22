#include <linux/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

#define __NR_ptag 337		//imports the system call

int main(int argc, char* argv[]){
	int opt;
	int mode = 0; //0 = add, 1 = remove
	pid_t pid;
	if (argc != 4) {    
        	printf("Usage: ptag <PID> <flag> <tag>\n");
        	exit(1);
    	}	
	
	pid = atoi(argv[1]);				//retrieves pid from Command Line and convert to int
    	if(kill(pid, 0) != 0) {
        	printf("Error: PID does not exist or does not belong to you!\n");
        	return -1;
    	}
	
	char * tag = argv[3];					//retrieve tag from user and put in variable

	while ((opt = getopt(argc, argv, "a:r")) != -1){	//uses getopt to determine whether or not we'll be adding or removing a tag
		switch(opt){					//this is done in a switch
			case 'a':				//in the case of add, set variable to 0
				mode = 0;
				break;
			case 'r':
				mode = 1;			//in the case of remove, set variable to 1
				break;
			default:
				mode = -1;			//in the case where there is neither, break program with error message
				printf("Error: No add/remove instruction was given.\n");
				exit(1);
		}
	}

	syscall(__NR_ptag, pid, tag, mode); 			//system call with the given parameters
	
}
