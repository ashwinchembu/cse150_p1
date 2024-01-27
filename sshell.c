#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// MACROS:
#define CMDLINE_MAX 512
#define BIN "/bin/"

// INITALIZED LINKED LISTS STRUCTS

// linkedlist stuff modified from https://www.learn-c.org/en/Linked_lists

typedef struct node {
  char *cmd;
  struct anode *arg;
  struct node *next;
  char *type;
  int nargs;
  int count;
  bool isredirect;
  char *redirect[2];
} node_t;

typedef struct anode {
  char *arg;
  struct anode *next;
} node_a;

typedef struct outputnode {
  int n;
  struct outputnode *next;
} node_o;

// PUSH()

// push command linked list
void push(node_t *head, char *cmd, node_a *args, int nargs, char *type,
          int count, char **redirects) {

  // create new node
  node_t *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  // add properties into command linked list
  current->next = (node_t *)malloc(sizeof(node_t));
  current->next->cmd = cmd;
  current->next->arg = args;
  current->next->nargs = nargs;
  current->next->type = type;
  current->next->count = count;

  if (redirects != NULL) {
    current->next->redirect[0] = redirects[0];
    current->next->redirect[1] = redirects[1];
    current->next->isredirect = true;
  } else {
    current->next->redirect[0] = NULL;
    current->next->redirect[1] = NULL;
    current->next->isredirect = false;
  }
  current->next->next = NULL;
}

// POP()

// node from linked list and free memory
char *pop(node_t **head) {

  char *retval = NULL;
  node_t *next_node = NULL;

  if (*head == NULL || (*head)->cmd == NULL) {
    return NULL;
  }
  next_node = (*head)->next;
  retval = (*head)->cmd;

  // free allocated memory

  if ((*head)->type != NULL) {
    free((*head)->type);
  }

  if ((*head)->isredirect) {
    free((*head)->redirect[0]);
    free((*head)->redirect[1]);
  }

  free((*head)->cmd);
  free(*head);

  // update head pointer to next node
  *head = next_node;

  return retval;
}

// POPALL()

// pop all the values in command and argument linked list
// free all linked list memory
void popall(node_t **head) {
  
  char *clear = NULL;
  if ((*head == NULL || (*head)->cmd == NULL)) {
    clear = (*head)->cmd;
  }
  while (clear != NULL) {
    clear = pop(head);
  }
}

// PEEK()

// return the next command node's value
char *peek(node_t **head) {

  char *retval = NULL;
  node_t *next_node = NULL;

  if (*head == NULL) {
    return NULL;
  }

  next_node = (*head)->next;
  if (next_node == NULL) {
    return NULL;
  }

  retval = next_node->cmd;
  return retval;
}

// PUSHARGS()

// push arguments into argument linked list
void pushargs(node_a *head, char *argin) {

  // create new node
  node_a *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  // add argument information into node
  current->next = (node_a *)malloc(sizeof(node_a));
  current->next->arg = argin;
  current->next->next = NULL;
}

// POPARGS()

// get argument from linked list and pop to free memory
char *popargs(node_a **head) {

  char *retval = NULL;
  node_a *next_node = NULL;

  if (*head == NULL) {
    return NULL;
  }

  // get argument and next node pointer
  next_node = (*head)->next;
  retval = (*head)->arg;

  // free memory
  free(*head);

  // update head pointer to next node
  *head = next_node;

  return retval;
}

// PUSHOUTPUT()

// put the output into linked list
void pushoutput(node_o *head, int output) {

  // iterate to end of output linked list
  node_o *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  // add output to linked list
  current->next = (node_o *)malloc(sizeof(node_o));
  current->next->n = output;
  current->next->next = NULL;
}

// POPOUTPUT()

// get output form linked list and free memory
int popoutput(node_o **head) {

  int retval;
  node_o *next_node = NULL;

  if (*head == NULL) {
    return -999;
  }

  // get value and pointer to next node
  next_node = (*head)->next;
  retval = (*head)->n;

  // free memory and update head node
  free(*head);
  *head = next_node;

  return retval;
}

// CMDLINEPARSE()

// parse through user's inputted command line
// and input command and arguments into respective linked lists
int cmdlineparse(node_t *node, char *commandline) {

  int count = 0;
  const char delimiters[] =
      " "; // deliminter will split the cmd by empty spaces
  char *token;
  char *redirect[2];
  bool isredirectcheck = false;
  char *cmd;

  // get first token
  token = strtok(commandline, delimiters);
  node_t *tempheadc = node;

  // keep walking through rest of tokens until none left to process
  while (token != NULL) {

    // parse token into command linked list
    cmd = strdup(token);
    char *arg;
    token = strtok(NULL, delimiters);

    // initalize associated argument linked list
    node_a *tempheada = malloc(sizeof(node_a));
    tempheada->arg = "TEMP";
    tempheada->next = NULL;
    int nargs = 0;

    // parse arguments into argument linked list
    while (token != NULL && token[0] != '|') {

      // parse tokens as arguments until token is either |, >, >> or NULL
      if (token[0] != '>' && strcmp(token, ">>")) {
        arg = strdup(token);
        nargs++;
        pushargs(tempheada, arg);
        token = strtok(NULL, delimiters);
      }

      else if (token[0] == '>' || strcmp(token, ">>") == 0) {
        char *operation = strdup(token);
        char *filename;
        token = strtok(NULL, delimiters);
        if (token == NULL) {
          filename = "ERR_NO_FILE";
        } else {
          filename = strdup(token);
        }
        redirect[0] = operation, redirect[1] = filename;
        token = strtok(NULL, delimiters);
        isredirectcheck = true;
      }
    }

    // push to overall linkedlist
    count++;
    char *op;
    if (cmd[0] != '>' && strcmp(cmd, ">>") && cmd[0] != '|') {
      op = "command";
    } else {
      op = "operation";
    }
    if (isredirectcheck) {
      push(tempheadc, cmd, tempheada, nargs, op, count - 1, redirect);
    } else {
      push(tempheadc, cmd, tempheada, nargs, op, count - 1, NULL);
    }
    // process special commands
    if (token != NULL) {

      node_a *tempheadempty = malloc(sizeof(node_a));
      tempheadempty->arg = NULL;
      tempheadempty->next = NULL;
      push(tempheadc, token, tempheadempty, 0, "operation", 0, NULL);
      token = strtok(NULL, delimiters);
    }
  }
  free(token);
  return count;
}

// PULLARGS()

// pull a command node's arguments
// and format into args[] array for execv()
char **pullargs(node_t *node) {

  char **arguments = malloc(sizeof(char *) * (node->nargs + 2));
  arguments[0] = node->cmd;
  int i;
  node_a *acurr = node->arg;
  popargs(&acurr);

  // iterate and append to arguments array
  for (i = 1; i < node->nargs + 1; i++) {
    arguments[i] = popargs(&acurr);
  }

  arguments[i] = NULL;

  return arguments;
}

// CLOSEPIPEARRAY()

// close opened pipe
void closepipearray(int pipearray[][2], int nums_cmd) {
  int p;
  for (p = 0; p < nums_cmd - 1; p++) {
    close(pipearray[p][0]);
    close(pipearray[p][1]);
  }
}

// SINGLECOMMAND()

// executes command given head of command node
// for not redirection or piping cases
// has custom implementation for for cd,sls, pwd
void singlecommand(char *last, node_t *start, int pipearray[][2],
                   int nums_cmd) {

  // gets and formats command from given head node
  pid_t pid;
  char def[] = "";
  char *cmd = strdup(start->cmd);
  char cwd[CMDLINE_MAX];
  char appendedcmd[strlen(def) + strlen(cmd)];
  strcpy(appendedcmd, def);
  strcat(appendedcmd, cmd);

  // pulls arguments
  char **arguments = pullargs(start);

  // Not CD, so run the command with arguments
  char *next = peek(&start);
  pid = fork();

  if (pid == 0) { // child

    if (last != NULL && (strcmp(last, "|") == 0)) {
      dup2(pipearray[(start->count - 1)][0], STDIN_FILENO);
    }

    if (next != NULL && (strcmp(next, "|") == 0)) {
      dup2(pipearray[start->count][1], STDOUT_FILENO);
    }

    if (start->isredirect) {

      // if no file given:
      if (strcmp(start->redirect[1], "ERR_NO_FILE") == 0) {
        popall(&start);
        fprintf(stderr, "Error: no output file\n");
        exit(-999);
      }

      else {
        int fd;
        int op;

        // append
        if (strcmp(start->redirect[0], ">>") == 0) {
          op = O_APPEND;
        } else {
          op = O_TRUNC;
        }

        fd = open(start->redirect[1], O_WRONLY | op | O_CREAT, 0644);

        // if cannot open output file:
        if (fd == -1) {
          fprintf(stderr, "Error: cannot open output file\n");
          popall(&start);
          exit(-999);
        } else {
          dup2(fd, STDOUT_FILENO);
        }
      }
    }

    closepipearray(pipearray, nums_cmd);

    // CUSTOM SLS implementation:
    if (strcmp(cmd, "sls") == 0) {

      // sls modified from
      // https://man7.org/linux/man-pages/man3/scandir.3.html

      struct dirent **namelist;
      int n;

      getcwd(cwd, sizeof(cwd));
      if (strlen(cwd) != 0) {
        n = scandir(cwd, &namelist, NULL, NULL);
        if (n == -1) {
          fprintf(stderr, "Error: cannot open directory\n");
          popall(&start);
          exit(-999);
        }
        while (n--) {
          struct stat file_status;
          if (stat(namelist[n]->d_name, &file_status) == 0) {
            printf("%s (%ld bytes)\n", namelist[n]->d_name,
                   file_status.st_size);
          }

          // free memory
          free(namelist[n]);
        }

        // free rest of memory
        free(namelist);
        popall(&start);
        free(arguments);

        exit(0);

      } else { // catch case for errors
        fprintf(stderr, "Error: cannot open directory\n");

        // free rest of memory
        popall(&start);
        free(arguments);

        exit(1);
      }
    }

    // Custom PWD impelmentation:
    else if (strcmp(cmd, "pwd") == 0) {

      getcwd(cwd, sizeof(cwd));
      popall(&start);
      printf("%s\n", cwd);

      free(arguments); // free memory

      exit(0);
    }

    // For other commands:
    else if (strcmp(cmd, "cd") != 0) {

      popall(&start);
      execvp(cmd, arguments);

      // if execv() failed:
      fprintf(stderr, "Error: command not found\n");

      free(arguments); // free memory

      exit(1);
    }

  } // end of child

  else if (pid > 0) { // parent
    free(cmd);
    free(arguments);

  } // end of parent

  else { // fork failed
    perror("fork");

    // free memory
    popall(&start);
    free(arguments);

    exit(1);
  }
}

// ERRORHANDLER()

// iterates through linked lists to check if valid input was given
// returns true is valid, returns false if not
bool errorhandler(node_t *start, char *rawcmd) {

  pid_t pid = fork();

  if (pid == 0) { // child

    // too many process args
    node_t *procargs = start;
    while (procargs != NULL) {
      if (procargs->nargs >= 16) {
        fprintf(stderr, "Error: too many process arguments\n");
        popall(&start);
        exit(1);
        break;
      }
      procargs = procargs->next;
    }

    // missing command
    node_t *Lcommand = start;
    node_t *Rcommand;

    // see if its a command and chained
    while (Lcommand->next != NULL && strcmp(Lcommand->type, "command") == 0) {
      node_t *op = Lcommand->next;

      // see if theres an value after the operation
      if (op->next != NULL) {

        // something after operation
        Rcommand = op->next;
        if (op->cmd[0] == '|') {                        // checking if pipe
          if (strcmp(Rcommand->type, "command") != 0) { // no command after pipe
            fprintf(stderr, "Error: missing command\n");
            popall(&start);
            exit(1);
            break;
          }
        }
      } else {
        // nothing after operation
        if (op->cmd[0] == '|') {
          // checking if pipe
          fprintf(stderr, "Error: missing command\n");
          popall(&start);
          exit(1);
          break;
        }
      }
      Lcommand = Rcommand;
    }

    // if first is operation
    if (strcmp(Lcommand->type, "command") != 0) {
      fprintf(stderr, "Error: missing command\n");
      popall(&start);
      exit(1);
    }

    // if > or >> comes before |
    char *write = strchr(rawcmd, '>');
    char *pipe = strchr(rawcmd, '|');

    if (write != NULL && pipe != NULL && (write < pipe)) {
      fprintf(stderr, "Error: mislocated output redirection\n");
      popall(&start);
      exit(1);
    }

    exit(0);

  } // end of child

  else if (pid > 0) { // parent
    int status;
    waitpid(pid, &status, 0);

    if (WEXITSTATUS(status) == 0) {
      return true;
    } else {
      return false;
    }

  } // end of parent

  else { // fork failed catch case
    perror("fork");

    exit(1);
  }
}

// TRIMTRAIL()

// trims ending white spaces from input string
void trimtrail(char *str) {
  int length = strlen(str);
  int i = length - 1;
  while (str[i] == ' ') {
    i--;
  }

  str[i + 1] = '\0';
}

// ADJSPACING()

// formats user inputted comand line for parsing
// in case user doesn't space command line properly
char *adjspacing(char *str) {
  int length = strlen(str);
  int newlength = 0;
  int i;

  // iterates through user inputted command line
  for (i = 0; i < length; i++) {
    if (str[i] == '>' || str[i] == '|') {
      newlength++;
      newlength++;
      if (str[i] == '>' && i + 1 < length && str[i + 1] == '>') {
        i++;
      }
    }
  }

  // creates new string with adjusted spacing
  char *spacedcmd = malloc(CMDLINE_MAX + newlength);
  int spacedi = 0;

  // iterates through cmdline to put into spacecmd
  for (i = 0; i < length; i++) {
    if (str[i] == '>' || str[i] == '|') {
      spacedcmd[spacedi] = ' ';
      spacedcmd[spacedi + 1] = str[i];
      spacedi += 2;

      if (str[i] == '>' && i + 1 < length && str[i + 1] == '>') {
        spacedcmd[spacedi] = '>';
        spacedi++;
        i++;
      }

      spacedcmd[spacedi] = ' ';
      spacedi++;

    } else {
      spacedcmd[spacedi] = str[i];
      spacedi++;
    }
  }

  spacedcmd[spacedi] = '\0';

  return spacedcmd;
}

// MAIN()

int main(void) {

  char cmd[CMDLINE_MAX];
  char *rawcmd;
  char *newcmd;

  while (1) {
    bool file = false;
    char *nl;

    // Prints Prompt
    printf("sshell$ ");
    fflush(stdout);

    // Get User Inputted Command Line
    fgets(cmd, CMDLINE_MAX, stdin);

    // Print command line if stdin is not provided by terminal
    if (!isatty(STDIN_FILENO)) {
      printf("%s", cmd);
      fflush(stdout);
    }

    // Remove trailing newline from command line
    nl = strchr(cmd, '\n');
    if (nl)
      *nl = '\0';
    // trailing spaces for no args

    // EXIT() Command
    if (!strcmp(cmd, "exit")) {
      fprintf(stderr, "Bye...\n");
      fprintf(stderr, "+ completed '%s' [0]\n", cmd);
      break;
    }

    // stores rawcmd for printing at the end
    rawcmd = strdup(cmd);

    trimtrail(cmd);

    newcmd = adjspacing(cmd);
    // Initalize Linked Lists
    node_t *head = malloc(sizeof(node_t));
    node_o *ocurr = malloc(sizeof(node_o));
    head->cmd = NULL;
    head->next = NULL;
    ocurr->next = NULL;

    // Check for valid input
    // if starts with '>', '>>', '|' or '\0', invalid input

    if (rawcmd[0] == '>' || rawcmd[0] == '|' || newcmd[0] == '\0') {
      fprintf(stderr, "Error: missing command\n");

      free(rawcmd); // free memory
      continue;

    } else { // if valid:

      int nums_cmd = cmdlineparse(head, newcmd);
      node_t *start = head->next;

      // if passes all other error handler cases, continue
      if (errorhandler(start, rawcmd)) {
        // check cd
        if (strcmp(start->cmd, "cd") == 0) {
          char **arguments = pullargs(start);
          int ret;
          if (arguments[1] == NULL) {
            ret = chdir("/");
            if (ret != 0) {
              perror("chdir");
              ret = 1;
              popall(&start);
              fprintf(stderr, "Error: cannot cd into directory\n");
            }
          } else {
            ret = chdir(arguments[1]);
            if (ret != 0) {
              ret = 1;
              popall(&start);
              fprintf(stderr, "Error: cannot cd into directory\n");
            }
          }
          pushoutput(ocurr, ret);
          // break;
        } else {
          // printf("test");
          //  initalize pipes based on number of commands given
          int pipearray[nums_cmd - 1][2];
          int p;
          for (p = 0; p < nums_cmd - 1; p++) {
            pipe(pipearray[p]);
          }

          char *last = head->cmd;
          while (start != NULL) {
            if (strcmp(start->cmd, "|") != 0) {
              singlecommand(last, start, pipearray, nums_cmd);
            }
            last = strdup(start->cmd);
            start = start->next;
          }
          free(last);

          // close pipes
          closepipearray(pipearray, nums_cmd);
        }
        int c;
        for (c = 0; c < nums_cmd; c++) {

          int status;
          waitpid(WAIT_ANY, &status, 0);

          if (status != 6400) {
            pushoutput(ocurr, WEXITSTATUS(status));
          }
        }

        // Free Memory
        popall(&head);
        int value = 0;
        ocurr = ocurr->next;

        // print output

        if (file) {
          nums_cmd--;
        }

        int i;
        for (i = nums_cmd; i > 0; i--) {
          value = popoutput(&ocurr);
          if (value != -999) {
            if (i == nums_cmd) {
              fprintf(stderr, "+ completed '%s' ", rawcmd);
            }
            fprintf(stderr, "[%d]", value);
            if (i == 1) {
              fprintf(stderr, "\n");
            }
          }
        }

        popoutput(&ocurr);
        popoutput(&ocurr);

      } else { // if user input is not valid (fails errorhandler())

        // free memory
        free(rawcmd);

        continue;
      }
    }

    // free memory
    free(rawcmd);
    free(newcmd);
  } // end of while(1) loop (end of shell)

  return EXIT_SUCCESS;
}