#include <kernel/types.h>
#include <user/user.h>

int main() {
	int pid;
	int p[2];
	int buf[35];
	int cnt = 0;
	for(int i = 2; i <= 35; ++i) {
		buf[cnt++] = i;
	}

	while(cnt > 0) {
		pipe(p);
		pid = fork();
		if(pid < 0) {
			fprintf(2,"fork failed\n");
			exit(1);
		} else if(pid > 0) {
			close(p[0]);
			for(int i = 0; i < cnt; ++i) {
				write(p[1],&buf[i],sizeof(int));	
			}
			//write(p[1],);
			close(p[1]);
			wait((int*)0);
			exit(0);
		} else {
			close(p[1]);
			cnt = -1;
		//	int* primes = (int*)0;
		//	int* elem   = (int*)0;
			int primes  = 0;
			int elem    = 0;
			//read(p[0],primes,sizeof(int));
			//read(p[0],elem,sizeof(int));
			//fprintf(1,"begin""\n");
			while(read(p[0],&elem,sizeof(int)) != 0) {
				if(cnt == -1) {
					primes = elem;
					cnt    = cnt + 1;
				} else {
				if(((elem) % (primes)) != 0) {
					buf[cnt++] = elem;
				//	write(p[1],buf,sizeof(int));
				}		
				}
			}
			//close(p[0]);
			//close(p[1]);
			//if(cnt != -1) 
			fprintf(1,"prime %d\n",primes);
			close(p[0]);
		}
	}
	exit(0);
}
