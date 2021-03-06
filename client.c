#include "myhead.h"

#define INVALID_USERINFO 'n'
#define VALID_USERINFO   'y'


/*函数声明*/
int get_userinfo(char *buf, int len, char *string);
void input_userinfo(int conn_fd, char *string);
// void menu_login(int conn_fd);
void login(int conn_fd);
void my_register(int conn_fd);
void menu_login(int);
void menu_chat(int);
void * recv_thread(void *);

int taccount;
char tname[NAMESIZE];
pthread_mutex_t mutex;
pthread_cond_t cond;

void err(const char *string, int line) {
    perror(string);
    printf("line: %d\n", line);
    exit(1);
}

int main(int argc, char **argv) {
    int conn_fd, serv_port;
    struct sockaddr_in serv_addr;
    char recv_buf[BUFSIZE];
    int i, ret;
    pthread_t tid;

    if(argc != 3)
        err("The number of argc", __LINE__);

    /*初始化服务器端地址结构*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    /*检查参数*/
    if(!strcmp(argv[1], "-a")) {
        if(inet_aton(argv[2], &serv_addr.sin_addr) < 0) {
            printf("invalid server ip address\n");
            exit(1);
        }
    } else {
        printf("there should be [ -a ] [ server_ip ]\n");
        exit(1);
    }

    /*创建socket*/
    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(conn_fd < 0)
        err("socket", __LINE__);

    /*向服务器发送连接请求*/
    if(connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0)
        err("connect", __LINE__);
    
    menu_login(conn_fd);
    pthread_create(&tid, NULL, recv_thread, &conn_fd);

    menu_chat(conn_fd);

    close(conn_fd);

    return 0;
}


/*子线程接收服务器数据*/
void do_recv (struct message info, int conn_fd) {
    int n = info.n;
// printf("n = %d\n", n);
    switch(n) {
        case 1: {  //私聊
            printf(BLUE"%s (%d) %s"END, info.name_from, info.account_from, info.time);
            printf(BLUE"%s"END, info.buf);
            printf("\n");
        }
        break;

        case 101: {
            printf(RED "无该好友\n" END);
        }
        break;

        case 102: {
            printf(RED "该帐号未注册\n" END);
        }
        break;
        
        case 103: {
            printf(RED "已经是好友\n" END);
        }
        break;

        case 2: {
            printf(BLUE "[群聊] %d -- %s %s\n" END, info.account_from, info.time, info.buf);
        }
        break;


        case 3: {  //输出聊天记录
            printf(GREEN "%s\n" END, info.buf);
        }
        break;

        case 30: {
            printf(RED "Error: you are not a group member\n" END);
        }
        break;


        case 31: {
            printf(BLUE "%s" END, info.buf);
        }
        break;

        case 4: {  //输出在线状态
            printf(GREEN "friends online:\n" END);
            for(int i = 0; i < info.num; i++)
                printf(GREEN "%d\n" END, info.state[i][0]);
        }
        break;


        case 5: {  //添加好友
            printf(GREEN "%s (%d) %s请求添加你为好友\n" END, info.name_from, info.account_from, info.time);
        }
        break;


        case 6: {  //删除好友
            printf(GREEN "%s (%d)\n\n" END, info.name_from, info.account_from);
        }
        break;

        case 7: {  //查看好友列表
            printf(GREEN "%d\n" END, info.account_to);
        }
        break;


        case 8: {  //加群

            if(info.flag == 3)
                printf(GREEN "无更多添加请求\n" END);
            else {

                printf(GREEN "friend %d invite you to join group %d. (1 to agree, 0 to disagree): " END, info.account_to, info.group);
                scanf("%d", &info.flag);

                fflush(stdin);

                if(info.flag == 1) {
                    info.n = 81;
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else if(!info.flag){  
                    info.n = 80;
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else {
                    printf(RED "number input error\n" END);
                }
            }
            pthread_cond_signal(&cond);
        }
        break;

        case 80: {
            printf(RED "该群（%d）已添加\n" END, info.group);
        }
        break;

        case 81: {
            printf(RED "该群（%d）尚未被创建\n" END, info.group);
        }
        break;

        case 9: {
            printf(GREEN "Quit group %d success\n" END, info.group);
        }
        break;

        case 90: {
            printf(RED "you are header of this group, can not quit group\n" END);
        }
        break;

        case 10: {
            printf(GREEN "Create new group success, account: %d\n" END, info.group);
        }
        break;

        case 11: {
            printf(GREEN "Delete group success\n" END);
        }
        break;

        case 100: {
            printf(RED "This group account %d already exists.\n" END, info.group);
        }
        break;


        case 110: {
            printf(RED "解散群出错，你不是群主\n" END);
        }
        break;

        case 12: {
            printf(GREEN "%s%d 邀请你加入群%d\n" END, info.time, info.account_from, info.group);
        }
        break;

        case 131: {
            if(info.flag == 3)
                printf(GREEN "无更多添加请求\n" END);
            else {
                printf(GREEN "friend's account %d (1 to agree, 0 to disagree): " END, info.account_to);
                scanf("%d", &info.flag);
                if(info.flag == 1) {
                    info.n = 1311;
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);

                } else if(!info.flag){  
                    info.n = 1310;
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else {
                    printf(RED "number input error\n" END);
                }
            }
            pthread_cond_signal(&cond);
        }
        break;


        case 132: {
            if(info.flag == 3)
                printf(GREEN "无更多添加请求\n" END);
            else {
                printf(GREEN "friend's account %d to join group %d. (1 to agree, 0 to disagree): " END, info.account_to, info.group);
                scanf("%d", &info.flag);

                if(info.flag == 1) {
                    info.n = 1321;

                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else if(!info.flag){  
                    info.n = 1320;
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else {
                    printf(RED "number input error\n" END);
                }
            }
            pthread_cond_signal(&cond);
        }
        break;

        case 14: {
            int i;
            printf(GREEN "Group member:\n\n" END);
            for(i = 1; i < info.num; i++) {
                printf(GREEN "%d\n" END, info.state[i][0]);
            }
        }
        break;

        case 16: {
            FILE *fp;
            
            pthread_mutex_lock(&mutex);

printf("file = %s\n%s\n", info.filename, info.buf);

            fp = fopen(info.filename, "at+");
            fputs(info.buf, fp);
            fclose(fp);
            pthread_mutex_unlock(&mutex);
        }
        break;

        case 161: {
        	FILE *fp;
            printf(GREEN "Receiving file: %s\n" END, info.filename);

            fp = fopen(info.filename, "w");
            fclose(fp);
        }
        break;

        case 162:{
            printf(GREEN "File %s received\n" END, info.filename);
        }
        break;
    }
}

/*子线程接收服务器数据*/
void *recv_thread(void *arg) {
    struct message info_recv;
    int conn_fd = *(int *)arg;
    int ret;

    while(1) {
    	int sum = 0;
    	while(sum != sizeof(info_recv)) {
	        if((ret = recv(conn_fd, &info_recv, sizeof(info_recv), 0)) == 0) {
	        	printf(RED "server exit.\n" END);
	            pthread_exit(NULL);
	        }

	        if(ret < 0) {
	            err("recv", __LINE__);   
	        }

	        sum += ret;

// printf("recv_size = %d\n", ret);
if(ret != sizeof(info_recv))
	sleep(2);

	    }
	    
	    do_recv(info_recv, conn_fd);
    }
}

/*登陆函数*/
void login(int conn_fd){
    int i;
    input_userinfo(conn_fd, "account");
    input_userinfo(conn_fd, "password");

    fprintf(stderr, GREEN"Login success. Please wait for a while."END);
    for(i = 0; i < 2; i++){
        sleep(1);
        fprintf(stderr, ".");
    }
}

/*获取用户输入信息*/
int get_userinfo(char *buf, int len, char *string) {
    int i;

    if(buf == NULL)
        return -1;
    i = 0;
    while((buf[i++] = getchar()) != '\n' && i < len-1)
        ;
    buf[i-1] = 0;

    taccount = atoi(buf);
    
    return 0;
}

/*输入信息，提交到服务器*/
void input_userinfo(int conn_fd, char *string) {
    char recv_buf[ BUFSIZE ];
    char *input_buf = (char *)malloc(32);
    int flag_userinfo = 0;

    do {
        printf(GREEN "%s : " END, string);
        if(!strcmp("account", string)){
            if(get_userinfo(input_buf, NAMESIZE, string) < 0)
                err("error return from get_userinfo", __LINE__);
        } else {
            input_buf = getpass("");
        }
        if(send(conn_fd, input_buf, sizeof(input_buf), 0) < 0)
            err("send", __LINE__);
        /*读取套接字数据*/
        if(recv(conn_fd, recv_buf, sizeof(recv_buf), 0) < 0)
            err("recv", __LINE__);

        if(recv_buf[0] == VALID_USERINFO) {
            flag_userinfo = VALID_USERINFO;
        } else {

            printf(RED "%s error, input again\n\n" END, string);
            flag_userinfo = INVALID_USERINFO;
        }
    } while(flag_userinfo == INVALID_USERINFO);
}


/*注册函数*/
void my_register( int conn_fd ) {
    int i;
    char recv_buf[ BUFSIZE ];
    int account = 0, flag = 0, flag_a;
    // char tpasswd[ PASSWDSIZE ];
    char *tpasswd, *passwd;
    struct userinfo info;

    memset(&info, 0, sizeof(info));

    /*帐号*/
    while(1) {
        flag_a = 0;

        printf(GREEN "Please input your account number:\n" END);

        scanf("%s", recv_buf);
        if(recv_buf[0] == '0') {
            printf(RED "The first number should not be 0\n\n" END);
            continue;
        }

        i = 0;
        while(recv_buf[i++]) {
            if(!(recv_buf[i-1] >= '0' && recv_buf[i-1] <= '9')) {
                printf(RED "The account format is incorrect\n\n" END);
                flag_a = 1;
                break;
            }
        }

        if(!flag_a) {
            recv_buf[i-1] = 0;
            info.account = atoi(recv_buf);
        } else {
            continue;
        }

        fflush(stdin);

        if(!info.account)
            return;

        if(send(conn_fd, &info.account, sizeof(int), 0) < 0)
            err("send", __LINE__);

        if(recv(conn_fd, recv_buf, sizeof(recv_buf), 0) < 0)
            err("recv", __LINE__);

        if(recv_buf[0] == 'y')
            break;
        else
            printf("This account has existed, input again:\n");
    }

    /*用户名*/
    printf("Please input your nick name (no ' '):\n");
    scanf("%s", info.name);


    /*密码*/
    flag = 0;
    while(1) {
        
        if(!flag){
            passwd = getpass("Please input password:(-1 to exit)\n");
            flag = 1;
        } else {
            passwd = getpass("The password is inconsistent, input again:\n");
        }

        if(!strcmp(passwd, "-1"))
            return ;

        tpasswd = getpass("Please input password again:\n");

        if(!strcmp(passwd, tpasswd)){
            memcpy(info.passwd, passwd, sizeof(passwd));
            if(send(conn_fd, (char *)&info, sizeof(info), 0) < 0)
                err("send", __LINE__);
        }

        if(recv(conn_fd, recv_buf, sizeof(recv_buf), 0) < 0)
            err("recv", __LINE__);
        if(recv_buf[0] == 'y'){
            printf("Congratulations! Register success!\n");
            sleep(1);
            fflush(stdin);
            break;
        }
    }
}


void menu_login(int conn_fd) {

    int choice;
    char recv_buf[ BUFSIZE];

    do{
        CLEAR; //clear the screen
        printf(GREEN);
        printf("----------------------------------------\n");
        printf("---      Welcome to my chatroom      ---\n");
        printf("----------------------------------------\n");
        printf("---                                  ---\n");
        printf("---         1. login                 ---\n");
        printf("---         2. register              ---\n");
        printf("---         0. exit                  ---\n");
        printf("---                                  ---\n");
        printf("----------------------------------------\n\n");
        printf("Please input a number(0~2):\n");
        printf(END);
        
        scanf("%d", &choice);
        getchar();
        fflush(stdin);

        switch(choice) {
            case 1: {   //login
                if(send(conn_fd, "1", 2, 0) < 0)
                    err("send", __LINE__);

                if(recv(conn_fd, recv_buf, sizeof(recv_buf), 0) < 0)
                    err("recv", __LINE__);

                if(recv_buf[0] == 'y'){  //登录成功
                    login(conn_fd);
                    choice = 0;
                    break;
                }                
                else
                    printf(RED "connect with server error\n\n" END);

                break;
            }

            case 2: {   //register
                if(send(conn_fd, "2", 2, 0) < 0)
                    err("send", __LINE__);

                if(recv(conn_fd, recv_buf, sizeof(recv_buf), 0) < 0)
                    err("recv", __LINE__);

                if(recv_buf[0] == 'y')  //已连接服务器
                    my_register(conn_fd);  // 开始注册
                else
                    printf(RED "connect to server error\n\n" END);
 
                break;
            }

            case 0:    //退出并修改在线状态
                if(send(conn_fd, "0", 2, 0) < 0)
                    err("send", __LINE__);
                pthread_exit(NULL);

            default: {
                printf(RED "Input error. The number should be 0 ~ 2\n" END);
                printf(RED "Press any key to continue...\n\n" END);
                getchar();
                break;
            }
        }

    } while(choice);
}


void menu_chat(int conn_fd) {
    int choice;
    char recv_buf[ BUFSIZE ];
    struct message info;
    int account;

    info.sock_from = conn_fd;
    info.account_from = taccount;

    info.n = 999;
    CLEAR;
    printf(GREEN "离线消息：\n" END);
    if(send(conn_fd, &info, sizeof(info), 0) < 0)
        err("send",__LINE__);

    getchar();

    do {
        CLEAR;
        printf(GREEN);
        printf("------------------------------------------------\n");
        printf("---          Welcome to my chatroom          ---\n");
        printf("------------------------------------------------\n");
        printf("---                                          ---\n");
        printf("---         1. Private chat                  ---\n");  //私聊
        printf("---         2. Group chat                    ---\n");  //群聊
        printf("---         3. Chat log                      ---\n");  //聊天记录
        printf("---         4. Friend's state                ---\n");  //在线状态
        printf("---         5. Add friend                    ---\n");  //添加好友
        printf("---         6. Delete friend                 ---\n");  //删除好友
        printf("---         7. Display friends               ---\n");  //好友列表
        printf("---         8. Add group                     ---\n");  //加群
        printf("---         9. Quit group                    ---\n");  //退群
        printf("---        10. Create group                  ---\n");  //建群
        printf("---        11. Delete group                  ---\n");  //解散群
        printf("---        12. Invite friend into group      ---\n");  //邀请好友加群
        printf("---        13. Deal with invitation          ---\n");  //处理添加请求
        printf("---        14. Display group member          ---\n");  //查看群成员
        printf("---        15. Send file                     ---\n");  //发送文件
        printf("---        16. Recieve file                  ---\n");  //jie shou文件
        printf("---         0. Exit                          ---\n");
        printf("---                                          ---\n");
        printf("------------------------------------------------\n\n");
        printf("Please input a number(0 ~ 12):\n");
        printf(END);
        
        choice = -1;
        scanf("%d", &choice);
// printf("choice = %d\n", choice);
        getchar();
        fflush(stdin);
        CLEAR;

        switch(choice) {
            
            case 1: {  //私聊
                printf(GREEN "Input your friend's account:\n" END);
                scanf("%d", &info.account_to);
                fflush(stdin);

                info.n = 1;

                while(1) {
                    fgets(info.buf, sizeof(info.buf), stdin);

                    if(!strcmp(info.buf, "exit\n"))
                        break;
                    if(strcmp(info.buf, "\n"))
                        if(send(conn_fd, &info, sizeof(info), 0) < 0)
                            err("send",__LINE__);

                }
            }
            break;


            case 2: {  //群聊

                info.n = 2;
                printf(GREEN "Input group account: " END);
                scanf("%d", &info.group);

                while(1) {
                    fgets(info.buf, sizeof(info.buf), stdin);

                    if(!strcmp(info.buf, "exit\n"))
                        break;
                    if(strcmp(info.buf, "\n"))
                        if(send(conn_fd, &info, sizeof(info), 0) < 0)
                            err("send",__LINE__);
                }
            }
            break;


            case 3: {//聊天记录
                int a;
                printf(GREEN "Input a number:\n" END);
                printf(GREEN "1. friend's chat_log\n" END);
                printf(GREEN "2. group's chat_log\n" END);
                scanf("%d", &a);
                getchar();
                fflush(stdin);

                if(a == 1) {
                    info.n = 3;
                    printf(GREEN "Input account:\n" END);
                    scanf("%d", &info.account_to);
                    fflush(stdin);
                    
                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                } else {

                    info.n = 31;
                    printf(GREEN "Input account:\n" END);
                    scanf("%d", &info.group);
                    fflush(stdin);

                    if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                }

                getchar();
                getchar();

            }
            break;


            case 4: {//在线状态
                info.n = 4;

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                getchar();
            }
            break;


            case 5: {  //添加好友
                info.n = 5;

                printf(GREEN "Input account: " END);
                scanf("%d", &info.account_to);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);
            }
            break;


            case 6 : {  //删除好友
                info.n = 6;
                printf(GREEN "Input account to delete: " END);
                scanf("%d", &info.account_to);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);
            }
            break;


            case 7: {  //查看好友列表
                info.n = 7;

                // pthread_mutex_lock(&mutex);
                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);
                getchar();
                // pthread_cond_wait(&cond, &mutex);
                // pthread_mutex_unlock(&mutex);
            }
            break;


            case 8:{  //加群
                info.n = 8;

                printf(GREEN "Input account to add: " END);
                scanf("%d", &info.group);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                getchar();
                getchar();
            }
            break;


            case 9: {  //退群
                info.n = 9;
                printf(GREEN "Input group account to delete: " END);
                scanf("%d", &info.group);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                getchar();
                getchar();
            }
            break;


            case 10: {  //建群
                info.n = 10;
                printf(GREEN "Input new account: " END);
                scanf("%d", &info.group);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                getchar();
                getchar();
            }
            break;


            case 11: {  //解散群
                info.n = 11;

                printf(GREEN "Input group account to delete(-1 to exit): " END);
                scanf("%d", &info.group);

                if(info.group == -1)
                    break;

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                getchar();
                getchar();
            }
            break;


            case 12: {  //邀请好友加群
                info.n = 12;

                printf(GREEN "Input your friend's account: \n" END);
                scanf("%d", &info.account_to);
                printf(GREEN "Input group account: \n" END);
                scanf("%d", &info.group);
                fflush(stdin);

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);
                getchar();
                getchar();
            }
            break;


            case 13: {  //处理添加好友信息
                int a;
                pthread_mutex_lock(&mutex);
                printf(GREEN "Input a number:\n" END);
                printf(GREEN "1. friend's invitation\n" END);
                printf(GREEN "2. group's invitation\n" END);
                printf(GREEN "0. exit\n" END);
                scanf("%d", &a);
                getchar();
                fflush(stdin);

                if(a == 1) {
                    info.n = 131;
                    // printf(GREEN "Input 0 or 1: " END);
                    // scanf("%d", );
                } else if(a == 2) {
                    info.n = 132;
                    // printf(GREEN "Input group account: " END);
                    // scanf("%d", &info.group);
                } else if(a == 0) {
                    break;
                } else {
                    printf(RED "input error\n" END);
                    getchar();
                    break;
                    // getchar();
                }
                // getchar();

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);

                pthread_cond_wait(&cond, &mutex);
                pthread_mutex_unlock(&mutex);
                getchar();
            }
            break;


            case 14: {
                info.n = 14;
                printf(GREEN "Input group account to display member(-1 to exit): " END);
                scanf("%d", &info.group);
                fflush(stdin);

                if(info.group == -1)
                    break;

                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                    err("send", __LINE__);
                getchar();
                getchar();
            }
            break;


            case 15: {
            	info.n = 15;
            	char filename[255];

                FILE *fp;
            	printf(GREEN "Input filename (absolute path):\n" END);
            	fgets(filename, sizeof(filename), stdin);

            	int len = strlen(filename), i, j;
            	filename[len - 1] = 0;
            	for(i = 0, j = 0; i < len ; i++) {
            		if(filename[i] == '/') {
            			j = 0;
            		}
            		info.filename[j++] = filename[i];
            	}
            	info.filename[j] = 0;
// printf("file = %s, -%s-\n", filename, info.filename);
            	printf(GREEN "Input your friend's account:\n" END);
            	scanf("%d", &info.account_to);
            	fflush(stdin);
                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);

                info.n = 151;
                printf("sending...\n");

            	pthread_mutex_lock(&mutex);
            	fp = fopen(filename, "r");
            	if(fp == NULL) {
            		printf(RED "Open file %s error\n" END, filename);
            		break;
            	}

            	while(fgets(info.buf, sizeof(info.buf), fp) != NULL) {
            		if(send(conn_fd, &info, sizeof(info), 0) < 0)
                        err("send", __LINE__);
                        usleep(70000);  
            	}
            	fclose(fp);

                printf("file send success\n");

            	pthread_mutex_unlock(&mutex);

            	getchar();
            	getchar();
            }
            break;


            case 16: {
                info.n = 16;
                if(send(conn_fd, &info, sizeof(info), 0) < 0)
                	err("send", __LINE__);

                getchar();
            }
            break;

            case 0 :{
                return;
            }
            break;

            default: {
                printf(RED "Input error\n" END);
                sleep(1);
            }
            break;
        }
    } while(choice);
}