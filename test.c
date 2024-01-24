#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

#define CMDLINE_MAX 512

typedef struct node {
    char* cmd;
    struct anode *arg;
    struct node * next;
} node_t;

typedef struct anode {
    char* arg;
    struct anode * next;
} node_a;

void push(node_t * head, char* cmd, node_a* args) {
    node_t * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = (node_t *) malloc(sizeof(node_t));
    current->next->cmd = cmd;
    current->next->arg = args;
    current->next->next = NULL;
}

char* pop(node_t ** head) {
    char* retval = NULL;
    node_t * next_node = NULL;

    if (*head == NULL) {
        return NULL;
    }

    next_node = (*head)->next;
    retval = (*head)->cmd;
    free(*head);
    *head = next_node;

    return retval;
}

char* peek(node_t ** head){
    char* retval = NULL;
    node_t * next_node = NULL;
    if (*head == NULL) {
        return NULL;
    }
    next_node = (*head)->next;
    retval = next_node->cmd;
    return retval;
}

void pushargs(node_a * head, char* argin) {
    node_a * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = (node_a *) malloc(sizeof(node_a));
    current->next->arg = argin;
    current->next->next = NULL;
}

char* popargs(node_a ** head) {
    char* retval = NULL;
    node_a * next_node = NULL;

    if (*head == NULL) {
        return NULL;
    }

    next_node = (*head)->next;
    retval = (*head)->arg;
    free(*head);
    *head = next_node;

    return retval;
}

void cmdlineparse(node_t* node, char* commandline){
        const char delimiters[] = " ";
    char *token;

    /* get the first token */
    token = strtok(commandline, delimiters);
    node_t* tempheadc = node;
    /* walk through other tokens */
    while (token != NULL) {
        //parse ommand
        char* cmd = token;
        token = strtok(NULL, delimiters);
        node_a* tempheada = malloc(sizeof(node_a));
        tempheada->arg = "TEMP";
        while (token != NULL && token[0] != '>' && token[0] != '|' && strcmp(token,"<<")){ 
            //parse args until pipe, redirection or end
            char* arg = token;
            pushargs(tempheada,arg);
            token = strtok(NULL, delimiters);
        }
        //push to overall linkedlist
        push(tempheadc,cmd, tempheada);
        //special commands
        if (token != NULL){
                node_a* tempheadempty = malloc(sizeof(node_a));
                char* special = token;
                tempheadempty->arg = NULL;
                push(tempheadc, cmd, tempheadempty);
        /*
        printf("\n");
        printf("special: %s", token);
        printf("\n");
        */
                token = strtok(NULL, delimiters);
        }
    }

}

void manreet(node_t* head){
    int i = 0;
    char output[CMDLINE_MAX]; 
    

    char *operation = linky[1]; // char *operation = linky[1].cmd
    char *previous_operation;

    // linked list : struct here for a simple linked list. 


    if (operation != "NULL"){
        while(1){
            int fd[2];
            pipe(fd);
            if (fork () != 0){ // PARENT

                // printf("Parent executes i = %d with command %s and argument: %s\n\n", i, linky[i], linkyarguments[i]);

                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[1]);

                // all done in one function
                char *command = linky[i]; //  char *command = linky[i].cmd
                char *argument = linkyarguments[i]; 
                char *args[] = {command, argument, NULL};

                execv(command, args); // done in function: (* )
                printf("was not executed\n");

            } else { // CHILD . We're only here if there's a next command

                // printf("Child executes code\n");

                // char output[CMDLINE_MAX];
                previous_operation = operation; // linky[i].operation == linky[i+2].past_operation
                i = i + 2;

                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);

                fgets(output, CMDLINE_MAX, stdin);
                printf("output: %s \n", output);


                // APPEND::: 

                // append output to linky[i].args
                // int new_length = strlen(linkyarguments[i])  + strlen(output) + 1;
                // char *new_string = malloc(new_length * sizeof(char));
                // strcpy(new_string, linkyarguments[i]);
                // strcat(new_string, output);
                // linkyarguments[i] = new_string;
                // printf("Appended output to linkyarguments[%d].args = %s\n", i, linkyarguments[i]);
                // free(new_string);

                operation = linky[i+1]; // operation = linky[i+1].cmd (peek)

                if (operation == "NULL" || previous_operation != "|"){
                    break;
                } else {
                    continue;
                }
            }
        }

        if (previous_operation == "|"){
            printf("Executing command: %d : %s : with argument: %s\n ", i, linky[i], linkyarguments[i]);
            char *command = linky[i];
            char *argument = linkyarguments[i];
            char *args[] = {command, argument, NULL};
            execv(command, args);
            // print output

        } else if (previous_operation == ">"){
            FILE *file = fopen("file.txt", "w");

            if (file == NULL){
                perror("File Does not Exist");
            }

            fprintf(file, "%s", output);
            fclose(file);


        } else if (previous_operation == ">>"){
            FILE *file = fopen("file.txt", "a");

            if (file == NULL){
                perror("File Does not Exist");
            }

            fprintf(file, "%s", output);
            fclose(file);
        } 

        

    } else {
        printf("WE ARE IN THE RIGHT PLACE!!! \n");

            // args (linky[i].cmd, linky[i].arguments.head, linky[i].output, linky[i].arguments.head)
            char *command = linky[i];
            char *argument = linkyarguments[i];
            char *args[] = {command, argument, NULL};
            execv(command, args);
    }

    printf("DONE!\n");
}

int main(void) {

    // GIVEN A FULLY FORMED LINKY
    char commandline[CMDLINE_MAX];

    printf("Enter your command:\n");
    if (fgets(commandline, CMDLINE_MAX, stdin) == NULL) {
        fprintf(stderr, "Error reading command.\n");
        return EXIT_FAILURE;
    }

    // Remove the newline character at the end of input
    commandline[strcspn(commandline, "\n")] = 0;

    // Initialize the command list head
    node_t* head = malloc(sizeof(node_t));
    if (!head) {
        fprintf(stderr, "Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    head->cmd = NULL;
    head->arg = NULL;
    head->next = NULL;

    cmdlineparse(head, commandline);


    //char *linky[] = {"/bin/echo", "NULL"}; // linky[] - not neccessary
    //char *linkyarguments[] = {"G Karda", "NULL"}; //linky.arguments[] - not necessary
    return 0;
}