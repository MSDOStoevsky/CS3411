#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFFERSIZE 256
#define READ  0
#define WRITE 1

#define FIRST 1
#define MIDDLE 0
#define LAST -1

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 07/07/2017               */
/**********************************/

/* change directory */
int cd(char *pth){

    char path[BUFFERSIZE];
    char cwd[BUFFERSIZE];
    strcpy(path,pth);

    if(pth[0] != '/')
    {
        getcwd(cwd,BUFFERSIZE);
        strcat(cwd,"/");
        strcat(cwd,path);
        chdir(cwd);
    }else{
        chdir(pth);
    }
    return 0;
}

/* splits tokens on string delim */
char *strtokm(char *str, const char *delim)
{
    static char *tok;
    static char *next;
    char *m;

    if (delim == NULL) return NULL;

    tok = (str) ? str : next;
    if (tok == NULL) return NULL;

    m = strstr(tok, delim);

    if (m) {
        next = m + strlen(delim);
        *m = '\0';
    } else {
        next = NULL;
    }

    return tok;
}

/* turn string into array of strings based on delimiter */
char **split(char *string, const char *delim)
{
    int position = 0, bufsize = BUFFERSIZE;
    char **argv = malloc(bufsize);
    char *arg;

    if(!argv) 
    {
        write(STDERR_FILENO, "Allocation error.\n", 18);
        exit(EXIT_FAILURE);
    }

    arg = strtok(string, delim);
    while (arg != NULL) {
        argv[position++] = arg;

        /* increase array size */
        if (position >= bufsize) {
            bufsize += BUFFERSIZE;
            argv = realloc(argv, bufsize);
            if (!argv) {
                write(STDERR_FILENO, "Allocation error.\n", 18);
                exit(EXIT_FAILURE);
            }
        }
        arg = strtok(NULL, delim);
    }
    argv[position] = NULL;
    return argv;
}

/* execute individual command */
/* 
    command comes in as char array 
    e.g. "./test Dylan"
*/
static int exec(char *in_cmd, int in, int out, int run)
{
    pid_t pid, wpid;
    int fd [2], status;
    const char delim[5] = " \t\r\n\a";
    char **cmd = split(in_cmd, delim);

    /* ignore if no input */
    if (cmd[0] == NULL) return -1;

    if(strcmp(cmd[0],"cd") == 0 && cmd[1] != NULL) cd(cmd[1]);
    else{
        pipe( fd );
        pid = fork();
        if (pid == 0) {
            
            if(run == 1 && in == 0)
                dup2( fd[WRITE], STDOUT_FILENO );
            else if(run == 0 && in != 0)
            {
			    dup2(in, STDIN_FILENO);
                dup2(fd[WRITE], STDOUT_FILENO);        
            }
            else
            {
                dup2( in, STDIN_FILENO );
                if(out > 0)
                {
                    dup2(out, STDOUT_FILENO);
                    close(out);
                }
            }

            if (execvp( cmd[0], cmd) == -1)
            {
                free(cmd);
                exit(EXIT_FAILURE);
            }

        } else if (pid < 0) {
            write(STDERR_FILENO, "fork error\n", 11);
            return -1;
        } else {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        if (in != 0) close(in);

        close(fd[WRITE]);

        if (run == -1) close(fd[READ]);

        return fd[READ];
    }
    free(cmd);
    return 0;
}

/* wait for all exec children */
static void p_wait(int n)
{
	int i;
	for (i = 0; i < n; ++i) 
		wait(NULL); 
}

/* 
    command comes in as array of char array 
    e.g. {"./test Dylan", "grep Hello"}
*/
void fork_exec(char ** cmd, int in, int out)
{
    int index, run;
    run = FIRST;
    for (index = 0; cmd[index + 1] != NULL; index++)
    {
        in = exec(cmd[index], in, 1, run);
        run = MIDDLE;
    }
    if( (in = exec(cmd[index], in, out, LAST)) < 0)
    {
        write(STDERR_FILENO, "exec error\n", 11);
        exit(1);
    }
    p_wait(index);
}

int main()
{
    int in, out;

    /* allocate buffer for print msgs */
    char *prompt_buff = ((char*) malloc(BUFFERSIZE) + 2);

    /* buffer for program names and their arguments */
    char *in_buff = (char*) malloc(BUFFERSIZE);
    char **outcmd;
    char **cmd;
    char **ifile;
    char **ofile;
    const char *delim = "|";
    const char *odelim = ">";
    const char *idelim = "<";

    while(1)
    {
        outcmd = NULL;
        cmd = NULL;
        ofile = NULL;
        ifile = NULL;
        bzero(in_buff, BUFFERSIZE);
        bzero(prompt_buff,(BUFFERSIZE + 2));
        /* set and display console prompt */
        sprintf(prompt_buff, "%s%s", getcwd(prompt_buff, BUFFERSIZE), "> ");
        if( write(STDOUT_FILENO, prompt_buff, (BUFFERSIZE + 2)) < 0) 
            write(STDERR_FILENO, "console prompt error\n", 22);

        /* read line into in_buff */
        if(read(STDIN_FILENO, in_buff, 256) < 0)
        {
            write(STDERR_FILENO, "read error\n", 11);
            exit(1);
        }

        /* break along > delims */
        outcmd = split(in_buff, odelim);
        if(outcmd[1] != NULL)
            ofile = split(outcmd[1], " \t\r\n\a");

        /* set stdout */
        out = -1;
        if(ofile != NULL)
        {
            if( (out = open( ofile[0], O_WRONLY|O_CREAT, 0666)) < 0)
            {
                write(STDERR_FILENO, "open error\n", 11);
                exit(1);
            }
        }
        /* break along < delims */
        cmd = split(outcmd[0], idelim);
        if(cmd[1] != NULL)
            ifile = split(cmd[1], " \t\r\n\a");

        /* set stdin DOESNT WORK */
        in = 0;
        if(ifile != NULL)
        {   
            if( (in = open(ifile[1] ,O_RDONLY)) < 0) 
            {
                write(STDERR_FILENO, "file not found\n", 15);
                exit(1);
            }
            dup2(in,STDIN_FILENO);
            close(in);
        } 
        
        cmd = split(cmd[0], delim);
        fork_exec(cmd, in, out);
    }
    free(ifile);
    free(ofile);
    free(prompt_buff);
    free(in_buff);
    free(outcmd);
    free(cmd);
    return 0;
}