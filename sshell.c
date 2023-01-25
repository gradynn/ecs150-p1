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

struct Command {
	char cmd[CMDLINE_MAX];
	char* args[16];
        char* path;
	struct Command* next;
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

int psuedo_system(struct Command* command)
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

        int pid = fork();
	int status = 0;

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

enum error command_parse(struct Command* command, char* cmd_text)
{
        /* isolate output redirection path */
        char* raw_cmd = (char*) malloc(sizeof(cmd_text));
        strcpy(raw_cmd, cmd_text);
        char* w_cmd = strtok(raw_cmd, ">");
        char* redirect_path = strtok(NULL, ">");
        if (redirect_path != NULL)
                command->path = redirect_path;

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

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;
                struct Command* head_command = (struct Command*) malloc(sizeof(struct Command));
	        struct Command* current_command = (struct Command*) malloc(sizeof(struct Command));
                int command_counter = 1;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                char cmd_cpy[CMDLINE_MAX];
                strcpy(cmd_cpy, cmd);

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Build a linked list of the command strings seperated by the
                        | operator */
                struct StringList* string_list_head = (struct StringList*) malloc(sizeof(struct StringList));
                struct StringList* string_list_current = (struct StringList*) malloc(sizeof(struct StringList));
                string_list_head = string_list_current;
                char* token = strtok(cmd, "|");
                strcpy(string_list_current->string, token);
                token = strtok(NULL, "|");
                while (token != NULL) {
                        command_counter++;
                        struct StringList* string_list_new = (struct StringList*) malloc(sizeof(struct StringList));
                        strcpy(string_list_new->string, token);
                        string_list_current->next = string_list_new;
                        string_list_current = string_list_current->next;
                        token = strtok(NULL, "|");
                }
                string_list_current->next = NULL;

                /* Build a linked list of parsed Command stucts */
                string_list_current = string_list_head;
                head_command = current_command;
                enum error err = command_parse(current_command, string_list_current->string);
                string_list_current = string_list_current->next;
                while(string_list_current != NULL) {
                        struct Command* next_command = (struct Command*) malloc(sizeof(struct Command));
                        enum error err = command_parse(next_command, string_list_current->string);
                        current_command->next = next_command;
                        current_command = current_command->next;
                        string_list_current = string_list_current->next;
                }

                /* Iterate through list of commands and execute sudo system
                        each time */
		current_command = head_command;

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
                int i = 0;
                while (current_command != NULL) {
                        retval = psuedo_system(current_command);
                        exit_codes[i] = retval;
                        i++;
                        current_command = current_command->next;
                }

                fprintf(stderr, "+ completed '%s' ", cmd_cpy);
                for (i = 0; i < command_counter; i++) {
                        fprintf(stderr, "[%i]", exit_codes[i]);
                }
                fprintf(stderr, "\n");

                free(head_command);
                free(current_command);
                free(string_list_head);
                free(string_list_current);
                free(token);
        }

        return EXIT_SUCCESS;
}
