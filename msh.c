// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017, 2021 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 7f704d5f-9811-4b91-a918-57c1bb646b70
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10    // Mav shell only supports 10 arguments

#define HISTORY_LENGTH 15       // Number of maximum pids/processes to show in
                                // their respective histories


// See function descriptions at initialization
void add_pid(pid_t pids[], pid_t pid, int* pid_ctr);
void listpids(pid_t pids[], int pid_ctr);
void add_command(char** commands, char* cmd, int* cmd_ctr);
void history(char** commands, int cmd_ctr);
int tokenize(char** token, char* working_str);
// These two functions end up calling each other, leaving me with LOTS of passed variables.
// Maybe these should've been global?
void run_history(char* cmd, char** commands, int* cmd_ctr, pid_t pids[], int* pid_ctr, char** token, 
                char* working_str);
int run(char** token, pid_t pids[], int* pid_ctr, char** commands, int* cmd_ctr, char* working_str);



// Function for adding pids to the array of stored pids for history purposes.
// Limits the structure to only holding the HISTORY_LENGTH previous pids generated.
// In: The array holding pid history, the pid being added to the pid history, and the current
//     number of pids in the history.
// Out: None
void add_pid(pid_t pids[], pid_t pid, int* pid_ctr) {
    if(*pid_ctr == HISTORY_LENGTH) {
        int i;
        for(i = 0; i < HISTORY_LENGTH; i++) {
            pids[i] = pids[i + 1];
        }
        pids[HISTORY_LENGTH - 1] = pid;
    } else {
        pids[*pid_ctr] = pid;
        (*pid_ctr)++;
    }
}
// List all pids in the pid history array
// In: The array holding pid history and the current number of pids in the history
// Out: Print to terminal the index of the pid in the history and the pid itself
void listpids(pid_t pids[], int pid_ctr) {
    int i;
    for(i = 0; i < pid_ctr; i++) {
        printf("%d: %d\n", i, pids[i]);
    }
}
// Function for adding commands to the array of stored commands for history purposes.
// Limits the structure to only holding the HISTORY_LENGTH previous commands attempted.
// In: The array holding command history, the command being added to the command history,
//     and the current number of commands in the history.
// Out: None
void add_command(char** commands, char* cmd, int* cmd_ctr) {
    if(*cmd_ctr == HISTORY_LENGTH) {
        int i;
        for(i = 0; i < HISTORY_LENGTH; i++) {
            commands[i] = commands[i + 1];
        }
        strcpy(commands[HISTORY_LENGTH - 1], cmd);
    } else if(*cmd_ctr < HISTORY_LENGTH){
        strcpy(commands[*cmd_ctr], cmd);
        (*cmd_ctr)++;
    } else {
        *cmd_ctr = HISTORY_LENGTH;
    }
}
// List all commands in the command history array
// In: The array holding command history and the current number of commands in the history
// Out: Print to terminal the index of the command in the history and the command itself, with tags
void history(char** commands, int cmd_ctr) {
    int i;
    for(i = 0; i < cmd_ctr; i++) {
        printf("%d: %s\n", i, commands[i]);
    }
}
// Tokenize a string using the WHITESPACE delimiter. Moved here for reuse with !n command
// In: pointer to token array, pointer to working string (user input)
// Out: Number of tokens generated
int tokenize(char** token, char* working_str) {

    char* argument_ptr;
    int token_count = 0;

    while(((argument_ptr = strsep(&working_str, WHITESPACE)) != NULL) && 
        (token_count<MAX_NUM_ARGUMENTS)) {
      
        token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
        if(strlen(token[token_count]) == 0) {
            token[token_count] = NULL;
        }
        token_count++;
    }
    token[MAX_NUM_ARGUMENTS] = NULL;
    return token_count;
}
// Function to run the nth most recent command in the command history
// In: char array starting with ! followed by int n, in format "!n", command history array
//     number of commands in history currently, as well as pid history and address of pid ctr
// Out: None
void run_history(char* cmd, char** commands, int* cmd_ctr, pid_t pids[], int* pid_ctr, char** token, 
                char* working_str) {
    // get num after the !
    int index = atoi(cmd + 1);

    if(index >=0 && index < *cmd_ctr){
        // MUST change "working_str" or we'll get an infinite loop
        working_str = commands[index];
        tokenize(token, working_str);
        run(token, pids, pid_ctr, commands, cmd_ctr, working_str);
    } else {
        printf("Command not in history.");
    }
}
// Function to run the command given
// In: tokenized char ** holding each token of the command to be run, pid history array, 
//     and address of pid counter pid_dtr. Pass in history and count for commands for 
//     use in "run_history". Maybe these should've been global.
// Out: Return 0 if quit/exit, otherwise return 1
int run(char** token, pid_t pids[], int* pid_ctr, char** commands, int* cmd_ctr,  char* working_str) {

    // Save a copy of user input into command history
    add_command(commands, working_str, cmd_ctr);

    if(token[0] != NULL) {

        if(strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0) {
            return 0;
        } else if(strcmp(token[0], "cd") == 0) {
            if(chdir(token[1]) == -1) {
                printf("%s: No such file or directory.", token[1]);
            }
        } else if(strcmp(token[0], "listpids") == 0) {
            listpids(pids, *pid_ctr);
        } else if(strcmp(token[0], "history") == 0) {
            history(commands, *cmd_ctr);
        } else if(token[0][0] == '!') {
            run_history(token[0], commands, cmd_ctr, pids, pid_ctr, token, working_str);
        } else {

            // create a new process to execute our command
            pid_t pid = fork();

            //child process
            if(pid == 0) {
                //run the command in token[0] with arguments in the remainder of token,
                //tell the user their command DNE if can't be found
                int ret = execvp(token[0], &token[0]);
                if(ret == -1) {
                    printf("%s: Command not found.\n", token[0]);
                }
                return 0;

            //parent process
            } else {
                // Only add a pid if we actually fork a process, even if the process doesn't
                //result in successful command execution
                add_pid(pids, pid, pid_ctr);

                int status;

                //stop parent execution while child executes
                wait(&status);
            }
        }
    }
    return 1;
}


int main() {

    char* cmd_str = (char*) malloc(MAX_COMMAND_SIZE);

    // Store pids and commands in these for history purposes. See "add_pid"
    // and "add_command".
    pid_t pids[HISTORY_LENGTH + 1];
    int pid_ctr = 0;

    char** commands = malloc((HISTORY_LENGTH + 1) * sizeof(char *));
    int i;
    for(i = 0; i <= HISTORY_LENGTH; i++) {
        commands[i] = malloc(MAX_COMMAND_SIZE);
    }
    int cmd_ctr = 0;


    // Begin infinite loop. This allows the shell to keep accepting input
    // after each command execution.
    while(1) {
        // Print out the msh prompt
        printf("msh> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while(!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));


        /* Parse input */
        char* token[MAX_NUM_ARGUMENTS + 1];                                                               
                                                           
        char* working_str = strdup(cmd_str);        

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char* working_root = working_str;

        //split the string by whitespace delimiter for processing
        tokenize(token, working_str);
        
        // Attempt to run the command stored in token. If the command was quit or exit,
        // run will return 0 and we can quit here with another return 0.
        if(run(token, pids, &pid_ctr, commands, &cmd_ctr, working_str) == 0) {
            return 0;
        }
        free(working_root);
    }
    for(i = 0; i < HISTORY_LENGTH; i++) {
        free(commands[i]);
    }
    free(commands);
    return 0;
}
