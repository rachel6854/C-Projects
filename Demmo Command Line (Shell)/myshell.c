#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

void sigChilHandler(int signum) 
{ 
    waitpid(0,NULL,0); 
} 
 
int prepare(void)
{
	struct sigaction s;
	s.sa_handler = SIG_IGN;
	sigaction(SIGINT, &s, NULL);

	return 0;
}

// Help function to remove ampersand
char **removing_ampersand(char **arglist,int count)
{
	char **rm_arglist=malloc(sizeof(char*)*(count-1));
	size_t i=0;
	
	//removing ampersand from arglist 
	while(strcmp(arglist[i],"&")&&arglist[i]!=NULL) 
	{
		rm_arglist[i]=arglist[i];
		i++;
	}
	rm_arglist[i]=NULL;
	
	return rm_arglist;	
}

// Help function to check if command contain pipe
int isPipe(char **arglist, int count)
{
	int i;
	
	//checking if arglist contain pipe
	for(i=0;i<count-1;++i) 
	    {
	    if(!strcmp(arglist[i],"|"))
	    	{
	    	arglist[i]=NULL;
	    	break;
	    	}
	    }
	return i;
}

// Help function to run process
void runProcess(char ** cmd)
{
    if (fork() == 0) 
    {
    	struct sigaction child;	
	child.sa_handler = SIG_DFL;
	sigaction(SIGINT, &child,NULL);
        // run child process
        if (execvp(cmd[0], cmd) == -1) 
        {
            printf("%s: command not found\n", cmd[0]);
            exit(0);
        }
    }
    else 
    {
        // waiting in the parent process
        waitpid(0,NULL,0);
    }

}

//Help function to run pipe process
void pipeProcesses(char ** cmd1, char ** cmd2){

    int fd[2];
    if (pipe(fd) < 0) 
    { 
        printf("\nERROR! Pipe could not be initialized\n"); 
        return; 
    }
    if (fork()==0) 
    {
        struct sigaction child;	
	child.sa_handler = SIG_DFL;
	sigaction(SIGINT, &child,NULL);
	
        // make STD_OUT same as fd[1]
        if(dup2(fd[1],1)==-1)
        	printf("\nERROR! Could not dup2\n");
        close(fd[0]);
        runProcess(cmd1);
        exit(0);
    } 
    else 
    {
        // make STD_IN same as fd[0]
        if(dup2(fd[0],0)==-1)
        	printf("\nERROR! Could not dup2\n");
        close(fd[1]);
        execvp(cmd2[0], cmd2);
    }
}

int process_arglist(int count, char** arglist)
{
	pid_t pid1,pid2;
	int isAmpersand=false;
	int i=isPipe(arglist,count);
	
	//checking if pipe command
	if(i<count-1) 
		{
		if((pid2=fork())!=0)
			waitpid(pid2,NULL,0);
		else
			pipeProcesses(arglist,arglist+i+1);
		}
			
	//if not pipe command
	else
	    	{
	    	if((pid1=fork())==0)
	    		{

	    		//checking if ampersand
	    		if(!strcmp(arglist[count-1],"&")) 
	    			{
	    			// removing ampersand
	    			arglist=removing_ampersand(arglist,count); 
	    			isAmpersand=true;
	    			}
	    		else
	    			{
	    			struct sigaction child;	
				child.sa_handler = SIG_DFL;
				sigaction(SIGINT, &child,NULL);
	    			}
	    		if (execvp(arglist[0],arglist)==-1)
	    			printf("%s: command not found\n",arglist[0]);
	    		}
	    	// waiting if no ampersand
	    	if(arglist[count-1]==NULL)
	    		{ 
			waitpid(pid1,NULL,0);
			}
		/*else
			{
			// zombie handler
		    	struct sigaction sc;	
			sc.sa_handler = &sigChilHandler;
			sigaction(SIGCHLD, &sc,NULL);
			}*/
		}
		
	// free dynamic allocation
	if(isAmpersand) 
		free(arglist);
	return 1;
}


int finalize(void){
    return 0;
}
