#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#define MAXLINE 200			//每个消息的最大长度
#define MAXNAME 20			//每个用户的最大用户名长度
#define SERV_PORT 8000		//服务器端口号
#define MAXFILE 10240		//最大文件缓存大小
#define FINISHFLAG "|_|_|"	//文件上传完成表示

struct sockeaddr_in servaddr;
char buf[MAXLINE+50];
char recervemsg[MAXLINE+50];
char filebuf[MAXLINE+50];

int sockfd,n;

char IP[INET_ADDRSTRLEN+5];
int stop=0;
pthread_t tid;


int isIP(char* IP);
void startlistening();
void* listening();

int main() {

    printf("输入服务器IP（本机输入：127.0.0.1）：\n");
    fgets(IP,INET_ADDRSTRLEN+5,stdin);
    IP[strlen[IP]-1]='\0';
    if(isIP[IP]){
        printf("请重新输入（本机输入：127.0.0.1）：\n");
        fgets(IP,INET_ADDRSTRLEN+5,stdin);
        IP[strlen[IP]-1]='\0';
    }
    IP[strlen(IP)]='\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, IP, &server_addr.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(server_addr))==0){
        perror("无法连接到服务器");
        return 0;
    }

    startlistening();
    stop=1;

    getname(0);     // 首次输入姓名
    while(stop);    // 此时在判断是否重名，暂停主函数运行
    printf("输入 ':q' 退出聊天室\n");
	printf("输入 ':r' 改名\n");
	printf("输入 ':s' 显示所有在线用户\n");
	printf("输入 ':f' 显示所有云端文件\n");
	printf("输入 ':u' 上传文件\n");
	printf("输入 ':d' 下载文件\n");

    while(fget(buf,MAXLINE,stdin)!=NULL){
        buf[strlen(buf)-1]='\0'; // 去掉换行符
        int quit=0;

        if(buf[0]==':'){
            if(buf[1]=='q'){ // 退出聊天室
                quit=1;
            }
            else if(buf[1]=='r'){ // 改名
                stop=1; // 暂停主函数运行
                getname(1);
                while(stop); // 等待改名是否重名
                memset(buf, 0, sizeof(buf));
                continue; // 继续输入
            }
            else if(buf[1]=='s'){ // 显示在线用户
            }
            else if(buf[1]=='f'){ // 显示云端文件
            }
            else if(buf[1]=='u'){ // 上传文件
                stop=1; // 暂停主函数运行
                upload_file(); // 上传文件函数
                while(stop); // 等待上传是否成功
                memset(buf, 0, sizeof(buf));
                continue; // 继续输入
            }
            else if(buf[1]=='d'){ // 下载文件
                stop=1; // 暂停主函数运行
                download_file(); // 下载文件函数
                while(stop); // 等待下载是否成功
                memset(buf, 0, sizeof(buf));
                continue; // 继续
            }
        }else{
            printf("输入格式错误，请重新输入。\n");
            memset(buf, 0, sizeof(buf));
            continue; // 继续输入
        }
        sendonemsg(buf); // 发送消息
        memset(buf, 0, sizeof(buf)); // 清空输入缓冲区
        if(quit){ // 如果是退出聊天室
            pbreak; // 退出循环
        }
    }
    close(sockfd); // 关闭套接字
    return 0;
}

int isIP(char* IP){
    int n=strlen(IP);
    int np=0;
    int num=0;

    for(int i=0;i<=n;i++){
        if(IP[i]=='.'||i==n){
            np++;
            if(num>255)
                return 0;
            sum=0;
        }else if(IP[i]>='0' && IP[i]<='9'){
            sum=sum*10+IP[i]-'0';
        }
        else return 0;
    }
    if(np==4){
        return 1;
    }else{
        return 0;
    }
}

void startlistening(){
    // 重新启动监听线程，会创建一个全新的线程
    // 如果之前的监听线程已经被 pthread_cancel 取消了，这里会新建一个线程来继续监听服务器消息
    int rt=pthread_creat(&tid,NULL,listening,NULL);
    if(rt!=0){
        printf("Fail to create a new thread.");
		exit(0);
    }
    rt=pthread_detach(tid);
    if(rt!=0){
        printf("Fail to detach the thread.");
		exit(0);
    }
}

void* listening(){
    while(1){
        memset(recervemsg, 0, sizeof(recervemsg));
        n = read(sockfd, recervemsg, MAXLINE);
        if(n <= 0){
            perror("服务器断开连接");
            close(sockfd);
            exit(0);
        }
        else{
            printf("%s",recervemsg);
        }
        if(receivemsg[0]!='E') stop = 0; // 如果出错，即重名等情况，暂停主函数运行
		if(receivemsg[0]=='E' && receivemsg[6]=='1') exit(0);
		if(receivemsg[0]=='E' && receivemsg[6]=='2') get_name(0);
		if(receivemsg[0]=='E' && receivemsg[6]=='3') get_name(1);
		if(receivemsg[0]=='E' && receivemsg[6]=='4') stop = 0;
		if(receivemsg[0]=='E' && receivemsg[6]=='5') stop = 0;
		// 六种错误代码：
		// 1: 聊天室人满，退出程序
		// 2: 首次输入姓名重名，重新进行输入姓名
		// 3: 改名时姓名重名，重新进行输入姓名
		// 4: 服务器没有成功新建文件，上传失败，主函数继续运行
		// 5: 上传时，服务器中存在相同文件，上传失败，主函数继续运行
		// 6: 下载时，服务器中不存在该文件，下载失败，主函数继续运行
		//    下载文件时关闭了此线程，此错误处理写在下载文件函数内
        
        // 处理接收到的消息
        // 这里可以根据具体的协议进行解析和处理
    }
}

void sendonemsg(char* msg){
    strcat(msg, "\n");
    write(sockfd, msg, strlen(msg));
}

void getname(int mode){
    char name[MAXNAME];
    printf("请输入您的姓名（不超过20个字符）：\n");
    fgets(name, MAXNAME, stdin);
    memset(buf,0,sizeof(buf));

    if(mode==0){
        strcat(buf,":n ");
    }
    else if(mode==1){
        strcat(buf,":r ");
    }

    strcat(buf, name);
    sendonemsg(buf);
} 

void upload_file(){
    printf("输入文件路径及文件名(for example ./client/filename 或完整路径)：\n");
    char filename[MAXLINE];
    fgets(filename, MAXLINE, stdin);
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL){
        printf("Error: 无法打开文件 '%s'\n", filename);
        memset(buf, 0, sizeof(buf));
        stop = 0; // 继续主函数运行
        return;
    }

    //计算文件的字节数
    struct stat st;
    stat(filename, &st);
    int size=st.st_size;
    int total = 0;

    //清除路径，只保留文件名，并转换为命令+参数格式发送给服务器
    int nn=strlen(filename)-1;
    while(nn>=0 && filename[nn]!='/'){
        nn--;
    }
    while(nn+1){
        strcpy(filename,filename+1);
        nn--;
    }
    memset(buf,0,sizeof(buf));
    sprintf(buf,":u %s",filename);
    sendonemsg(buf);

    usleep(100000); // 等待服务器进行处理
	// 此处判断是否发送错误4或5，出现则不再发送
    if(stop==0){
        fclose(fp);
        return;
    }

    memset(filebuf, 0, sizeof(filebuf));
    pthread_cancel(tid); // 取消监听线程，避免阻塞

    while((nn = fread(filebuf, sizeof(char), MAXFILE, fp)) > 0){
        write(sockfd, filebuf, nn);
        total += nn;
        memset(filebuf, 0, sizeof(filebuf));
    }
    
    //开始传输
    while(nn=fread(filebuf,sizeof(char),MAXFILE,fp)){
        if(nn==0) break;
        total+=nn;
        printf("%6.2f%%", (float)total/(size)*100.0); // 显示已发送的百分比
        write(sockfd,filebuf,MAXFILE);
        printf("\b\b\b\b\b\b\b");
        memset(filebuf,0,sizeof(filebuf));
    }

    startlistening();
    strcpy(buf,FINISHFLAG);
    usleep(1000000); 	// 等待服务器处理完最后一个数据包后
	sendonemsg(buf);	// 发送结束标志
	stop = 0;			// 主函数继续运行
	fclose(fp);
}

void download_file(){
    pthread_cancel(tid);    //下载时关闭接收信息线程，此函数进行接收

    //输入并发送下载命令
    printf("输入服务器上的文件名:\n");
	char filename[MAXLINE - 10];
	scanf("%s", filename);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, ":d %s", filename);
	sendonemsg(buf);
	usleep(10000);

    n=read(sockfd,recervemsg,MAXLINE);
    int size=0;
    if(recervemsg[0]=='E'&&recervemsg[6]=='6'){
        puts(recervemsg);
        stop=0;
        startlistening();
        return ;
    }else{
        // recervemsg 是一个字符串，内容是服务器发来的文件大小（比如 "10240"）。
        // atoi 是 C 标准库函数（ASCII to Integer），可以把字符串形式的数字转换为整型数值。
        size=atoi(recervemsg);
    }

    FILE* fp = fopen(filename, "wb");
    if(fp == NULL){
        printf("打开文件失败\n");
        stop = 0; // 继续主函数运行
        startlistening();
        return ;
    }
    int total = 0;

    char command[MAXLINE];
    sprintf(command, "rm -f %s", filename);
    while(1){
        memset(filebuf, 0, sizeof(filebuf));
        printf("%6.2f%%", (float)total/(size)*100.0); // 输出已下载的百分比
        n = read(sockfd, filebuf, MAXFILE);
        printf("\b\b\b\b\b\b\b"); // 清除百分比显示
        if(n <= 0){
            printf("下载失败，服务器断开连接或传输错误。\n");
            fclose(fp);
            system(command); // 删除未完成的文件
            exit(0);
        }
        if(strcmp(filebuf, FINISHFLAG) == 0){
            printf("文件下载完成。\n");
            break;
        }
        fwrite(filebuf, sizeof(char), n, fp);
        total += n;
        fflush(fp); // 确保数据写入文件
    }

}

// 当主线程需要等待某些操作（如改名、上传、下载等）完成时，
// 会把 stop 设为 1，主线程就会 while(stop); 暂停等待。
// 当监听线程（listening 函数）收到服务器的响应，并且操作完成或出错时，
// 会把 stop 设为 0，主线程就会继续运行。