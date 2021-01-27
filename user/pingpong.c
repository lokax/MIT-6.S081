#include <kernel/types.h>
#include <user/user.h>

int main(int argc, char* argv[]) {
	int pid;
	int p[2];
	int my_pid;
	char buf[5];
	pipe(p);
	pid = fork();
	if(pid > 0) {
		my_pid = getpid();
		write(p[1],"ping",5);
		wait(0);
		read(p[0],buf,5);
		fprintf(1,"%d: received %s\n",my_pid,buf);
	} else if(pid == 0) {
		read(p[0],buf,5);
		my_pid = getpid();
		fprintf(1,"%d: received %s\n",my_pid,buf);
		write(p[1],"pong",5);
	} else {
		//fprintf(1,"%d",pid);
		fprintf(2,"fork failed\n");
		exit(1);
	}
	exit(0);
	
}
