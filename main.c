#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

#define MAX_ARGS 64

char* read_user_input(){
    char* cmnd;
    size_t len;
    getline(&cmnd,&len,stdin);
    return cmnd;
}

// int create_process_and_run(char* command){
//     int pid;
//     int status;

//     int len_cmnd_type = 0;

//     for(int i = 0;(command[i]!=' ' && command[i]!='\n' && command[i]!='|');i++){
//         len_cmnd_type++;
//     }

//     int spaces = 0;
//     for(int i = 0;i!='\0';i++){
//         if(command[i]==' ')spaces++;
//     }

//     char *command_type = malloc(len_cmnd_type+1);

//     for(int i = 0;i<len_cmnd_type;i++){
//         command_type[i]=command[i];
//     }

//     command_type[len_cmnd_type]='\0';

//     // printf("%s\n",command_type);
//     // Now we have extracted Our Command!
//     // Now To Extract The Arguments
//     // For Now we assume no "|" in our command

//     int num_args = spaces+1;
//     // printf("%d\n",num_args);
//     // char* args[num_args];
//     // int ptr = 0;
    
//     // for(int i = 0;i<num_args;i++){
//     //     int ptr_ptr = ptr;
//     //     int len_word = 0;
//     //     while(command[ptr_ptr]!='\0'){
//     //         printf("%c\n",command[ptr_ptr]);
//     //         if(command[ptr_ptr]==' ') break;
//     //         if(command[ptr_ptr]=='\n') continue;
//     //         else len_word++;
//     //         ptr_ptr++;
//     //     }
//     //     char* word = malloc(len_word+1);
//     //     // printf("WORD LEN %d\n",len_word);
//     //     int i = 0;
//     //     while(command[ptr]!='\0'){
//     //         if(command[ptr]==' ') break;
//     //         if(command[ptr]=='\n') continue;
//     //         else {
//     //             word[i] = command[ptr];
//     //             i++;
//     //         }
//     //         ptr++;
//     //     }
//     //     word[len_word]='\0';
//     //     args[i]=word;
//     // }

//     char* args[num_args];
//     int ptr = 0;

//     for (int i = 0; i < num_args; i++) {
//         int ptr_ptr = ptr;
//         int len_word = 0;

//         // Count the length of the current word
//         while (command[ptr_ptr] != '\0') {
//             if (command[ptr_ptr] == ' ' || command[ptr_ptr] == '\n') break;
//             len_word++;
//             ptr_ptr++;
//         }

//         char* word = malloc(len_word + 1); // Allocate memory for the word + null terminator
//         if (word == NULL) {
//             perror("malloc failed");
//             exit(EXIT_FAILURE);
//         }

//         // Fill the word array
//         for (int j = 0; j < len_word; j++) {
//             word[j] = command[ptr++];
//         }
//         word[len_word] = '\0'; // Null-terminate the string
//         args[i] = word;

//         // Skip the spaces for the next word
//         while (command[ptr] == ' ') ptr++;
//         if (command[ptr] == '\n') ptr++; // Skip newline if present
//     }

//     // for(int i = 0;i<num_args;i++){
//     //     printf("ARGS:%s\n",args[i]);
//     // }

//     pid = fork();

//     if(pid==0){
//         // Child Process
//         if(execvp(command_type,args)==-1){
//             printf("execvp Failed\n");
//         }
//         exit(1);
//     }else if(pid<1){
//         printf("Fork Failed\n");
//     }else{
//         wait(pid,&status,0);
//     }

//     return status;
// }

char** split_command(char *command) {
    // Step 1: Split by '|'
    char **commands = malloc(64 * sizeof(char*)); // Array to hold commands
    char *token;
    int cmd_count = 0;

    // Tokenize using '|'
    token = strtok(command, "|");
    while (token != NULL) {
        commands[cmd_count++] = token; // Store each command
        token = strtok(NULL, "|");
    }
    commands[cmd_count] = NULL; // Null-terminate the array of commands

    // Step 2: Split each command by spaces and store the arguments
    char **args = malloc(128 * sizeof(char*)); // Array to hold all arguments, assume 128 max
    int ptr = 0;
    
    for (int i = 0; i < cmd_count; i++) {
        // Tokenize each command by spaces
        token = strtok(commands[i], " ");
        while (token != NULL) {
            args[ptr++] = token;  // Store the argument
            token = strtok(NULL, " ");
        }
        // if (i < cmd_count - 1) {
        //     args[ptr++] = "|";  // Add a pipe symbol between commands
        // }
    }

    args[ptr] = NULL; // Null-terminate the args array
    free(commands); // Free the commands array as it's no longer needed

    return args; // Return the array of arguments
}

int piped_process(char* command){
    // Assume Input Is Sanitized
    
    int status;

    int pipes = 0;

    for(int i = 0;command[i]!='\0';i++){
        if(command[i]=='|') pipes++;
    }

    char** args = split_command(command);

    for(int i = 0;args[i]!=NULL;i++){
        printf("%s\n",args[i]);
    }
    
    int fd[2]; // File descriptors for the pipe
    int pid;
    int fds = 0; // To hold the read end of the previous pipe

    while (*args != NULL) {
        // Create a pipe
        pipe(fd);
        if ((pid = fork()) == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            dup2(fds, 0);  // Use the input from the previous command
            if (*(args + 1) != NULL) {
                dup2(fd[1], 1);  // Output to the next command
            }
            close(fd[0]); // Close unused read end
            execvp(*args, args);  // Execute the command
            perror("exec failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            wait(NULL);  // Wait for the child process to finish
            close(fd[1]); // Close unused write end
            fds = fd[0];  // Save the read end of the current pipe for the next command
            args += 2;    // Move to the next command (assuming the arguments are separated by NULL)
        }
    }

}

int create_process_and_run(char* command){
    int pid;
    int status;

    bool pipe = false;

    for(int i = 0;command[i]!='\0';i++){
        if(command[i]=='|') pipe = true;
    }

    if(pipe){
        status = piped_process(command);
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
        exit(1);
    }

    // Copy command type
    for (int i = 0; i < len_cmnd_type; i++) {
        command_type[i] = command[i];
    }
    command_type[len_cmnd_type] = '\0'; // Null-terminate the string

    // Extract arguments
    int num_args = spaces + 1;
    char** args = malloc((num_args + 1) * sizeof(char*)); // Allocate for arguments
    if (args == NULL) {
        printf("malloc failed\n");
        exit(1);
    }
    
    int ptr = 0;
    for (int i = 0; i < num_args; i++) {
        int ptr_ptr = ptr;
        int len_word = 0;

        // Count the length of the current word
        while (command[ptr_ptr] != '\0') {
            if (command[ptr_ptr] == ' ') {
                break;
            }
            len_word++;
            ptr_ptr++;
        }

        char* word = malloc(len_word + 1); // Allocate memory for the word + null terminator
        if (word == NULL) {
            printf("malloc failed\n");
            exit(1);
        }

        // Fill the word array
        for (int j = 0; j < len_word; j++) {
            word[j] = command[ptr++];
        }
        word[len_word] = '\0'; // Null-terminate the string
        args[i] = word; // Store the word in args

        // Skip spaces for the next word
        while (command[ptr] == ' ') ptr++;
        if (command[ptr] == '\n') ptr++; // Skip newline if present
    }
    args[num_args] = NULL; // Null-terminate the args array for execvp

    // Fork and execute the command
    pid = fork();
    if (pid == 0) {
        // Child Process
        if (execvp(command_type, args) == -1) {
            printf("Invalid Command\n");
            return 1;
        }
    } else if (pid < 0) {
        perror("Fork Failed");
    } else {
        // Parent process waits for the child to finish
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
    }while(status==0);
}

void exit_shell(){
    printf("\nExiting!\n");
    exit(0);
}

int main(int argc,char** argv){
    signal(SIGINT, exit_shell);

    printf("Welcome To My Shell!\n");
    shell_loop();
    
    return 0;
}