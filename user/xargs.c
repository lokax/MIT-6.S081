#include <kernel/types.h>
#include <user/user.h>

int main(int argc, char* argv[]) {
    char* param[32];
    char* command = argv[1];
    char buf[32];
    int i = 0;
    for(int j = 1; j < argc; ++j) {
        param[i++] = argv[j];
    }
    
    #if 0
    for(int j = 0; j < i; ++j)
        printf("%s\n",param[j]);
    printf("%s\n",command);
    //exit(0);
    #endif

    char c;
    int  j = 0;
    while(read(0,&c,sizeof(char)) != 0) {
        buf[j++] = c;
        int pid = fork();
        if(pid == 0) {
            while(read(0,&c,sizeof(char)) != 0) {
                if(c == '\n') {
                    buf[j] = 0;
                    break;
                } //else if(c == 'q') {
                   // break;    
                else {
                    buf[j++] = c;
                }
                //buf[j++] = 
            }
            param[i++] = buf;
            param[i] = 0;
            exec(command,param); 
        } else {
            j = 0;
            wait((int*)0);
        }
    }
    exit(0);

}