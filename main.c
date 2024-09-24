#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 64
#define MAX_COMMANDS 2000

char** history[MAX_COMMANDS];
int history_ptr = 0;

char* read_user_input(){
    char* cmnd;
    size_t len = 0;
    size_t read;
    read = getline(&cmnd, &len, stdin);
    
    if (read == -1) {
        printf("getline failed\n");
        free(cmnd);
        return "echo Some Error Occurred";
    }

    return cmnd;
}

// work on from here

void remove_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t')) {
        len--;
    }
    while(&str==' ')str++;
    str[len] = '\0';
}

void remove_leading_spaces(char *str) {
    char *start = str; // Pointer to the beginning of the string

    // Move the pointer forward until a non-space character is found
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1); // Include the null terminator
    }
}

char** split_command_space(char* command){
    // Determine command type length
    int len_cmnd_type = 0;
    for (int i = 0; (command[i] != ' ' && command[i] != '\0'); i++) {
        len_cmnd_type++;
    }

    int spaces = 0;
    for (int i = 0; command[i] != '\0'; i++) { 
        if (command[i] == ' ') spaces++;
        if (i>0 && command[i-1]==' ' && command[i]==' ') spaces--;
        else if(i==0 && command[i]==' ')spaces--;
    }

    char *command_type = malloc(len_cmnd_type + 1);
    if (command_type == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }

    // Copy command type
    for (int i = 0; i < len_cmnd_type; i++) {
        command_type[i] = command[i];
    }
    command_type[len_cmnd_type] = '\0';

    // Extract arguments
    int num_args = spaces + 1;
    char** args = malloc((num_args+1)*sizeof(char*));
    if (args == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }
    
    int ptr = 0;
    for (int i = 0; i < num_args; i++) {
        int ptr_ptr = ptr;
        int len_word = 0;

        // Count the length of the current word
        while (command[ptr_ptr] != '\0') {
            if (command[ptr_ptr] == ' ' && len_word!=0) {
                break;
            }else if(command[ptr_ptr]==' ' && len_word==0){
                ptr_ptr++;
                continue;
            }
            len_word++;
            ptr_ptr++;
        }

        char* word = malloc(len_word + 1); // Allocate memory for the word + null terminator
        if (word == NULL) {
            printf("Malloc failed\n");
            return 0;
        }

        // Fill the word array
        for (int j = 0; j < len_word; j++) {
            while(command[ptr]==' '){
                ptr++;
            }
            word[j] = command[ptr++];
        }
        word[len_word] = '\0'; // Null-terminate the string
        args[i] = word; // Store the word in args

        // Skip spaces for the next word
        while (command[ptr] == ' ') ptr++;
        if (command[ptr] == '\n') ptr++; // Skip newline if present
    }
    args[num_args] = NULL; // Null-terminate the args array

    return args;
}

char** split_command(char *command) {
    // split by |

    int pipes = 0;

    for(int i = 0;command[i]!='\0';i++){
        if(command[i]=='|') pipes++;
        if (i>0 && command[i-1]=='|' && command[i]=='|') pipes--;
        else if(i==0 && command[i]=='|')pipes--;
    }

    char** args=malloc((pipes+2)*(sizeof(char*)));
    if(args==NULL){
        printf("Malloc Failed\n");
        return 0;
    }
    int num_args = pipes+1;
    
    int ptr = 0;
    for (int i = 0; i < num_args; i++) {
        int ptr_ptr = ptr;
        int len_cmnd = 0;

        // Count the length of the current word
        while (command[ptr_ptr] != '\0') {
            if (command[ptr_ptr] == '|') {
                break;
            }
            len_cmnd++;
            ptr_ptr++;
        }

        char* word = malloc(len_cmnd + 1); // Allocate memory for the word + null terminator
        if (word == NULL) {
            printf("Malloc failed\n");
            return 0;
        }

        // Fill the word array
        for (int j = 0; j < len_cmnd; j++) {
            word[j] = command[ptr++];
        }
        word[len_cmnd] = '\0'; // Null-terminate the string
        args[i] = word; // Store the word in args

        // Skip spaces for the next word
        while (command[ptr] == '|') ptr++;
        if (command[ptr] == '\n') ptr++; // Skip newline if present
    }

    args[pipes+1] = NULL;

    return args;


}

int piped_process(char* command){
    // Assume Input Is Sanitized
    
    int status;
    char** args = split_command(command);

    // for(int i = 0;args[i]!=NULL;i++){
    //     printf("%s\n",args[i]);
    // }
    
    int num_commands = 0;
    
    // Count the number of commands
    while (args[num_commands] != NULL) {
        num_commands++;
    }

    // printf("%d",num_commands);

    int pipe_fd[2];
    int pid;
    int fds = 0; // Variable to keep track of the previous command's output

    for (int i = 0; i < num_commands; i++) {
        char* comand = strdup(args[i]);
        remove_leading_spaces(comand);
        remove_trailing_spaces(comand);
        char** exec = split_command_space(comand);

        if (i < num_commands - 1) {
            // Create a pipe for all but the last command
            if (pipe(pipe_fd) == -1) {
                perror("Pipe Error");
                free(comand); // Free dynamically allocated memory
                return 0;
            }
        }

        pid = fork();
        if(pid==-1){
            printf("Fork Failure\n");
            free(comand);
            return 0;
        }else if(pid==0){
            // Child
            if (fds != 0) {
                dup2(fds, STDIN_FILENO);  // Get input from the previous command's output
                close(fds); // Close the old input
            }

            if (i < num_commands - 1) {
                dup2(pipe_fd[1], STDOUT_FILENO); // Output to the pipe
                close(pipe_fd[0]); // Close unused read end
            }

            if (execvp(exec[0], exec) == -1) {
                perror("Invalid Command");
                return 0;
            }
        }else{
            // Parent
            if (fds != 0) {
                close(fds); // Close previous input pipe if it exists
            }

            if (i < num_commands - 1) {
                close(pipe_fd[1]); // Close write end of the current pipe
            }

            fds = pipe_fd[0]; // Save read end for the next command
            waitpid(pid, &status, 0); // Wait for the child process
        }
        free(comand); // Free dynamically allocated memory for the command
        free(exec); // Free the exec array if dynamically allocated
    }

    return 0;

}

int create_process_and_run(char* command){
    remove_leading_spaces(command);
    remove_trailing_spaces(command);

    history[history_ptr] = strdup(command);
    history_ptr++;

    char* str = "history";
    if(strcmp(str,command)==0) {
        history_ptr--;
        int i = 0;
        while(i<history_ptr){
            printf("%s\n",history[i]);
            i++;
        }
        return 0;
    }

    int pid;
    int status;

    bool pipe = false;

    for(int i = 0;command[i]!='\0';i++){
        if(command[i]=='|') pipe = true;
    }

    if(pipe){
        status = piped_process(command);
        // printf("%d",status);
        return status;
    }

    // Determine command type length
    int len_cmnd_type = 0;
    for (int i = 0; (command[i] != ' ' && command[i] != '\0'); i++) {
        len_cmnd_type++;
    }

    int spaces = 0;
    for (int i = 0; command[i] != '\0'; i++) { 
        if (command[i] == ' ') spaces++;
        if (i>0 && command[i-1]==' ' && command[i]==' ') spaces--;
        else if(i==0 && command[i]==' ')spaces--;
    }

    char *command_type = malloc(len_cmnd_type + 1);
    if (command_type == NULL) {
        printf("malloc failed\n");
        return 0;
    }

    // Copy command type
    for (int i = 0; i < len_cmnd_type; i++) {
        command_type[i] = command[i];
    }
    command_type[len_cmnd_type] = '\0';

    // Extract arguments
    int num_args = spaces + 1;
    char** args = malloc((num_args+1)*sizeof(char*));
    if (args == NULL) {
        printf("Malloc failed\n");
        return 0;
    }
    
    int ptr = 0;
    for (int i = 0; i < num_args; i++) {
        int ptr_ptr = ptr;
        int len_word = 0;

        // Count the length of the current word
        while (command[ptr_ptr] != '\0') {
            if (command[ptr_ptr] == ' ' && len_word!=0) {
                break;
            }else if(command[ptr_ptr]==' ' && len_word==0){
                ptr_ptr++;
                continue;
            }
            len_word++;
            ptr_ptr++;
        }

        char* word = malloc(len_word + 1); // Allocate memory for the word + null terminator
        if (word == NULL) {
            printf("Malloc failed\n");
            return 0;
        }

        // Fill the word array
        for (int j = 0; j < len_word; j++) {
            while(command[ptr]==' '){
                ptr++;
            }
            word[j] = command[ptr++];
        }
        word[len_word] = '\0'; // Null-terminate the string
        args[i] = word; // Store the word in args

        // Skip spaces for the next word
        while (command[ptr] == ' ') ptr++;
        if (command[ptr] == '\n') ptr++; // Skip newline if present
    }
    args[num_args] = NULL; // Null-terminate the args array

    // Fork and execute the command
    pid = fork();
    if (pid == 0) {
        // Child Process
        if (execvp(command_type, args) == -1) {
            printf("Invalid Command\n");
            return 0;
        }
    } else if (pid < 0) {
        printf("Fork Failed\n");
        return 0;
    } else {
        waitpid(pid, &status, 0);
    }

    // Free allocated memory
    free(command_type);
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
    free(args);

    return status;
}

// code all good below this line

int launch(char* command){
    int status;
    status = create_process_and_run(command);
    // printf("STATUS %d\n",status);
    return status;
}

void shell_loop(){
    int status;
    do {
        printf("@User:~$: ");
        char* command = read_user_input();

        if(command[0]=='\n'){
            status = 0;
            continue; // user just pressed enter
        }
        
        // Note : command also has a new line character at the end
        // so we remove it
        command[strlen(command)-1] = '\0';
        
        // printf("%s",command);
        // exit(1);

        status = launch(command);
        free(command);
    }while(status==0);
}

void exit_shell(){
    printf("\nExiting!\n");
    exit(0);
}

int main(int argc,char** argv){
    signal(SIGINT, exit_shell);

    printf("Welcome To Our Shell!\n");
    shell_loop();
    
    return 0;
}
