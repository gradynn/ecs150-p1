#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#define CMDLINE_MAX 512

enum error {
        ERR,
        NO_ERR,
        ERR_TOO_MANY_ARGS,
        ERR_MISS_CMD,
        ERR_NO_OUT,
        ERR_CANT_OPEN_OUT,
        ERR_OUT_REDIR,
        ERR_CANT_CD,
        ERR_CMD_NOT_FND,
};

struct Command {
	char* args[16];
        char* path;
        int append;
	struct Command* next;
};

struct CommandString {
        char string[CMDLINE_MAX];
        struct CommandString* next;
};

enum error error_mgmt(enum error err)
{
        switch (err) {
        case ERR_TOO_MANY_ARGS:
                fprintf(stderr, "Error: too many process arguments\n");
                break;
        case ERR_MISS_CMD:
                fprintf(stderr, "Error: missing command\n");
                break;
        case ERR_NO_OUT:
                fprintf(stderr, "Error: no output file\n");
                break;
        case ERR_CANT_OPEN_OUT:
                fprintf(stderr, "Error: cannot open output file\n");
                break;
        case ERR_OUT_REDIR:
                fprintf(stderr, "Error: mislocated output redirection\n");
                break;
        case ERR_CANT_CD:
                fprintf(stderr, "Error: cannot cd into directory\n");
                break;
        case ERR_CMD_NOT_FND:
                fprintf(stderr, "Error: command not found\n");
                break;
        default:
		return NO_ERR;
                break;
        }

	return ERR;
}

enum error directory_traversal(struct Command* command_head) {
        struct Command* ptr = command_head;

        while (ptr != NULL) {
                if (!strcmp(ptr->args[0], "cd")) {
                        if (chdir(ptr->args[1]) == -1)
                                return error_mgmt(ERR_CANT_CD);
                }

                if (!strcmp(ptr->args[0], "pwd")) {
                        char cwd[CMDLINE_MAX];
                        getcwd(cwd, sizeof(cwd));
                        printf("%s\n", cwd);
                        return NO_ERR;
                }

                ptr = ptr->next;
        }

        return NO_ERR;
}

enum error output_redirection(struct Command* command) {
        if (command->path != NULL) {
                int file_o;

                if (command->append == 0)
                        file_o = open(command->path, O_RDWR | O_CREAT, 0644);
                else
                        file_o = open(command->path, O_RDWR | O_CREAT | O_APPEND, 0644);
                if (file_o == -1) {
                        return error_mgmt(ERR_CANT_OPEN_OUT);
                }
                
                dup2(file_o, STDOUT_FILENO);
                close(file_o);
        }

        return NO_ERR;
}

enum error parent_execute(struct Command* ptr, int* fd, int* exit_codes, int pid, int exit_code_index, int next)
{
	close(*(fd + 1));
	dup2(*fd, STDIN_FILENO);
	close(*fd);

	waitpid(pid, &exit_codes[exit_code_index], 0);

	if (next == 1)
		ptr = ptr->next;
	else if (next == 2)
		ptr = ptr->next->next;
	else if (next == 3)
		ptr = ptr->next->next->next;

	if (output_redirection(ptr) == ERR)
                return ERR;
	
        if (execvp(ptr->args[0], ptr->args) == -1) {
                error_mgmt(ERR_CMD_NOT_FND);
                raise(SIGKILL);
        }

        return NO_ERR;
}

enum error child_execute(struct Command* ptr, int* fd)
{
	close(*fd);
	dup2(*(fd + 1), STDOUT_FILENO);
	close(*(fd + 1));
	
	if (output_redirection(ptr) == ERR)
                return ERR;
                
	if(execvp(ptr->args[0], ptr->args) == -1) {
                error_mgmt(ERR_CMD_NOT_FND);
                raise(SIGKILL);
        }      

        return NO_ERR;
}

enum error execute_job(struct Command* command_head, int command_counter, int exit_codes[])
{
        struct Command* ptr = command_head;

        if (directory_traversal(ptr) == ERR)
                return ERR_CANT_CD;

        if (command_counter == 1) {
                int pid = fork();

                if (pid == 0) { //child
                        if (output_redirection(ptr) == ERR)
                                return ERR;
                        if (execvp(ptr->args[0], ptr->args) == -1) {
                                error_mgmt(ERR_CMD_NOT_FND);
                                raise(SIGKILL);
                        }
                } else { // parent
                        waitpid(pid, &(exit_codes[0]), 0);
                }
        } else if (command_counter == 2) {
                int pid = fork();
                int fd[2];

                if (pid == 0) { // child 1
                        pipe(fd);
                        
                        pid = fork();
                        if(pid == 0) { // child 2
                                if (child_execute(ptr, fd) == ERR)
                                        return ERR;
                        } else { // parent 2
                                if (parent_execute(ptr, fd, exit_codes, pid, 0, 1) == ERR)
                                        return ERR;       
                        }
                } else { // parent 1
                      waitpid(pid, &(exit_codes[1]), 0);
                }
        } else if (command_counter == 3) {
                int pid = fork();
                int fd[2];

                if (pid == 0) { // child 1
                        pipe(fd);
                        pid = fork();

                        if (pid == 0) { // child 2
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);

                                pipe(fd);
                                pid = fork();
                                if (pid == 0) { // child 3
                                        if (child_execute(ptr, fd) == ERR)
                                                return ERR;
                                } else { //parent 3
                                        if (parent_execute(ptr, fd, exit_codes, pid, 0, 1) == ERR)
                                                return ERR;
                                }
                        } else { // parent 2
                                if (parent_execute(ptr, fd, exit_codes, pid, 1, 2) == ERR)
				        return ERR;
                        }
                } else { // parent 1
                        waitpid(pid, &(exit_codes[2]), 0);
                }
        } else if (command_counter == 4) { 
                int pid = fork();
                int fd[2];

                if (pid == 0) { // child 1
                        pipe(fd);
                        pid = fork();

                        if (pid == 0) { // child 2
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);

                                pipe(fd);
                                pid = fork();
                                if (pid == 0) { // child 3
                                        close(fd[0]);
                                        dup2(fd[1], STDOUT_FILENO);
                                        close(fd[1]);

                                        pipe(fd);
                                        pid = fork();
                                        if (pid == 0) { // child 4
                                                if (child_execute(ptr, fd) == ERR)
                                                        return ERR;
                                        } else { // parent 4
                                                if (parent_execute(ptr, fd, exit_codes, pid, 0, 1) == ERR)
                                                        return ERR;
                                        }

                                } else { //parent 3
					if (parent_execute(ptr, fd, exit_codes, pid, 1, 2) == ERR)
                                                return ERR;
                                }
                        } else { // parent 2
                                if (parent_execute(ptr, fd, exit_codes, pid, 2, 3) == ERR)
                                        return ERR;
                        }
                } else { // parent 1
                        waitpid(pid, &(exit_codes[3]), 0);
                }
        }

        return NO_ERR;
}

enum error stringlist_parse(struct CommandString* head_command_string, int* command_counter, char cmd[CMDLINE_MAX])
{
        /* Create pointer to move through list, preserving head */
        struct CommandString* current_command_string = (struct CommandString*) malloc(sizeof(struct CommandString));
        current_command_string = head_command_string;

        /* Count instances of | in the whole command */
        int operator_count = 0;
        for (int i = 0; i < strlen(cmd); i++) {
                if(cmd[i] == '|')
                        operator_count++;
        }

        char* token = strtok(cmd, "|");
        strcpy(current_command_string->string, token);
        token = strtok(NULL, "|");
        
        while (token != NULL) { // capture all piped commands as strings
                ++(*(command_counter));
                struct CommandString* new_command_string = (struct CommandString*) malloc(sizeof(struct CommandString));
                strcpy(new_command_string->string, token);
                
                current_command_string->next = new_command_string;
                current_command_string = current_command_string->next;
                token = strtok(NULL, "|");
        }
        current_command_string->next = NULL;

        /* Check for instances of > in inner commands */
        current_command_string = head_command_string;
        
        for (int i = 0; i < *(command_counter); i++) {
                for (int j = 0; j < strlen(current_command_string->string); j++) {
                        if ((current_command_string->string[j] == '>') && (i != (*command_counter - 1))) //ERROR HANDLING
                                return error_mgmt(ERR_OUT_REDIR);

                }
                current_command_string = current_command_string->next;
        }

        if (*command_counter != (operator_count + 1)) //ERROR HANDLIND
                return error_mgmt(ERR_MISS_CMD);
        
        return NO_ERR;
}

enum error command_parse(struct Command* command, char* cmd_text)
{
        /* Create copy of the passed command string to preseve original through
                tokeninzing */
        char* raw_cmd = (char*) malloc(sizeof(cmd_text));
        strcpy(raw_cmd, cmd_text);

        /* isolate output redirection path */
        int found_alpha = 0;
        int operator_count = 0; 
        for (long unsigned int i = 0; i < strlen(raw_cmd); i++) {
                if (isalpha(raw_cmd[i])) // detect alpha char for error handling
                        found_alpha = 1;
                
                if (raw_cmd[i] == '>') {
                        if (found_alpha == 0) // ensure an alpha char was found
                                return error_mgmt(ERR_MISS_CMD);

                        operator_count++; // count number of > operators
                }
        }

        if (operator_count > 1) 
                command->append = 1; // set the flag to signify append mode
        else
                command->append = 0; // set the flag to signify overwrite mode

        char* w_cmd = strtok(raw_cmd, ">"); // isolate command
        char* redirect_path = strtok(NULL, ">"); // isolate path if there is one
	
        if (operator_count > 0 && redirect_path == NULL) {
                return error_mgmt(ERR_NO_OUT);
        }
        
        if (redirect_path != NULL) {
                command->path = redirect_path;
        }

        /* isolate/assign command */
        char* token = (char*) malloc(CMDLINE_MAX);
        token = strtok(w_cmd, " ");
        command->args[0] = token; // store command

        token = strtok(NULL, " ");
        int i = 1; 
        do {
                if (i == 16) { // check for too many arguments
                        return error_mgmt(ERR_TOO_MANY_ARGS);
 
                }
                command->args[i] = token;
                token = strtok(NULL, " ");
                i++;
        } while (token != NULL); 
        command->args[i] = NULL; // set last argument to NULL terminating list

        return NO_ERR;
}

enum error commandlist_parse(struct Command* head_command, struct CommandString* head_command_string) {
        /* Create pointers to move through lists, preserving heads */
        struct Command* current_command = (struct Command*) malloc(sizeof(struct Command));
        current_command = head_command;
        struct CommandString* current_command_string  = (struct CommandString*) malloc(sizeof(struct CommandString));
        current_command_string = head_command_string;

        /* Implicit first call to command parse */
        if(command_parse(current_command, current_command_string->string) == ERR)
                return ERR;
        current_command_string = current_command_string->next;

        /* Parse the remining strings */
        while(current_command_string != NULL) {
                /* Create pointer for new command */
                struct Command* next_command = (struct Command*) malloc(sizeof(struct Command));
                if(command_parse(next_command, current_command_string->string) == ERR)
                        return ERR;

                /* Append new command to list and increment pointer to
                        newly created and increment CommandString pointer */
		current_command->next = next_command;
                current_command = current_command->next;
                current_command_string = current_command_string->next;
        } 

        return NO_ERR;
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                //int retval;
                int command_counter = 1;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        //printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Store copy of the entire command for output purposes */
                char cmd_cpy[CMDLINE_MAX];
                strcpy(cmd_cpy, cmd);

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Build a linked list of the command strings seperated by the
                        | operator */
                struct CommandString* head_command_string = (struct CommandString*) malloc(sizeof(struct CommandString));
                if(stringlist_parse(head_command_string, &command_counter, cmd) == ERR)
                        continue;


                /* Build a linked list of parsed Command stucts */
                struct Command* head_command = (struct Command*) malloc(sizeof(struct Command));
                if(commandlist_parse(head_command, head_command_string) == ERR)
                        continue;

                /* Execute the job (all commands in a chain) */
                int exit_codes[command_counter];              
                if (execute_job(head_command, command_counter, exit_codes) == ERR) 
                       continue;

                /* Output process completed with process exit codes */
                fprintf(stderr, "+ completed '%s' ", cmd_cpy);
                for (int i = 0; i < command_counter; i++)
                {
                        fprintf(stderr, "[%i]", WEXITSTATUS(exit_codes[i]));
                }
                fprintf(stderr, "\n");

                //free(head_command);
                //free(current_command);
                //free(head_command_string);
                //free(current_command_string);
                //free(new_command_string);
		//free(token);
        }

        return EXIT_SUCCESS;
}
