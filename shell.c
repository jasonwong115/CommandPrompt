#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#define MAXSIZE 100
#define MAXDIRSIZE 1024

/*******CONSTRUCTORS*/
int numArguments(const char string[ ]);
void printError();
void addJob(int child, char* command);
void removeJob();

/*******Global variables*/
//List of jobs and process ids
char jobs[MAXSIZE][30];
int numJobs = 0;

/*******MAIN PROGRAM*/
int main(int argc, char**argv){
	
	char* prompt;

	/*******INITIALIZATION PHASE*/
	//Process command line input
	if(argc == 3 && strncmp("-p", argv[1], 2) == 0){
		prompt = argv[2];
	}else if(argc == 1){
		prompt = "308sh>";
	}else{
		printf("Invalid input detected. Exiting shell.\n");
		return;
	}

	/*******CREATE GLOBAL VARIABLES*/
	char userinput[MAXSIZE];

	//String to be used to store directory name for cd command
	char directory[MAXSIZE];

	//value of current working directory
	char cwd[MAXDIRSIZE];

	//Used to hold values of split string (delimeter = ' ')
	char* pch;

	//1 if background process, 0 otherwise
	int bgProcess = 0;

	//status of execvp command
	int childstatus;
	
	//return values of forks
	int child;
	int childBackground;

	/*******SHELL: PROCESS USER INPUT UNTIL EXIT*/
	while(1){
		//Print the prompt string and wait for user input
		printf("%s",prompt);
		fgets(userinput, MAXSIZE, stdin);

		//Removing newline from user input and replacing with null terminator
		if(strlen(userinput) > 0 && userinput[strlen(userinput) - 1] == '\n')
			userinput[strlen(userinput)-1] = '\0';
		
		/*******Process Built-in commands*/
		//If nothing is entered, print error
		if(strlen(userinput) == 0){
			printError(userinput);

		//Terminate shell on "exit" command
		}else if(strlen(userinput) == 4 && strncmp("exit", userinput, 4) == 0){
			return 0;
	
		//Print out the process ID of this shell
		}else if(strlen(userinput) == 3 && strncmp("pid", userinput, 3) == 0){
			printf("The process ID of this shell is %d.\n", getpid());
	
		//Print out the process ID of the parent of this shell
		}else if(strlen(userinput) == 4 && strncmp("ppid", userinput, 4) == 0){
			printf("The parent process ID of this shell is %d.\n", getppid());

		//Change working directory to supplied directory. If none, then default to home directory
		}else if(strncmp("cd", userinput, 2) == 0 && (strlen(userinput) == 2 || userinput[2] == ' ')){
			if(strlen(userinput) == 2){
				chdir(getenv("HOME"));
			}else{
				//Copy directory to change to into string
				memcpy(directory,&userinput[3],strlen(userinput)-2);
				directory[strlen(userinput)-3] = '\0';
				
				//Check if directory change unsuccessful
				if(chdir(directory) == -1){
					printf("Directory not found.\n");
				}
			}

		//Print out the current working directory
		}else if(strlen(userinput) == 3 && strncmp("pwd", userinput, 3) == 0){
			if(getcwd(cwd,MAXDIRSIZE) == -1){
				printf("The current working directory could not be printed.");
			}else{
				printf("%s\n", cwd);
			}

		//Set environment variable. Clear the variable if only one argument
		}else if(strncmp("set", userinput, 3) == 0){
			int num = numArguments(userinput);
			//Set env variable since value provided
			if(num == 3){

				int pos = 0;
				char* key;
				char* value;
				
				//split string and find key, value using " " as delimeter
				pch = strtok (userinput," ");
				while (pch != NULL){
					if(pos == 1){
						key=pch;
					}else if(pos == 2){
						value=pch;
					}
					pch = strtok (NULL, " ");
					pos=pos+1;
				}

				//Try to set env variable, and check if any errors
				if (setenv(key, value, 1) != 0 ){
					printf("Was unable to set environment variable");
				};


			//Clear env variable since value not provided
			}else if(num == 2){
				int pos = 0;
				char* key;
				
				//split string and find key using " " as delimeter
				pch = strtok (userinput," ");
				while (pch != NULL){
					if(pos == 1){
						key=pch;
					}
					pch = strtok (NULL, " ");
					pos=pos+1;
				}
				if (unsetenv(key) != 0 ){
					printf("Was unable to clear environment variable");
				};

			}else{
				printError(userinput);
			}

		//Print value of current environment variable
		}else if(strncmp("get", userinput, 3) == 0){
			int num = numArguments(userinput);
			if(num==2){
				int pos = 0;
				char* key;

				//split string and find key,value using " " as delimeter
				pch = strtok (userinput," ");
				while (pch != NULL){
					if(pos == 1){
						key=pch;
					}
					pch = strtok (NULL, " ");
					pos=pos+1;
				}

				if(getenv(key) == NULL){
					printf("Failed to find environment variable\n");
				}else{
					printf("%s\n", getenv(key));
				}
			}else{
				printError(userinput);

			}
			
		//Not built-in command. Spawn child to run process and wait to complete
		}else if(strncmp("jobs", userinput, 4) == 0){
			int i = 0;
			printf("Num jobs: %d\n",numJobs);
			for(i = 0; i < numJobs; i++){
				printf("%s\n", jobs[i]);
			}

		}else{
			int numArg = numArguments(userinput);

			//Check to see if this needs to be run in background
			if(userinput[strlen(userinput) - 1] == '&'){
				numArg = numArg - 1;
				bgProcess = 1;
			}

			//create array of arguments to pass to execvp
			char* args[numArg + 1];
			int i = 0;
				
			//split user input by " " and put into arguments array
			pch = strtok (userinput," ");
			while (pch != NULL){
				//value = strcat(pch,'\0');
				args[i] = pch;
				pch = strtok (NULL, " ");
				i=i+1;
			}

			//add null pointer to end of args
			args[numArg] = (char*) NULL;

			//background process so we need to run command in "parallel"
			if(bgProcess == 1){
				bgProcess = 0;
				//first child needed to wait on second child in order to grab exit codes	
				childBackground = fork();
				if(childBackground == 0){
					//second child created -- running in parallel now
					child = fork();
					if(child == 0){
						
						fprintf(stderr,"\n[%d] %s\n", getpid(), args[0]);
						execvp(args[0], args);
						//if we reach this point execution has failed
						perror("Problem occurred\0");
						return 0;
					}else{
						childstatus = -1;

						waitpid(-1, &childstatus, 0);

						//Detect child return value
						if (WIFEXITED(childstatus)) {
							printf("\n[%d] %s Exit %d\n", child, args[0], WEXITSTATUS(childstatus));
						} else if (WIFSIGNALED(childstatus)) {
						    	printf("\n[%d] %s Killed %d\n", child, args[0], WTERMSIG(childstatus));
						}

						printf("%s",prompt);
						return 0;
					}
				}else{
					usleep(400);
					//prepare variables to save to "jobs" list
					addJob(childBackground + 1, args[0]);
				}
			
			//Run the command and wait (not background process)
			}else{
				//use fork to create new process for command
				child = fork();
				// we are the child process since fork returned 0
				if(child == 0){
					printf("[%d] %s", getpid(), args[0]);
					//execute the command in the forked process
					execvp(args[0], args);
					//error occurred if anything beyond this point executes
					perror("Problem occurred\0");
					return 0;
				
				// we are the parent process
				}else{
					//wait a bit for child process to start
					usleep(1500);
					childstatus = -1;
					
					//prepare variables to save to "jobs" list
					addJob(child, args[0]);

					//wait for the child to complete since not a background process
					waitpid(child, &childstatus, 0);
					 
					//Detect child return value
					if (WIFEXITED(childstatus)) {
						printf("\n[%d] %s Exit %d\n", child, args[0], WEXITSTATUS(childstatus));
					} else if (WIFSIGNALED(childstatus)) {
					    	printf("\n[%d] %s Killed %d\n", child, args[0], WTERMSIG(childstatus));
					}
				}
			} // End of else (not background process)

		} // End of else loop (not built in)
	} // End of while(1) loop
} // End of main

/*******HELPER METHODS*/

//given a string, find number of arguments (using space as delimeter)
//e.g. "set hey 1" returns 3
//e.g. "set hey" returns 2
int numArguments(const char string[ ]){
	int numArguments = 0; // result
 
	const char* it = string;
	//currently processed a word
	int word = 0;
 
	//Switch statement to find num words
	do switch(*it) {
		case '\0': 
		case ' ': case '\t': case '\n': case '\r':
			if (word) { word = 0; numArguments++; }
			break;
		default: word = 1;
	} while(*it++);
 
	return numArguments;
}

//prints a message when a command is not known or is formatted incorrectly
void printError(char* userinput){
	printf("Cannot exec %s: No such file or directory\n", userinput);
	return;
}

//add process to job list
void addJob(int child, char* command){
	char pid[15];
	char temp[30];
	char temp2[30];
	//convert int to string
	sprintf(pid, "%d", child);
	//format into <id> (process_name)
	strcpy(temp,strcat(pid, " ("));
	strcpy(temp2,strcat(temp,command));
	strcpy(jobs[numJobs], strcat(temp2,")"));
	numJobs = numJobs + 1;
	return;
}

//remove job from process list
void removeJob(int child){
	char pid[15];
	sprintf(pid, "%d", child);
	int i = 0;
	printf("Num jobs: %d\n",numJobs);
	//find job id and replace with last element in job list
	for(i = 0; i < numJobs; i++){
		if(strncmp(pid, jobs[i], 3) == 0){
			strcpy(jobs[i], jobs[numJobs]);
			numJobs = numJobs - 1;
			break;
		}
	}
	return;
}
