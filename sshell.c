#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define BIN "/bin/"

// linkedlist stuff modified from https://www.learn-c.org/en/Linked_lists

// initialize linked lists

typedef struct node {
  char *cmd;
  struct anode *arg;
  struct node *next;
  char *type;
  int nargs;
  int count;
} node_t;

typedef struct anode {
  char *arg;
  struct anode *next;
} node_a;

typedef struct outputnode {
  int n;
  struct outputnode *next;
} node_o;

// push, pop, popall and peek for commands linked lists

void push(node_t *head, char *cmd, node_a *args, int nargs, char *type,
          int count) {
  //printf("cmd: %s, count: %d\n",cmd, count);
  node_t *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  /* now we can add a new variable */
  current->next = (node_t *)malloc(sizeof(node_t));
  current->next->cmd = cmd;
  current->next->arg = args;
  current->next->nargs = nargs;
  current->next->type = type;
  current->next->count = count;
  current->next->next = NULL;
}

char *pop(node_t **head) {
  //printf("popping: %s\n", (*head)->cmd);
  char *retval = NULL;
  node_t *next_node = NULL;

  if (*head == NULL) {
    return NULL;
  }

  next_node = (*head)->next;
  retval = (*head)->cmd;
  free(*head);
  *head = next_node;

  return retval;
}

void popall(node_t **head) {
  // printf("poping all\n");
  char *clear = "clear";
  if (*head == NULL) {
    return;
  }
  while (clear != NULL) {
    clear = pop(head);
  }
  return;
}

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

// push and pop for arguments linked lists

void pushargs(node_a *head, char *argin) {
  node_a *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  /* now we can add a new variable */
  current->next = (node_a *)malloc(sizeof(node_a));
  current->next->arg = argin;
  current->next->next = NULL;
}

char *popargs(node_a **head) {
  char *retval = NULL;
  node_a *next_node = NULL;

  if (*head == NULL) {
    return NULL;
  }

  next_node = (*head)->next;
  retval = (*head)->arg;
  free(*head);
  *head = next_node;

  return retval;
}
// push and pop for output linked lists
void pushoutput(node_o *head, int output) {
  // printf("pushing\n");
  node_o *current = head;
  while (current->next != NULL) {
    current = current->next;
  }

  /* now we can add a new variable */
  current->next = (node_o *)malloc(sizeof(node_o));
  current->next->n = output;
  current->next->next = NULL;
}

int popoutput(node_o **head) {
  int retval;
  node_o *next_node = NULL;

  if (*head == NULL) {
    return -999;
  }

  next_node = (*head)->next;
  retval = (*head)->n;
  free(*head);
  *head = next_node;

  return retval;
}

int cmdlineparse(node_t *node, char *commandline) {
  int count = 0;
  const char delimiters[] = " ";
  char *token;

  /* get the first token */
  token = strtok(commandline, delimiters);
  node_t *tempheadc = node;
  /* walk through other tokens */
  while (token != NULL) {
    // parse ommand
    char *cmd = strdup(token);

    token = strtok(NULL, delimiters);
    node_a *tempheada = malloc(sizeof(node_a));
    tempheada->arg = "TEMP";
    tempheada->next = NULL;
    int nargs = 0;
    while (token != NULL && token[0] != '>' && token[0] != '|' &&
           strcmp(token, ">>")) {
      // parse args until pipe, redirection or end
      char *arg = strdup(token);
      nargs++;
      pushargs(tempheada, arg);
      token = strtok(NULL, delimiters);
    }
    // push to overall linkedlist
    count++;
    push(tempheadc, cmd, tempheada, nargs, "command", count - 1);
    // printf("adding command: %d\n", tempheadc->count);
    //  special commands
    if (token != NULL) {
      node_a *tempheadempty = malloc(sizeof(node_a));
      tempheadempty->arg = NULL;
      tempheadempty->next = NULL;
      push(tempheadc, token, tempheadempty, 0, "operation", 0);
      /*
      printf("\n");
      printf("special: %s", token);
      printf("\n");
      */
      token = strtok(NULL, delimiters);
    }
  }
  return count;
}

char **pullargs(node_t *node) {
  char **arguments = malloc(sizeof(char *) * (node->nargs + 2));
  arguments[0] = node->cmd;
  int i;
  node_a *acurr = node->arg;
  // pop dummy "TEMP" arg
  popargs(&acurr);
  // iterate and append to arguments array
  for (i = 1; i < node->nargs + 1; i++) {
    // printf("arg[%d]: %s\n", i, acurr->arg);
    arguments[i] = popargs(&acurr);
  }
  arguments[i] = NULL;
  return arguments;
}
void closepipearray(int pipearray[][2], int nums_cmd) {
  int p;
  for (p = 0; p < nums_cmd - 1; p++) {
    close(pipearray[p][0]);
    close(pipearray[p][1]);
  }
}
void redirect(char *filename, char *rawoperation) {
  //char output[CMDLINE_MAX];
  char buf[CMDLINE_MAX];
  char filteredoperation[3];
  // printf("rawoperation: %s\n", rawoperation);
  if (strcmp(rawoperation, ">") == 0) {
    strcpy(filteredoperation, "w");
  } else if (strcmp(rawoperation, ">>") == 0) {
    // char* file_name = linky[i].cmd
    strcpy(filteredoperation, "a");
  }
  // printf("filtered operation: %s\n", filteredoperation);
  //  output[0] = '\0';
  FILE *file = fopen(filename, filteredoperation);
  if (file == NULL) {
    // perror("file does not exist");
    exit(-999);
  }

  while (fgets(buf, CMDLINE_MAX, stdin)) {
    // strcpy(output, buf);
    fprintf(file, "%s", buf);
  }
  // printf("output: %s \n", output);
  fclose(file);
  exit(-999);
}
void singlecommand(char *last, node_t *start, node_o *outputhead,
                   int pipearray[][2], int nums_cmd) {
  // not redirection or piping
  // printf("%s",cmd);
  // printf("no redirect\n");

  pid_t pid;
  char def[] = "";
  char *cmd = strdup(start->cmd);
  char cwd[CMDLINE_MAX];
  char appendedcmd[strlen(def) + strlen(cmd)];
  strcpy(appendedcmd, def);
  strcat(appendedcmd, cmd);
  //char output[CMDLINE_MAX];

  // printf("%s\n",appendedcmd);
  char **arguments = pullargs(start);
  // cd doesn't need fork so check first:
  if (strcmp(cmd, "cd") == 0) {
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
    // fprintf(stderr, "+ completed '%s' [%d]\n", rawcmd, ret);
    pushoutput(outputhead, ret);
  }
  // not cd, so run cmd with args
  // printf("going to fork: %s\n", cmd);
  char *next = peek(&start);
  // printf("going to fork: %s, %s, %s \n", last, cmd, next);
  pid = fork();
  if (pid == 0) {
    // printf("inside child: %d, cmd: %s\n", getpid(), cmd);
    if (next != NULL && (strcmp(next, "|") == 0 || strcmp(next, ">") == 0 ||
                         strcmp(next, ">>") == 0)) {
      // printf("pipe out\n");
      dup2(pipearray[start->count][1], STDOUT_FILENO);
      // fprintf(stdout, "piped out\n");
    }
    if (last != NULL && (strcmp(last, "|") == 0 || strcmp(last, ">") == 0 ||
                         strcmp(last, ">>") == 0)) {
      // printf("pipe in\n");
      dup2(pipearray[(start->count - 1)][0], STDIN_FILENO);
      // printf("piped in\n");
    }
    closepipearray(pipearray, nums_cmd);
    // check if redirect and write to respective fi
    // printf("done piping\n");
    if (last != NULL && (strcmp(last, ">") == 0 || strcmp(last, ">>") == 0)) {
      // dup2(fd[0], STDIN_FILENO);
      // if (strcmp(last, ">") == 0) {
      char *name = pop(&start);
      redirect(name, last);
      /*
      char buf[CMDLINE_MAX];
      // output[0] = '\0';
      char *name = pop(&start);
      FILE *file = fopen(name, "w");
      if (file == NULL) {
        // perror("file does not exist");
        exit(-999);
      }

      while (fgets(buf, CMDLINE_MAX, stdin)) {
        strcpy(output, buf);
        fprintf(file, "%s", output);
      }
      // printf("output: %s \n", output);

      fclose(file);
      exit(-999);
    }
    */
      /*
      // concatenation >>
      else if (strcmp(last, ">>") == 0) {
        // char* file_name = linky[i].cmd

        char buf[CMDLINE_MAX];
        output[0] = '\0';
        char *name = pop(&start);
        FILE *file = fopen(name, "a");

        if (file == NULL) {
          // perror("file does not exist");
          exit(-999);
        }
        while (fgets(buf, CMDLINE_MAX, stdin)) {
          strcpy(output, buf);
          fprintf(file, "%s", output);
        }
        // printf("output: %s \n", output);

        fclose(file);
        exit(-999);
      }*/
    }
    // check if sls
    else if (strcmp(cmd, "sls") == 0) {
      // sls modified from
      // https://man7.org/linux/man-pages/man3/scandir.3.html
      struct dirent **namelist;
      int n;
      getcwd(cwd, sizeof(cwd));
      if (strlen(cwd) != 0) {
        n = scandir(cwd, &namelist, NULL, NULL);
        if (n == -1) {
          perror("scandir");
          popall(&start);
          exit(1);
        }
        while (n--) {
          struct stat file_status;
          if (stat(namelist[n]->d_name, &file_status) == 0) {
            printf("%s (%ld bytes)\n", namelist[n]->d_name,
                   file_status.st_size);
          }

          free(namelist[n]);
        }
        free(namelist);
        popall(&start);
        exit(0);
      } else {
        fprintf(stderr, "Error: cannot open directory\n");
        popall(&start);
        exit(1);
      }
    }
    // check if pwd
    else if (strcmp(cmd, "pwd") == 0) {
      getcwd(cwd, sizeof(cwd));
      popall(&start);
      printf("%s\n", cwd);
      exit(0);
    }
    // anything else
    else if (strcmp(cmd, "cd") != 0) {
      /*
      char* buf[CMDLINE_MAX];
      printf("get from pipe in\n");
      while (fgets(buf, CMDLINE_MAX, stdin)) {
        //strcpy(output, buf);
        printf("%s\n", buf);
      }
      */
      // printf("command: %s\n", cmd);
      popall(&start);
      execvp(cmd, arguments);
      // perror("execvp");
      fprintf(stderr, "Error: command not found\n");
      exit(1);
    }
  } else if (pid > 0) {
    // printf("child pid: %d\n",pid);
    // int status;
    // printf("waiting on child\n");
  } else {
    perror("fork");
    popall(&start);
    exit(1);
  }
}
bool errorhandler(node_t *start, char *rawcmd) {

  pid_t pid = fork();
  if (pid == 0) {
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
        if (op->cmd[0] == '|') {
          // checking if pipe
          if (strcmp(Rcommand->type, "command") != 0) {
            // no command after pipe
            fprintf(stderr, "Error: missing command\n");
            popall(&start);
            exit(1);
            break;
          }
        } else {
          // file redirect
          // FILE *in_file = fopen(Rcommand->cmd, "r"); // read only
          // printf("%s", Rcommand->cmd);
          struct stat file;
          // file exists
          if (!(stat(Rcommand->cmd, &file))) {
            // not a real file
            // -https://users.cs.utah.edu/~germain/PPS/Topics/C_Language/file_IO.html
            FILE *in_file = fopen(Rcommand->cmd,
                                  "r"); // read to make sure file is accessible
            if (in_file == NULL) {
              fprintf(stderr, "Error: cannot open output file\n");
              popall(&start);
              exit(1);
              break;
            }
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
        } else {
          // file redirect
          fprintf(stderr, "Error: no output file\n");
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
    // printf("%s,%s,%s", rawcmd, write,pipe);
    if (write != NULL && pipe != NULL && (write < pipe)) {
      fprintf(stderr, "Error: mislocated output redirection\n");
      popall(&start);
      exit(1);
    }
    exit(0);
  }

  else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == 0) {
      return true;
    } else {
      return false;
    }
  } else {
    perror("fork");
    exit(1);
  }
}

void trimtrail(char *str) {
  int i = strlen(str) - 1;
  while (str[i] == ' ') {
    // printf("checking: %d\n", i);
    i--;
  }
  // printf("end: %d\nwhich is character %c\n", i, str[i]);
  str[i + 1] = '\0';
}

int main(void) {
  char newcmd[CMDLINE_MAX];
  char *rawcmd;
  // int original_stdin = dup(STDIN_FILENO);
  // int original_stdout = dup(STDOUT_FILENO);
  while (1) {
    bool file = false;
    //memset(newcmd, 0, CMDLINE_MAX);
    char *nl;
    // int retval;

    /* Print prompt */
    printf("sshell$ ");
    fflush(stdout);
    // fflush(stdin);

    /* Get command line */
    // printf("%d", fileno(stdin));
    fgets(newcmd, CMDLINE_MAX, stdin);
    //printf("%s", newcmd);

    /* Print command line if stdin is not provided by terminal */
    
    if (!isatty(STDIN_FILENO)) {
      printf("%s", newcmd);
      fflush(stdout);
    }

    /* Remove trailing newline from command line */
    nl = strchr(newcmd, '\n');
    if (nl)
      *nl = '\0';
    /*trailing spaces for no args*/

    /* Builtin command */
    if (!strcmp(newcmd, "exit")) {
      fprintf(stderr, "Bye...\n");
      fprintf(stderr, "+ completed '%s' [0]\n", newcmd);
      break;
    }

    /* Regular command */
    /* GIVEN:
    retval = system(cmd);
    fprintf(stdout, "Return status value for '%s': %d\n",
            cmd, retval);
    */
    // no args
    // printf("space: %s",strchr(cmd, " "));
    rawcmd = strdup(newcmd);
    trimtrail(newcmd);
    // parse w/ cmdlineparse
    // printf("hella args\n");
    node_t *head = malloc(sizeof(node_t));
    node_o *ocurr = malloc(sizeof(node_o));
    head->cmd = NULL;
    head->next = NULL;
    ocurr->next = NULL;
    // starts with | or >
    if (rawcmd[0] == '>' || rawcmd[0] == '|' || newcmd[0] == '\0') {
      fprintf(stderr, "Error: missing command\n");
      continue;
    } else {
      int nums_cmd = cmdlineparse(head, newcmd);
      // printf("nums cmd: %d\n", nums_cmd);
      node_t *start = head->next;
      if (errorhandler(start, rawcmd)) {

        // printf("init stuff");
        // int nargs = start->nargs;
        int pipearray[nums_cmd - 1][2];
        int p;
        for (p = 0; p < nums_cmd - 1; p++) {
          pipe(pipearray[p]);
        }
        // int cmd_ind = 0;
        char *last = head->cmd;
        while (start != NULL) {
          // printf("last command: %s\n", last);
          // printf("reading command: %s\n", start->cmd);
          if (strcmp(start->cmd, "|") != 0 && strcmp(start->cmd, ">") != 0 &&
              strcmp(start->cmd, ">>") != 0) {
            singlecommand(last, start, ocurr, pipearray, nums_cmd);
            // cmd_ind++;
          }
          if (strcmp(start->cmd, ">") == 0 || strcmp(start->cmd, ">>") == 0) {
            // printf("FILE!");
            file = true;
          }
          last = strdup(start->cmd);
          start = start->next;
        }
        closepipearray(pipearray, nums_cmd);
        // printf("closing pipearray\n");
        int c;
        for (c = 0; c < nums_cmd; c++) {
          int status;
          waitpid(WAIT_ANY, &status, 0);
          // printf("child finished: %d\n", status);
          // printf("child: %s completed\n",  cmd);
          //  fprintf(stderr, "+ completed '%s' [%d]\n", rawcmd,
          //  WEXITSTATUS(status));
          //  free(arguments);
          // pop(&start);
          if (status != 6400) {
            pushoutput(ocurr, WEXITSTATUS(status));
          }
          // printf("exited child %s with: %d\n",start->cmd,
          // WEXITSTATUS(status));
        }
        popall(&head);
        int value = 0;
        ocurr = ocurr->next;
        fprintf(stderr, "+ completed '%s' ", rawcmd);
        if (file) {
          // printf("one less\n");
          nums_cmd--;
        }
        int i;
        for (i = nums_cmd; i > 0; i--) {
          value = popoutput(&ocurr);
          if (value != -999) {
            fprintf(stderr, "[%d]", value);
          }
        }

        fprintf(stderr, "\n");
        popoutput(&ocurr);
        /*
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);

        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
        */
        /*while (peek(&start) != NULL){
          pop(&start);
        }*/
        popoutput(&ocurr);
        pop(&head);
        // free(&newcmd);

      } else {
        continue;
      }
    }
  }

  return EXIT_SUCCESS;
}
