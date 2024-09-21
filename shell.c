#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFSIZE 4096
#define MAX_ARGS 100

/* Retrieve the hostname and make sure that this program is not being run on the main odin server.
 * It must be run on one of the vcf cluster nodes (vcf0 - vcf3).
 */
void check()
{
        char hostname[10];
        gethostname(hostname, 9);
        hostname[9] = '\0';
        if (strcmp(hostname, "csci-odin") == 0) {
                fprintf(stderr, "WARNING: TO MINIMIZE THE RISK OF FORK BOMBING THE ODIN SERVER,\nYOU MUST RUN THIS PROGRAM ON ONE OF THE VCF CLUSTER NODES!!!\n");
                exit(EXIT_FAILURE);
        } // if
} // check

/**
 * Tokenizes the command string into an array of strings.
 *
 * @param cmd - the command string to be tokenized
 * @param args - an array of strings to store the tokenized command
 */
void tokenize(char* cmd, char** args) {
	int i;
	for (i = 0; i < MAX_ARGS - 1; i++) {
		if (i == 0) {
			args[i] = strtok(cmd, " ");
		} else {
			args[i] = strtok(NULL, " ");
		} // else
		if (args[i] == NULL) {
			break; // exit the loop if there are no more tokens
		} // if
	} // for
	args[i] = NULL; // set last element to NULL
} // tokenize

/**
 * This program allows users to enter commands and then executes them.
 * It runs in an infinite loop, continuously prompting the user for input.
 *
 * @return 0 upon successfull execution.
 */
int main()
{
	check();
	setbuf(stdout, NULL); // makes printf() unbuffered
	int n;
	char cmd[BUFFSIZE];

	// Project 3 TODO: set the current working directory to the user home directory upon initial launch of the shell
	// You may use getenv("HOME") to retrive the user home directory

	char* home = getenv("HOME");

	if (home != NULL) {
		if (chdir(home) != 0) {
			fprintf(stderr, "Failed to change to home directory.\n");
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "Home directory not found.\n");
		exit(EXIT_FAILURE);
	}

	// inifite loop that repeated prompts the user to enter a command
	while (1) {

		char* cwd = getcwd(NULL, 0);

		if (cwd != NULL && home != NULL) {
			size_t homeLength = strlen(home);

			if (strncmp(cwd, home, homeLength) == 0) {
				// print the abbreviated path if the home directory is included in the cwd
				printf("1730sh:~%s$ ", cwd + homeLength);
				free(cwd);
			} else {
				// Project 3 TODO: display the current working directory as part of the prompt
				printf("1730sh:%s$ ", cwd);
				free(cwd);
			} // else
		} else {
			fprintf(stderr, "Failed to get current working directory.\n");
			exit(EXIT_FAILURE);
		} // else

		n = read(STDIN_FILENO, cmd, BUFFSIZE);

		// if user enters a non-empty command
		if (n > 1) {
			cmd[n-1] = '\0'; // replaces the final '\n' character with '\0' to make a proper C string


			// Lab 06 TODO: parse/tokenize cmd by space to prepare the
			// command line argument array that is required by execvp().
			// For example, if cmd is "head -n 1 file.txt", then the
			// command line argument array needs to be
			// ["head", "-n", "1", "file.txt", NULL].
			char* args[MAX_ARGS];
			tokenize(cmd, args);


			// Lab 07 TODO: if the command contains input/output direction operators
			// such as "head -n 1 < input.txt > output.txt", then the command
			// line argument array required by execvp() needs to be
			// ["head", "-n", "1", NULL], while the "< input.txt > output.txt" portion
			// needs to be parsed properly to be used with dup2(2) inside the child process

			// check for input/output redirection
			int redirectInput = 0;
			int redirectOutput = 0;
			char *inputFile = NULL;
			char *outputFile = NULL;

			for (int i = 0; args[i] != NULL; i++) {
				switch (args[i][0]) {
				case '<':
					redirectInput = 1;
					args[i] = NULL; // terminate args before input file
					inputFile = args[i + 1];
					break;
				case '>':
					if (args[i][1] == '>') {
						redirectOutput = 2;
					} else {
						redirectOutput = 1;
					}
					args[i] = NULL;
					outputFile = args[i + 1];
					break;
				} // switch
			} // for

			// Lab 06 TODO: if the command is "exit", quit the program
			if (strcasecmp(args[0], "exit") == 0) {
				exit(EXIT_SUCCESS);
			} else if (strcasecmp(args[0], "cd") == 0) {
				// Project 3 TODO: else if the command is "cd", then use chdir(2) to
				// to support change directory functionalities
				if (args[1] != NULL) {
					char* target = args[1];

					// change to home directory for "~"
					if (strcmp(target, "~") == 0) {
						char* home = getenv("HOME");
						if (home != NULL) {
							target = home;
						} else {
							fprintf(stderr, "Home directory not found.\n");
						} // else
					} // if
					if (chdir(target) != 0) {
						perror("cd");
					} // if
					cwd = getcwd(NULL, 0);
					free(cwd);
				} // if
				continue;
			} // else if

			// Lab 06 TODO: for all other commands, fork a child process and let
			// the child process execute user-specified command with its options/arguments.
			// NOTE: fork() only needs to be called once. DO NOT CALL fork() more than one time.

			// Lab 06 TODO: inside the child process, invoke execvp().
			// if execvp() returns -1, be sure to use exit(EXIT_FAILURE);
			// to terminate the child process

			// Lab 06 TODO: inside the parent process, wait for the child process
			// You are not required to do anything special with the child's
			// termination status

			pid_t pid = fork();

			if (pid < 0) {
				perror("Fork failed");
				exit(EXIT_FAILURE);
			} else if (pid == 0) {

				// handle input redirection
				if (redirectInput) {
					int fdInput = open(inputFile, O_RDONLY);
					if (fdInput == -1) {
						perror("Failed to open input file");
						exit(EXIT_FAILURE);
					} // if

					// Lab 07 TODO: inside the child process, use dup2(2) to redirect
					// standard input and output as specified by the user command
					dup2(fdInput, STDIN_FILENO);
					close(fdInput);
				} // if

				// handle output redirection
				int fdOutput;
				switch (redirectOutput) {
				case 1:
					fdOutput = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fdOutput == -1) {
						perror("Failed to open output file");
						exit(EXIT_FAILURE);
					} // if

					// Lab 07 TODO: inside the child process, use dup2(2) to redirect
					// standard input and output as specified by the user command
					dup2(fdOutput, STDOUT_FILENO);
					close(fdOutput);
					break;
				case 2:
					fdOutput = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
					if (fdOutput == -1) {
						perror("Failed to open output file");
						exit(EXIT_FAILURE);
					} // if

					// Lab 07 TODO: inside the child process, use dup2(2) to redirect
					// standard input and output as specified by the user command
					dup2(fdOutput, STDOUT_FILENO);
					close(fdOutput);
					break;
				} // switch

				// execute user-specified command with its options/args
				int result = execvp(args[0], args);

				for (int i = 0; args[i] != NULL; i++) {
					free(args[i]);
				} // for

				// if execvp fails, print an error message and exit the child process
				if (result == -1) {
					perror("Execvp failed");
					exit(EXIT_FAILURE);
				} // if
			} else {
				int status;
				waitpid(pid, &status, 0);
			} // else
		} // if
	} // while
	return 0;
} // main
