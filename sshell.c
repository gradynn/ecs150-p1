#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int psuedo_system(char*, char*[]);

struct Command {
	char* whole_cmd;
	char* cmd;
	char* args[];
};
/*
int command_parse(struct Command *command, char* whole_cmd)
{
	command->whole_cmd = whole_cmd;

        command->cmd = strtok(whole_cmd, " ");
        command->args = {NULL, NULL, NULL, NULL,
 		        NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL};
         
	  	command->args[0] = command->cmd;

                char* token = strtok(NULL, " ");
                int i = 1;
		do {
			if (i == 16)
				return 1;
                        command->args[i] = token;
                        token = strtok(NULL, " ");
                	i++;
		} while (token != NULL);

		return 0;
}
*/
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



                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */
		
                strcpy(command.whole_cmd, cmd);
		char* token = strtok(cmd, " ");

              	strcpy(command.cmd, token);
               /* command.args = {NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL};
                */
		strcpy(command.args[0], command.cmd);

                token = strtok(NULL, " ");
                int i = 1;
		do {
                        strcpy(command.args[i], token);
                        token = strtok(NULL, " ");
                        i++;
                } while (token != NULL);
/*
		struct Command *command_ptr = &command;
		int error = command_parse(command_ptr, cmd);
		
		if (error == 1)
			fprintf(stderr, "Error: too many process arguments\n");
*/		
		//int arr_len = sizeof(command.args)/sizeof(command.args[0]);

//		for (int i = 0; i < 2; i++)
//			printf("%s\n", command.args[i]);

                retval = psuedo_system(command.cmd, command.args);
//                printf("2 %s\n", cmd);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        command.whole_cmd, retval);
        }

        return EXIT_SUCCESS;
}

int psuedo_system(char* cmd, char* args[])
{
        int pid = fork();
	int status = 0;

        if (pid == 0) { // child
                execvp(cmd, args);
        }
        else { // parent
                waitpid(pid, &status, 0);
        }

	return WEXITSTATUS(status);
}
