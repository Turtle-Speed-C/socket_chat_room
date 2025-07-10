#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define MAXLINE 200			//每个消息的最大长度
#define MAXNAME 20			//每个用户的最大用户名长度
#define SERV_PORT 8000		//服务器端口号
#define MAXCON (100 + 1) 	//最大连接数为100，最后一个用于临时连接
#define MAXFILE 10240		//最大文件缓存大小
#define FINISHFLAG "|_|_|"	//文件上传完成表示

struct sockaddr_in servaddr,chiladdr[MAXCON];
socklen_t cliaddr_len;
int listenfd,connfd[MAXCON],n;

char buf[MAXCON][MAXLINE+50],	\	//每个客户端的消息缓存区，用于接收并存储来自客户端的消息。
	spemsg[MAXCON][MAXLINE+50],	\	//发送给特定用户的消息缓存区。
	filebuf[MAXCON][MAXFILE+50];	//用于存储文件内容的缓存区。
	
char str[INET_ADDRSTRLEN];			//存储客户端的IP地址字符串。
char names[MAXCON][MAXNAME];		//存储每个连接用户的名称。

int used[MAXCON],	\				//记录每个连接用户端是否使用。0代表未使用，1代表正在使用
	downloading[MAXCON];			//记录每个用户是否正在下载文件。0代表未使用，1代表正在使用

void *TRD(void *arg);	//处理客户端的消息
int Process(int ID);	//对接收的信息进行处理

int main(){
	//服务器初始化
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERV_PORT);
	
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	
	listen(listenfd,20);
	
	memset(names,0,sizeof(names));				//初始化每个连接用户的名称
	memset(used,0,sizeof(used));				//初始化每个连接用户端
	memset(downloading,0,sizeof(downloading));	//初始化每个用户是否正在下载文件
	
	//预先开启最大用户连接数个线程
	pthread_t tids[MAXCON];
	int index[MAXCON];			//用来存储每个线程的索引值
	for(int i=0;i<MAXCON;i++){	//为每个线程创建一个索引值
		index[i]=i;
	}
	for(int i=0;i<MAXCON;i++){
		int ret=pthread_create(&tids[i],NULL,TRD,&index[i]);
		if(ret!=0){
			printf("thread create failed");
			return 0;
		}
	}
	
	//服务器在监听端口时，不断地等待客户端连接。一旦有客户端连接到达，
	//它就会为该客户端分配一个ID并接收连接请求。
	printf("Accepting connections ...\n");

	while (1) {
		// 读取下一个为占用的ID，作为下一个连接的用户ID
		int nowID = 0;
		for(nowID = 0; nowID < MAXCON-1; nowID++)
			if(!used[nowID]) break;
		cliaddr_len = sizeof(cliaddr[nowID]);
		// 等待连接
		connfd[nowID] = accept(listenfd, (struct sockaddr *)&cliaddr[nowID], &cliaddr_len);
		
		//检查是否是临时连接
		if(nowID > MAXCON-1){
			// 再次进行判断是否连接数满，因为第一次判断后可能有用户退出
			for(nowID = 0; nowID < MAXCON-1; nowID++)
				if(!used[nowID]) break;
			if(nowID > MAXCON-1){
				// 服务器连接数满
				memset(spemsg[nowID], 0, sizeof(spemsg[nowID]));
				strcpy(spemsg[nowID], "Error(1): 聊天室人已满。");
				sendonemsg(connfd[nowID], spemsg[nowID]);
				close(connfd[nowID]);
				continue;
			}else{
				// 再次判断后未满，设置对应的ID
				connfd[nowID] = connfd[MAXCON-1];
				cliaddr[nowID].sin_family = cliaddr[MAXCON-1].sin_family;
				cliaddr[nowID].sin_port = cliaddr[MAXCON-1].sin_port;
				cliaddr[nowID].sin_addr.s_addr = cliaddr[MAXCON-1].sin_addr.s_addr;
			}
		}
		used[nowID] = 1; // 该ID有人占用
	}
	return 0;
}

inline int Process(int ID){
	char op[20];
	int p=0;
	
	if(buf[ID][0]==':'){
		
		while(buf[ID][0]!='0'&&buf[ID][0]!=' '){// 分离命令和参数 命令保存在op 参数保存在buf[ID]
			op[p]=buf[ID][0];
			p++;
			char temp[MAXLINE];
			strcpy(temp,buf[ID]+1);
			strcpy(buf[ID],temp);
		}
		
		op[p]='\0';
		while(buf[ID][0]!='0'&&buf[ID][0]==' '){
			char temp[MAXLINE];
			strcpy(temp,buf[ID]+1);
			strcpy(buf[ID],temp);			
		}
		
		if(op[1]=='n'){
			// 新用户登陆，判断是否重名
			// 重名返回错误信息
			for(int i=0;i<MAXCON-1;i++){
				if(used[i]){
					if(memcmp(name[i],buf[ID])==0){
						memset(spemsg[ID],0,sizeof(spemsg[ID]));
						strcpy(spemsg[ID],"Error(2): 姓名重复，请重试。");
						sendonemsg(connfd[ID],spemsg[ID]);
						return 0;
					}
				}				
			}
			
			// 不重名则发送提示信息
			strcpy(names[ID],buf[ID]);
			sprintf(buf[ID], "%s(%s:%d)进入了聊天室", names[ID],
					inet_ntop(AF_INET, &cliaddr[ID].sin_addr, str, sizeof(str)),
					ntohs(cliaddr[ID].sin_port));
			
			memset(spemsg[ID],0,sizeof(spemsg[ID]));
			strcpy(spemsg[ID],"成功进入聊天室");
			sendonemsg(ID);
				
			return 0;
		}
		
		if(op[1]=='r'){
			// 改名，判断是否重名
			// 重名返回错误信息
			char newname[MAXNAME];
			for(int i=0;i<MAXCON-1;i++){
				if(i=ID) continue;
				
				if(used[i]){
					if(memcmp(name[i],buf[ID])==0){
						memset(spemsg[ID],0,sizeof(spemsg[ID]));
						strcpy(spemsg[ID],"Error(2): 姓名重复，请重试。");
						sendonemsg(connfd[ID],spemsg[ID]);
						return 0;
					}
				}				
			}
			
			// 不重名则发送提示信息
			strcpy(newname,buf[ID]);
			memset(buf[ID],0,strlen(buf[ID]));
			sprintf(buf[ID], "%s(%s:%d)改名为%s", names[ID],
					inet_ntop(AF_INET, &cliaddr[ID].sin_addr, str, sizeof(str)),
					ntohs(cliaddr[ID].sin_port), newname);
			
			memset(names[ID],0,strlen(names[ID]));
			strcpy(names[ID],newname);
			memset(spemsg[ID], 0, sizeof(spemsg[ID]));
			strcpy(spemsg[ID], "改名成功");
			sendonemsg(ID);
			sendmsgtoall(ID);
			return 0;
		}
		
		if(op[1]=='q'){
			// 用户退出时，给其他所用用户发送提示信息
			memset(buf[ID],0,strlen(buf[ID]));
			sprintf(buf[ID], "%s(%s:%d)离开了聊天室", names[ID],
					inet_ntop(AF_INET, &cliaddr[ID].sin_addr, str, sizeof(str)),
					ntohs(cliaddr[ID].sin_port));
					
			memset(spemsg[ID], 0, sizeof(spemsg[ID]));
			sendmsgtoall(ID);
			memset(names[ID], 0, sizeof(names[ID]));
			return 1; // 给线程函数返回1，用作后续处理
		}
		
		if(op[1]=='s'){
			// 给请求的用户发送所有用户信息
			memset(spemsg[ID], 0, sizeof(spemsg[ID]));
			strcpy(spemsg[ID], "IP              Port   name");
			sendonemsg(connfd[ID], spemsg[ID]);
			
			for(int i=0;i<MAXCON-1;i++){
				if(used[i]){
					memset(spemsg[ID], 0, sizeof(spemsg[ID]));
					sprintf(spemsg[ID], "%-16s%-7d%s",
							inet_ntop(AF_INET, &cliaddr[i].sin_addr, str, sizeof(str)),
							ntohs(cliaddr[i].sin_port), names[i]);
					sendonemsg(connfd[ID], spemsg[ID]);
				}
			memset(spemsg[ID], 0, sizeof(spemsg[ID]));				
			}
			return 0;
		}
		
		if(op[1]=='f'){
			// 给请求的用户发送服务器端文件
			system("mkdir Server_File");
			system("ls Server_File > ./file.txt");
			FILE* filename=fopen("./file.txt","r");
			
			memset(spemsg[ID], 0, sizeof(spemsg[ID]));
			while(fgets(spemsg,MAXLINE,filename)!=NULL){
				spemsg[ID][strlen(spemsg[ID])-1]='\0';
				sendonemsg(connfd[ID],spemsg[ID]);
			}
			system("rm -r file.txt");
			
			return 1;
		}
		
		if(op[1]=='u'){
			// 服务器端接收文件
		}
		
		if(op[1]=='d'){
			// 服务器发送文件
		}
	}
	
	
	
}

inline void sendonemsg(int sockfd,char* msg){

	strcat(msg,"\n");
	
	write(sockfd,msg,strlen(msg));
	
}


inline void sendmsgtoall(int ID){
	
	if(strlen(spemsg[ID])!=0){
		
		sendonemsg(connfd[ID],spemsg[ID]);
	}
	
	for(int i=0;i<MAXCON-1;i++){
		if(i==ID){
			continue;
		}
		else if(used[i]&&(!downloading[i])){
		
			sendonemsg(connfd[i],buf[ID]);
			
		}		
	}
	
}


//TRD是每个客户端连接所对应的线程函数。作用是处理客户端的消息交互，接受客户端的数据
//处理数据并会送回应。当客户端断开时，线程还需要通知其他用户并清理资源。
void *TRD(void *arg){		//创建的时候传入的是NULL
	int ID=*(int *)arg;
	
	while(1){
		while(!used[ID]);	//只有由用户连接，才可以使用
		memset(buf[ID],0,sizeof(buf[ID]));
		
		n=read(connfd(ID),buf[ID],,MAXLIEN);
		if(n<0){
			
			sprintf(buf[ID],"%s(%s:%d)离开聊天室",name[ID],	\
				inet_ntop(AF_INET,&cliaddr[ID].sin_addr,str,sizeof(str)),	\
				ntohs(cliaddr[ID].sin_port));
			
			memset(spemsg[ID],0,sizeof(spemsg[spemsg[ID]]));
			sendmsgtoall(ID);
			close(connfd[ID]);
			memset(name[ID],0,strlen(name[ID]));
			used(ID)=0;
		}
		
		buf[ID][n]=0;
		printf("Received from %s at PORT %d: %s\n",	\
				inet_ntop(AF_INET,&cliaddr[ID].sin_addr,str,sizeof(str)),
				ntohs(cliaddr[ID].sin_port));
				
		int q=Process(ID);
		
		if(q==1){
			close(connfd[ID]);
			used[ID]=0;
		}
		
	}
}
