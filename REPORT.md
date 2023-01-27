### Project 1, ECS 150, Joël Porque-Lupine
### Gradyn Nagle, Chase Keighley
### 01-27-22
***
## Project Report: `sshell.c`
### **v1.0.0**
Our implentation for this project relies on three main phases of execution. The
first phase is the parsing of the input command into seperated strings. The
second phase is the parsing of each input command into a struct that contains
fields relevant to the execution of each command. The third phase is the
exceution of each command in order.

### Structures
The structures we used for this project are as follows:
```
struct Command {
    char* args[16];
    char* path;
    int append;
    struct Command* next;
}
```
+ The Command struct is used to store the parsed input command. The args field
contains the command keyword itself and all arguments in order. This is what is
passed to execvp(). The path field contains the path to the executable file as
specified following a > or >>. The append field is a boolean that is set to 1 if
the command output is to be appended to the file specified by path. The next 
field is a pointer to another Command struct. This allows the storing of 
successive piped commands in a lined list.

```
struct CommandString {
    char string[CMDLINE_MAX];
    struct CommandString* next;
}
```
+ The `CommandString` struct is used to store each input command as a string 
(that is, the entire command before it is parsed). It contains a pointer to the 
next CommandString struct, allowing the storing of successive input commands in 
a linked list.

### Primary Functions
The execution `sshell.c` is handled by endlessly repeating while loop. It 
outputs the shell prompt, reads the input commands, parses the input commands,
then executes the parsed commands. The main function relies on the 
following functions:
```
enum error stringlist_parse(struct CommandString* head_command_string, 
    int* command_counter, 
    char cmd[CMDLINE_MAX]);
```
+ This function recieves a pointer to the head of a linked list of 
`CommandStrings`, a pointer to an integer that will store the number of commands
piped together, and a string containing the input command. It parses the input
using the delimeter "|", and stores each command in a `CommandString` struct. It
then increments the `command_counter` by 1 for each command parsed. It returns an
error representing the success or failure of the function.

```
enum error commandlist_parse(struct CommandString* head_command, 
    struct Command* head_command_string);
```
+ This function recieves a pointer to the head of a linked list of `Command`
structures and a pointer to the head of the linked list of `CommandStrings` 
(previously initialized in `stringlist_parse`). It iterates through the list of
unparsed command strings dynamically creating a new `Command`, passing that 
Command and the current `CommandString` to `command_parse`, and then appending
the new `Command` to the end of the linked list. It returns an error 
representing the success or failure of the function.

```
command_parse(struct Command* command, struct CommandString* command_string);
```
+ This function recieves a pointer to a `Command` struct and a pointer to a 
`CommandString` struct. It parses the `CommandString` string using the delimeter
" ", and stores each argument in the `Command` struct. It then checks for 
various special characters, such as ">" and ">>", and sets the appropriate
flags within the command struct. It uses logical checks to detect input errors
and returns an error representing the success or failure of the function.

```
enum error execute_job(struct Command* command_head, 
    int command_counter, 
    int exit_codes[]);
```
+ This function is repsonsible for executing the parsed commands. It recieves a
pointer to the head of the linked list of `Command` structs, the number of 
commands piped together, and an array of integers that will store the exit codes
of each commands execution. Based on the number of commands piped together, it
uses branching to fork the program an approiate number of times so that there
n = `command_counter` processes. Before each successiive call to `fork()`, it
uses `pipe()` to create a pipe between the current process and the next process.
The output of the current process is redirected to the input of the next process
using `dup2()`. The first process in the chain is executed using `execvp()` in
the most recent child process. The parent process waits for the child process to
and stores the exit code in the `exit_codes` array. The parent process then 
executes the next command in the chain. The last process in the chain is
executed in a process that a child of the original process containing the shell.



### Helper Functions
The following functions are used to assist in the execution of the primary
functions:
```
enum error directory_traversal(struct Command* command_head);
```
+ THis function handles all directory traversal commands. It recieves a pointer
to the head of the `Command` linked list. It checks the first argument of each
command to see if it is a directory traversal command. If it is, it executes the
comand from the main process before the program has a chance to fork to a new
process. It returns an error representing the success or failure of the 
function.

```
enum error output_redirection(struct Command* command);
```
+ This function handles all output redirection commands. It recieves a pointer
to the a `Command` struct. It uses conditional branching to determine if the
command contains redirection and a valid path (if path is non-null). If the 
command path is valid, it opens the file using `open()` and redirects the
output of the current process to the file using `dup2()`. Depending on the 
status of the append flag, it opens the file either in O_APPEND if appropraite. 
It returns an error representing the success or failure of the function.

### Error Handling
The following errors are handled by the program, and are defined using 
enumeration. The function below reveieves an error and prints the appropriate
error message to stderr.
```
enum error error_mgmt(enum error err);
```
+ `ERR_MANY_ARGS` - Too many process arguments
+ `ERR_MISS_CMD` - Missing command
+ `ERR_NO_OUT` - No output file
+ `ERR_CANT_OPEN_OUT` - Cannot open output file
+ `ERR_OUT_REDIR` - Misplocated output redirection
+ `ERR_CANT_CD` - Cannot cd into directory
+ `ERR_CMD_NOT_FOUND` - Command not found
+ `ERR` - Generalized error
+ `NO_ERR` - No error

### Testing
We tested our program using the commands provided in the assignment description
as well as a variety of other commands used on UNIX systems. We also tested our
program using the shell script provided by Professor Porquet-Lupine. In order to
ensure compatibility with the CSIF computers, we copied our program to them
multiple times throughout the development process and tested it there.

### Sources
1. Porquet-Lupine, Joel. “System Calls.” ECS 150. University of California, 
    Davis, 2023. Lecture.
    + Consulted heavily for the use of system calls and concepts like process
    forking and piping.

2. Kerrisk, Micheal. “man7.Org.” Michael Kerrisk, https://man7.org/index.html. 
    + Consulted for details descriptions of system calls and their parameters.
