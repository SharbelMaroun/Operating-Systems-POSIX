#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 100

int main(void)
{
    close(2); /* closing stderr */
    dup(1); /* directing errors to stdout */
    char command[BUFFER_SIZE]; /* to save the recieved command */
    char **history = (char**)malloc(sizeof(char*));
    char **temp; /* to backup the history while reallocating */
    int i = 0; /* to use in for loops */
    int his_count = 0; /* history counter */
    int his_size = 1; /* history size */
    if(history == NULL)
        {
            perror("error");
            return 0;
        }
        
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE); /* filling all the cells with '\0' */
        fgets(command, BUFFER_SIZE, stdin); /* getting the input and saving it to command */
        if(strncmp(command, "exit", 4) == 0) /* checking if the user insert exit */
        {
            break;
        }
        
        if (command[strlen(command) - 1] == '\n') /* removing the enter char if exists */
        {
            command[strlen(command) - 1] = '\0'; /* replacing '\n' with \0'*/
        }
        
        if(his_count == his_size)
        {
            his_size = his_size*2; /* making the history larger by 1 */
            temp = (char**)realloc(history, his_size * sizeof(char *));
            if(temp == NULL)
            {
                /* Free the allocated memory before exiting */
                for (i = 0; i < his_count; i++)
                {
                    free(history[i]);
                }
                free(history);
                perror("error");
                return 0;
            }
            history = temp; /* history will point again (the memory allocation succeeded) */
        }
        
        history[his_count] = (char*)malloc(strlen(command)+1); /* allocating memory for the recieved command to store in history */
        if(history[his_count] == NULL)
        {
            /* Free the allocated memory before exiting */
            for (i = 0; i < his_count; i++)
            {
                free(history[i]);
            }
            free(history);
            perror("error");
            return 0;
        }
        strcpy(history[his_count], command); /* copying the received command to the history */
        his_count ++;
        
        if(strncmp(command, "history", 7) == 0) /* checking if the command's prefix is history */
        {
            for(i = (his_count-1) ; i >= 0 ; i--)
            {
                fprintf(stdout, "%d %s\n",i+1 , history[i]);
            }
            continue; /* go to next  command */
        }
        
        int background = 0; /* background executing indicator */
        if ((strlen(command) >= 2) && command[strlen(command)-1] == '&' && command[strlen(command)-2] == ' ')
        {
            background = 1; /* executing in background */
            command[strlen(command) - 1] = '\0'; /* removing '&' from the command */
        }
        else
        {
            background = 0; /* without '&' the command will execute in foreground */
        }
        
        pid_t pid = fork(); /* new process */
        if (pid == 0) /* child process */ 
        {
            char *argv[BUFFER_SIZE]; /* array to send to execvp function as a parameter*/
            int argc = 0; /* agrv index */
            char *token = strtok(command, " "); /* take the token from command */
            
            while (token != NULL) /* tokenize till NULL */
            {
                argv[argc++] = token;
                token = strtok(NULL, " ");
            }
        argv[argc] = NULL; /* to make sure that the last cell in the array contains NULL */
        execvp(argv[0],argv); /* executing the recieved command */
        perror("error"); /* report an error */
        exit(EXIT_FAILURE); /* exit child process */
        }
        else if (pid > 0) /* father process */
        {
            if(background == 0)
            {
                waitpid(pid, NULL, 0); /* waiting for child process to finish */
            }
        }
        else /* pid = -1 (fork() has failed) */
        {
            perror("error");
        }
    }
    
    /* Free the allocated memory before exiting */
    for (i = 0; i < his_count; i++)
    {
        free(history[i]);
    }
    free(history);
    
    return 0;
}



