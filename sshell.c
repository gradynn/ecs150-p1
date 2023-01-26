#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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
        ERR_CMD_NOT_FND
};

struct CommandList {
	char cmd[CMDLINE_MAX];
	char* args[16];
        char* path;
        int append;
	struct CommandList* next;
};

struct StringList {
        char string[CMDLINE_MAX];
        struct StringList* next;
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

int psuedo_system(struct CommandList* command)
{
        /* Handles change of directory without creating new process */
        if (!strcmp(command->args[0], "cd")) {
                int ch_status = chdir(command->args[1]);
                return WEXITSTATUS(ch_status);
        }

        if (!strcmp(command->args[0], "pwd")) {
                char cwd[CMDLINE_MAX];
                getcwd(cwd, sizeof(cwd));
                printf("%s\n", cwd);
                return 0;
        }

	int status = 0;
        int pid = fork();

        if (pid == 0) { // child
                /* Support for output redirection */
                if (command->path != NULL) {
                        int file_o = open(command->path, O_RDWR | O_CREAT, 0644);
                        //if (file_o == ERROR OUTPUT OF OPEN)
                                // use this to throw a relevant error
                        dup2(file_o, STDOUT_FILENO);
                        close(file_o);
                }

                execvp(command->args[0], command->args);
                exit(1);
        }
        else { // parent
                waitpid(pid, &status, 0);
        }

	return WEXITSTATUS(status);
}

int directory_traversal(struct CommandList* command_head) {
        struct CommandList* ptr = command_head;

        while (ptr != NULL) {
                if (!strcmp(ptr->args[0], "cd")) {
                        int ch_status = chdir(ptr->args[1]);
                        return WEXITSTATUS(ch_status);
                }

                if (!strcmp(ptr->args[0], "pwd")) {
                        char cwd[CMDLINE_MAX];
                        getcwd(cwd, sizeof(cwd));
                        printf("%s\n", cwd);
                        return 0;
                }

                ptr = ptr->next;
        }

        return 0;
}

void output_redirection(struct CommandList* command) {
        if (command->path != NULL) {
                int file_o;

                if (command->append == 0)
                        file_o = open(command->path, O_RDWR | O_CREAT, 0644);
                else
                        file_o = open(command->path, 
                                O_RDWR | O_CREAT | O_APPEND,
                                0644);
                //if (file_o == ERROR OUTPUT OF OPEN)
                                // use this to throw a relevant error
                dup2(file_o, STDOUT_FILENO);
                close(file_o);
        }
}

void execute_job(struct CommandList* command_head, int command_counter, int exit_codes[])
{
        struct CommandList* ptr = command_head;

        directory_traversal(ptr);

        if (command_counter == 1) {
                int pid = fork();

                if (pid == 0) { //child
                        output_redirection(ptr);
                        execvp(ptr->args[0], ptr->args);
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
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);

                                output_redirection(ptr);
                                execvp(ptr->args[0], ptr->args);
                                exit(1);
                        } else { // parent 2
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                
                                waitpid(pid, &(exit_codes[0]), 0);

                                ptr = ptr->next;
                                output_redirection(ptr);
                                execvp(ptr->args[0], ptr->args);
                                exit(1);
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
                                        close(fd[0]);
                                        dup2(fd[1], STDOUT_FILENO);
                                        close(fd[1]);

                                        output_redirection(ptr);
                                        execvp(ptr->args[0], ptr->args);
                                        exit(1);
                                } else { //parent 3
                                        close(fd[1]);
                                        dup2(fd[0], STDIN_FILENO);
                                        close(fd[0]);

                                        waitpid(pid, &(exit_codes[0]), 0);

                                        ptr = ptr->next;
                                        output_redirection(ptr);
                                        execvp(ptr->args[0], ptr->args);
                                        exit(1);
                                }
                        } else { // parent 2
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                
                                waitpid(pid, &(exit_codes[1]), 0);

                                ptr = ptr->next->next;
                                output_redirection(ptr);
                                execvp(ptr->args[0], ptr->args);
                                exit(1);
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
                                                close(fd[0]);
                                                dup2(fd[1], STDOUT_FILENO);
                                                close(fd[1]);

                                                output_redirection(ptr);
                                                execvp(ptr->args[0], ptr->args);
                                                exit(1);
                                        } else { // parent 4
                                                close(fd[1]);
                                                dup2(fd[0], STDIN_FILENO);
                                                close(fd[0]);

                                                waitpid(pid, &(exit_codes[0]), 0);

                                                ptr = ptr->next;
                                                output_redirection(ptr);
                                                execvp(ptr->args[0], ptr->args);
                                                exit(1);
                                        }

                                } else { //parent 3
                                        fprintf(stderr, "I should be the second call to exec\n");

                                        close(fd[1]);
                                        dup2(fd[0], STDIN_FILENO);
                                        close(fd[0]);

                                        waitpid(pid, &(exit_codes[1]), 0);

                                        ptr = ptr->next->next;
                                        output_redirection(ptr);
                                        execvp(ptr->args[0], ptr->args);
                                        exit(1);
                                }
                        } else { // parent 2
                                fprintf(stderr, "I should be the third call to exec!\n");

                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                
                                waitpid(pid, &(exit_codes[2]), 0);

                                ptr = ptr->next->next->next;
                                output_redirection(ptr);
                                execvp(ptr->args[0], ptr->args);
                                exit(1);
                        }
                } else { // parent 1
                        waitpid(pid, &(exit_codes[3]), 0);
                }
        } else {
                
        }
}

void stringlist_parse(struct StringList* head_string_list, int* command_counter, char cmd[CMDLINE_MAX])
{
        /* Create pointer to move through list, preserving head */
        struct StringList* current_string_list = (struct StringList*) malloc(sizeof(struct StringList));
        current_string_list = head_string_list;

        char* token = strtok(cmd, "|");
        strcpy(current_string_list->string, token);
        token = strtok(NULL, "|");
        while (token != NULL) {
                ++(*(command_counter));
                struct StringList* new_string_list = (struct StringList*) malloc(sizeof(struct StringList));
                strcpy(new_string_list->string, token);
                current_string_list->next = new_string_list;
                current_string_list = current_string_list->next;
                token = strtok(NULL, "|");
        }
        current_string_list->next = NULL;
}

enum error command_parse(struct CommandList* command, char* cmd_text)
{
        char* raw_cmd = (char*) malloc(sizeof(cmd_text));
        strcpy(raw_cmd, cmd_text);

        /* isolate output redirection path */
        int operator_count = 0; 
        for (int i = 0; i < strlen(raw_cmd); i++) {
                if (raw_cmd[i] == '>')
                        operator_count++;
        }

        if (operator_count > 1) 
                command->append = 1;
        else
                command->append = 0;
        char* w_cmd = strtok(raw_cmd, ">");
        char* redirect_path = strtok(NULL, ">");
        if (redirect_path != NULL) {
                command->path = redirect_path;
        }

        /* isolate/assign command */
        char* token = (char*) malloc(CMDLINE_MAX);
        token = strtok(w_cmd, " ");
        strcpy(command->cmd, token);
        command->args[0] = token;

        token = strtok(NULL, " ");
        int i = 1;
        do {
                if (i == 16)
                        return ERR_TOO_MANY_ARGS;
                command->args[i] = token;
                token = strtok(NULL, " ");
                i++;
        } while (token != NULL);
        command->args[i] = NULL;

        return 0;
}

void commandlist_parse(struct CommandList* head_command, struct StringList* head_string_list) {
        /* Create pointers to move through lists, preserving heads */
        struct CommandList* current_command = (struct CommandList*) malloc(sizeof(struct CommandList));
        current_command = head_command;
        struct StringList* current_string_list  = (struct StringList*) malloc(sizeof(struct StringList));
        current_string_list = head_string_list;

        /* Implicit first call to command parse */
        command_parse(current_command, current_string_list->string);
        current_string_list = current_string_list->next;

        /* Parse the remining strings */
        while(current_string_list != NULL) {
                struct CommandList* next_command = (struct CommandList*) malloc(sizeof(struct CommandList));
                enum error err = command_parse(next_command, current_string_list->string);
                current_command->next = next_command;
                current_command = current_command->next;
                current_string_list = current_string_list->next;
        } 
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;
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
                struct StringList* head_string_list = (struct StringList*) malloc(sizeof(struct StringList));
                stringlist_parse(head_string_list, &command_counter, cmd);

                /* Build a linked list of parsed Command stucts */
                struct CommandList* head_command = (struct CommandList*) malloc(sizeof(struct CommandList));
                commandlist_parse(head_command, head_string_list);

                /*
                if (error_mgmt(err) != ERR) {
                	while (current_command->next != NULL) {
                        	retval = psuedo_system(current_command);
                        	fprintf(stderr, "+ completed '%s' [%d]\n",
                        	cmd_cpy, retval);
                	}
		}
                */

                int exit_codes[command_counter];
                execute_job(head_command, command_counter, exit_codes);

                fprintf(stderr, "+ completed '%s' ", cmd_cpy);
                for (int i = 0; i < command_counter; i++)
                {
                        fprintf(stderr, "[%i]", exit_codes[i]);
                }
                fprintf(stderr, "\n");

                // int exit_code;
                // exit_code = psuedo_system(current_command, command_counter);
                // current_command = current_command->next;

                // fprintf(stderr, "+ completed '%s' ", cmd_cpy);
                // for (int i = 0; i < command_counter; i++) {
                //         fprintf(stderr, "[%i]", exit_codes[i]);
                // }
                // fprintf(stderr, "\n");

                // free(head_command);
                // free(current_command);
                // free(string_list_head);
                // free(string_list_current);
                // free(token);
        }

        return EXIT_SUCCESS;
}
