#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>


int
main(int argc, char **argv)
{
    pid_t pid;
    int wait_val;

    pid=fork();

    if(pid == 0)
    {
        ptrace (PTRACE_TRACEME, 0, 0, 0);

        execlp("./test", "./test", NULL);
    }

    /* wait for test to start */
    wait(&wait_val);

    kill(pid, SIGSTOP);

    printf("pid=%d\n", pid);
    fflush(stdout);
    sleep(2);
    

    ptrace (PTRACE_DETACH, pid, 0, 0);

    return 1;
}
