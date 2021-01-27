#include <kernel/types.h>
#include <kernel/stat.h>
#include <kernel/fs.h>
#include <user/user.h>

char* fmtname(char* path) {
//	static char buf[DIRSIZ+1];
	char* p;

	for(p = path + strlen(path); p >= path && *p != '/';p--)
			;
	p++;
//	if(strlen(p) >= DIRSIZ)
		//	return p;
//	memmove(buf,p,strlen(p));
//	memset(buf+strlen(p),' ',DIRSIZ-strlen(p));
//	buf[strlen(p)] = 0;
	return p;
}

void find(char* path, char* name) {
//	if(argc < 3) {
//		fprintf(2,"too few argument\n");
//		exit(1);
//	}
	char buf[512];
	char* p;
	int fd;
	struct dirent de;
	struct stat st;
//	char* path = argv[1];
//	char* serach_name = argv[2];
//	
	fd = open(path,0);
	if(fd < 0) {
		fprintf(2,"find: cannot open %s\n",path);
	//	fprintf(1,"test: %s\n",path);
		exit(1);
	}
//	fprintf(1,"test\n");
	if(fstat(fd,&st) < 0) {
		fprintf(2,"find: cannot stat %s\n",path);
		close(fd);
		exit(1);
	}
	char* file_name = fmtname(path);
//	printf("%s\n",file_name);
//	printf("%s\n",name);
//	printf("%d\n",strlen(file_name));
//	printf("%d\n",strlen(name));
	switch(st.type) {
	case T_FILE:
	//	char* file_name = fmtname(path);
//		printf("test100\n");
//		int a  = strcmp(file_name,name);
//		printf("%d\n",a);

		if(strcmp(file_name,name) == 0) 
		
			//printf("%s\n",name);
			fprintf(1,"%s\n",path);
		break;
	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
			fprintf(2,"findL path too long\n");
			break;
		}
		strcpy(buf,path);
		p = buf + strlen(buf);
		*p++ = '/';
		//p++;
		while(read(fd,&de,sizeof(de)) == sizeof(de)) {
			if(de.inum == 0) {
				continue;
			}
			memmove(p,de.name,DIRSIZ);
			p[DIRSIZ] = 0;
			if(strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0) {
				continue;
			}
			find(buf,name);
		}
		break;
	}

	close(fd);
//	exit(0);
}



int main(int argc, char* argv[]) {
	if(argc < 3) {
		fprintf(2,"Usage: find <path> <filename>\n");
	}
	char* path = argv[1];
	char* name = argv[2];
//	fprintf(1,"%s\n",path);
//	fprintf(1,"%s\n",name);
	find(path,name);
	exit(0);
}


















