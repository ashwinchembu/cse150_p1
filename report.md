# SSHELL : A simple implementation of a shell

## Summary:

This program implements a simple UNIX shell system. The user 
types in commands with arguments they would like to execute 
and the program executes them. It employs built in commands 
for cd, pwd, sls and exit. It calls execv() for other commands. 
It also handles piping (up to 4 commands) and output redirection
into a file.

## Implementation:

The implementation can be broken down into 4 main steps:
1. Parse the user-inputted command line into 2 link-lists. 
2. Check for Valid Input.
3. Iterate through the linked lists and execute the commands.
4. Free allocated memory.


### Data Stucture and Linked Lists:
The program parses the user-inputted command line into 2 linked lists. 
The first is the command linked list which stores the command or 
operation (pipping "|", redirection ">" or ">>"). Second, for each
command node in the command linked list is an argument linked list that 
stores the arguments. 

Using a linked list to store both commands and any given arguments 
for those commands allows for flexibility with the dynamic allocation
of link lists. Meaning, whether the user inputs 4 commands with 
multiple arguments and pipping, or one simple command such as 
"echo hello," the program can parse, store and handle a variety
of inputs from the user.

The command link list is implemented in a struct named "node." 
Each command node has a string (char*) named "cmd" (of max length 
CMDLINE_MAX) that holds the command the user wishes to execute
(ex. "echo", "ls", "pwd", etc.). To further assist with the 
logistics of the program, each command node contains an int
variable called "nargs" and "count" to track how many arguments
that command has, and how many total commands the user gave. If there is a 
redirect after a command, the variables isredirect and an array of the 
specific redirect command and filename are stored as well.

To connect these command nodes and create that linked list
feature, the command node contains a pointer to the next
command. This allows for the program to iterate through the
all of the stored commands the node's properties given only
the head of the command node. 

Furthermore, since each command could be given any number of 
arguments, from 0 arguments (ex. ls), or multiple arguments 
(ex. echo ecs hello world), each command node contains a pointer
to the head of its argument linked list. This again allows for 
the program to handle a wide variety of inputs from the user, 
for flexibility and versatility. 

The argument linked list is implemented in a similar structure
to the command linked list, in a struct named "anode." Each
argument node contains a string (char *) called "arg" to 
hold the argument of its command (ex. "hello" for "echo"),
and a pointer to the next argument node (which is NULL if
there are no more arguments). 

### Parsing User-inputted Command Line

To parse the user-inputted command line and its commands,
operations (piping, redirection), and associated arguments 
into the linked lists, the program employs the strtok() function.
With delimiters[] = " ", the program grabs a token from the 
command line and while that token isn't null (until we run out 
of information the user inputted), the program allocates a new 
command node, attaches it to the head, and adds the token as the
command. To ensure that we can determine that piping and redirection are 
being counter when there may not be spaces, adjspacing() runs before
cmdline parse to make sure that they are appropriately separated.

It keeps iterating through the command line and inputs the tokens
into the argument linked list of that command, until it reaches 
either a pipe ("|"), redirection (">",">>"), or the end (NULL). Once it 
reaches the redirection, it updates the redirect array in the command
node_t in order to hold the type of redirect and the filename.
This marks the end of that command's arguments. It stores the
next token (if not null) as the operation (pipe)
into the next command head, and repeats the process. This captures
and stores each command, its arguments, and the operation to 
perform next in chronological order, iterating through the
command linked list. 


### Executing commands and Piping

Before executing the commands, the linked list is run through a function and 
smaller tests in main() in order to ensure that it is a valid input
(commands in wrong places, too many args, etc.).

To execute the commands, the program is given the head of the command
linked list, and iterates through until the pointer to the next node
is NULL (meaning this is the last command to execute that the user
inputted). 

The command is pulled from the char *cmd stored in the current command 
node. It pulls the arguments of the current command node the program 
is on through the function pullargs(node_t *node). It iterates through 
the arguments given the pointer of the head argument node and stores 
them into a formatted array for execvp(cmd, args[]) called arguments. 

CD is the only command performed without forking because it requires
a change of directory. If this was performed in the child process
of a fork(), the change in the directory would not be sustained because after
the child process dies, the program reverts back to the parent process.

Before the program is ready to execute the command, it first checks
whether the output of the current command needs to be pipped into the 
next command or has to be redirected. It does so by peeking at the next 
command node through the peek() function which returns the value stored at 
char *cmd of the next node. If that value is a pipe ("|"), the program will 
open a pipe and redirect the output from STD_OUT to the pipe for the next 
command to use. If there is a redirect value in the redirect array of the 
node, the function pipes stdout to the file directly.

After the program executes, if a pipe is next, the next
command can then take that output and use it for its execution. The 
process repeats until the last command is performed. 


### Memory Allocation:

The command and argument linked lists both employ malloc for their
dynamic memory allocation. To manage memory and to prevent any 
memory leaks, any and all allocated memory is freed carefully.
Whenever the program grabs the command and argument from the linked
lists, it "pops" that node. Meaning, that memory is cleared as the program
iterates when that node is no longer needed.

Furthermore, not only is memory cleared throughout iterating through
the linked lists, but also when forking. Whenever a child is creating 
through fork(), that child contains the copies of all its parent's 
information, including the data structures. Therefore, the program
runs a function called popall() which clears all the nodes in the
command linked list, and all associated argument linked lists before
the end of each child that's created. 

## Testing

In order to test our project, we utilized various commands that had no 
arguments, commands with the maximum argument limit, various cases of piping 
and redirection/appending, and finally the edge cases in the project1.
html. The greatest resource was the tester.sh which helped pinpoint where 
functionality was lacking. Finally, ChatGPT was heavily used to generate a 
plethora of edge test cases and test scripts to ensure the code worked to 
the specifications.

## Work of:

This program is the work of Ashwin Cheembu and Manreet Sohi


## Resources:
https://www.learn-c.org/en/Linked_lists
https://man7.org/linux/man-pages/man3/scandir.3.html


