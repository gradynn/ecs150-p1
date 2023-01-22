#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512

int psuedo_system(char*, char*[]);

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
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
                char input[CMDLINE_MAX];
                strcpy(input, cmd);
                char* command = strtok(input, " ");
                char* args[16] = {NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL};
                args[0] = command;

                char* token = strtok(NULL, " ");
                int i = 1;
                do {
                        args[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                } while (token != NULL);


                retval = psuedo_system(command, args);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}

int psuedo_system(char* cmd, char* args[])
{
        int pid = fork();
        if (pid == 0) { // child
                execvp(cmd, args);
        }
        else { // parent
                int status;
                waitpid(pid, &status, 0);
                return WEXITSTATUS(status);
        }
}


