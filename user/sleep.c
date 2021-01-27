#include <kernel/types.h>
#include <user/user.h>


int main(int argc, char* argv[]) {
	
	if(argc != 2) {
		fprintf(2,"Usage: sleep number\n");
		exit(1);
	}
	int sleep_tick = atoi(argv[1]);
	sleep(sleep_tick);
	exit(0);
}
