#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int psuedo_system(char*, char*[]);

struct Command {
	char* cmd;
	char* args[16];
};

int command_parse(struct Command* command, char* w_cmd)
{
        /* isolate/assign command */
        char* token = strtok(w_cmd, " ");
        strcpy(command->cmd, token);
        command->args[0] = token;

        token = strtok(NULL, " ");
        int i = 1;
        do {
                if (i == 16)
                        return 1;
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
		struct Command command;
                char *nl;
                int retval;

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

                char* cmd_cpy;
                strcpy(cmd_cpy, cmd);

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */
		struct Command* command_ptr = &command;
		command_parse(command_ptr, cmd);

		/*
		if (error == 1)
			fprintf(stderr, "Error: too many process arguments\n");
                		
		int arr_len = sizeof(command.args)/sizeof(command.args[0]);

		for (int i = 0; i < 2; i++)
			printf("%s\n", command.args[i]);
*/

                retval = psuedo_system(command.args[0], command.args);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd_cpy, retval);
        }

        return EXIT_SUCCESS;
}

int psuedo_system(char* cmd, char* args[])
{
        /* Handles change of directory without creating new process */
        if (!strcmp(cmd, "cd")) {
                int ch_status = chdir(args[1]);
                return WEXITSTATUS(ch_status);
        }

        int pid = fork();
	int status = 0;

        if (pid == 0) { // child
                execvp(cmd, args);
                exit(1);
        }
        else { // parent
                waitpid(pid, &status, 0);
        }

	return WEXITSTATUS(status);
}
