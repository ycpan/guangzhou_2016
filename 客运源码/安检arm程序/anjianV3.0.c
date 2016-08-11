/*2016-03-14 客运第安检版*/
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h>

#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h> 
#include <assert.h>
/*sqlite3 头文件*/
#include <sqlite3.h>  
#include <stddef.h>
/*FIONREAD*/
#include <sys/ioctl.h>

#define TIME_OUT_TIME 10    //time_out 10s
#define MAXSIZE 1024

int Serial_num=0;

#define R1 0    //定义每个路由在数组中的位置11
#define R2 1	//12
#define R3 2	//13
#define R4 3	//14
#define R5 4	//15
#define R6 5	//16
#define R7 6	//17
#define R8 7	//18	
#define R9 8	//19
#define acc 6  			//定位精度，收集的信号强度幀数
/*****************************************************************
传入数组及路由编号
返回对应的信号强度
******************************************************************/
int findRssi(int buff[11],int routeid);
/*****************************************************************
传入用于比较的二维数组以及要求差值的路由编号
返回最大值与最小值的差
******************************************************************/
int makesub(int tempbuff[acc][11],int routeID);                ////////if it ==0;    neeed debug

/*****************************************************************
传入一帧数据，找到最大的Rssi
返回最大值所在路由的编号
******************************************************************/
int findmaxRssiaddr(int buff[11]);

/*****************************************************************
传入一个路由编号，找到它所出现的次数
返回他所出现的次数
******************************************************************/
int findRsutimes(int tempbuff[acc][11],int routeID);

/*****************************************************************
传入一个二维数组和指定路由编号
返回指定路由所上的的平均值
******************************************************************/
int getAverRssi(int tempbuff[acc][11],int routeID);


unsigned char buffer[1024],buff[1024],temp_buff[1024],data_length,buff_rec[20];
unsigned char lenbuffer[100];/*存储二维数组长度的数组*/
int uart_fd;
void init_uart(void);
int open_uart(void);
/*数据库操作的回调函数*/
static int _sql_callback(void *notused, int argc, char **argv, char **szColName)  ;
int local_callback(void * data, int col_count, char ** col_values, char ** col_Name);

//const char *sSQL1_create_table = "create table Rssi_table(serial_num int, obu_id varchar(20), route1_id varchar(20),route1_val int,route2_id varchar(20),route2_val int,route3_id varchar(20),route3_val int,route4_id varchar(20),route4_val int,time varchar(50));";  
const char *sSQL1_create_table = "create table Rssi_table(serial_num int, obu_id varchar(20), route1_id int,route1_val int,route2_id int,route2_val int,route3_id int,route3_val int,route4_id int,route4_val int,time varchar(50));"; 

const char *sSQL1_create_local_table ="create table postable(serial_num int,obu_id varchar(20),pos int,time varchar(50));";  

const char *SQL_POS= "select * from postable by obu_id desc";
char SQL_Insert[200];
char SQL_Delete[200];
unsigned int local_num = 0;

unsigned int Serial_Id_Sql = 0;
/*有关日志功能的函数声明*/
FILE* openfile(const char *fileName,const char *mode);
int getTime(char *out,int fmt);
int writeFile(FILE *fp,const char *str,int blog);
int closeFile(FILE *fp);
int convet(char *src,char *dest,int length);/*转换16进制到字符串*/
unsigned long convert_atohex(char* str);/*字符串转16进制*/
unsigned long convert_atohex1(char* str,char *buff_hex);
time_t first;/*获取初始的时间*/
time_t heartbeat_t;/*汇聚心跳包*/
time_t jinchuzhan_beat;


unsigned char DEBUG ;
unsigned char debug_buf[5];

/*merge locate_taxier_V0.5.c*/
typedef unsigned char bool;
#define true 1
#define false 0
bool  cmpstr(char *src,char *dest,int t);
bool cmp_obu(char *obu,char *dest,int t);
static int _sql_callback(void *notused, int argc, char **argv, char **szColName);

//int getTime(char *out,int fmt);
const char *sSQL3 = "select * from Rssi_table;";  
const char *sSQL_delete = "delete  from Rssi_table;";  
const char *sSQL_pos_del = "delete  from postable;";  
const char *SQL_first = "select * from Rssi_table limit 1;";
const char *SQL_order = "select * from Rssi_table order by obu_id desc;";
char SQL_insert_local[100];

/*进出站*/
#define TAXI_GET_IN        0
#define TAXI_GET_OUT     1
#define PARK_NUM            400
int PARK_STATE[PARK_NUM] = {0};

unsigned char GET_IN_AND_OUT[50];







/*above locate_taxier_V0.5.c*/

unsigned int crc16(unsigned char *buf, unsigned int len);
ssize_t nread(int fd,unsigned char *ptr);

unsigned int crc16(unsigned char *buf, unsigned int len);
void *save_msg();
void *send_msg();
void *rec_ask();
void *info_from_service_and_send_to_uart();
void *locate_process();
unsigned int crc16_serial(unsigned char *buf, unsigned int len);
ssize_t xread(int fd,void *ptr,size_t n) ;


unsigned int CRC,k;
unsigned int dev_id;
unsigned int data_attr,data_num,data_crc,f_head,f_end;
unsigned long data_rec,data_send;
int buff_num=0,ask_num=0,sockfd,nbytes,nNetTimeout=1;
pthread_t save_msg_pthread,send_msg_pthread,rec_ask_pthread,locate_process_pthread,info_from_service_and_send_to_uart_pthread;
struct sockaddr_in server_addr; 
struct hostent *host; //  host  涓绘満锛?
unsigned int CRC_TAB[256] =
{
    0xF078,0xE1F1,0xD36A,0xC2E3,0xB65C,0xA7D5,0x954E,0x84C7,
    0x7C30,0x6DB9,0x5F22,0x4EAB,0x3A14,0x2B9D,0x1906,0x088F,
    0xE0F9,0xF170,0xC3EB,0xD262,0xA6DD,0xB754,0x85CF,0x9446,
    0x6CB1,0x7D38,0x4FA3,0x5E2A,0x2A95,0x3B1C,0x0987,0x180E,
    0xD17A,0xC0F3,0xF268,0xE3E1,0x975E,0x86D7,0xB44C,0xA5C5,
    0x5D32,0x4CBB,0x7E20,0x6FA9,0x1B16,0x0A9F,0x3804,0x298D,
    0xC1FB,0xD072,0xE2E9,0xF360,0x87DF,0x9656,0xA4CD,0xB544,
    0x4DB3,0x5C3A,0x6EA1,0x7F28,0x0B97,0x1A1E,0x2885,0x390C,
    0xB27C,0xA3F5,0x916E,0x80E7,0xF458,0xE5D1,0xD74A,0xC6C3,
    0x3E34,0x2FBD,0x1D26,0x0CAF,0x7810,0x6999,0x5B02,0x4A8B,
    0xA2FD,0xB374,0x81EF,0x9066,0xE4D9,0xF550,0xC7CB,0xD642,
    0x2EB5,0x3F3C,0x0DA7,0x1C2E,0x6891,0x7918,0x4B83,0x5A0A,
    0x937E,0x82F7,0xB06C,0xA1E5,0xD55A,0xC4D3,0xF648,0xE7C1,
    0x1F36,0x0EBF,0x3C24,0x2DAD,0x5912,0x489B,0x7A00,0x6B89,
    0x83FF,0x9276,0xA0ED,0xB164,0xC5DB,0xD452,0xE6C9,0xF740,
    0x0FB7,0x1E3E,0x2CA5,0x3D2C,0x4993,0x581A,0x6A81,0x7B08,
    0x7470,0x65F9,0x5762,0x46EB,0x3254,0x23DD,0x1146,0x00CF,
    0xF838,0xE9B1,0xDB2A,0xCAA3,0xBE1C,0xAF95,0x9D0E,0x8C87,
    0x64F1,0x7578,0x47E3,0x566A,0x22D5,0x335C,0x01C7,0x104E,
    0xE8B9,0xF930,0xCBAB,0xDA22,0xAE9D,0xBF14,0x8D8F,0x9C06,
    0x5572,0x44FB,0x7660,0x67E9,0x1356,0x02DF,0x3044,0x21CD,
    0xD93A,0xC8B3,0xFA28,0xEBA1,0x9F1E,0x8E97,0xBC0C,0xAD85,
    0x45F3,0x547A,0x66E1,0x7768,0x03D7,0x125E,0x20C5,0x314C,
    0xC9BB,0xD832,0xEAA9,0xFB20,0x8F9F,0x9E16,0xAC8D,0xBD04,
    0x3674,0x27FD,0x1566,0x04EF,0x7050,0x61D9,0x5342,0x42CB,
    0xBA3C,0xABB5,0x992E,0x88A7,0xFC18,0xED91,0xDF0A,0xCE83,
    0x26F5,0x377C,0x05E7,0x146E,0x60D1,0x7158,0x43C3,0x524A,
    0xAABD,0xBB34,0x89AF,0x9826,0xEC99,0xFD10,0xCF8B,0xDE02,
    0x1776,0x06FF,0x3464,0x25ED,0x5152,0x40DB,0x7240,0x63C9,
    0x9B3E,0x8AB7,0xB82C,0xA9A5,0xDD1A,0xCC93,0xFE08,0xEF81,
    0x07F7,0x167E,0x24E5,0x356C,0x41D3,0x505A,0x62C1,0x7348,
    0x8BBF,0x9A36,0xA8AD,0xB924,0xCD9B,0xDC12,0xEE89,0xFF00
};

const unsigned char s_crc8_table[256] = {
	0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
	0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
	0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
	0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
	0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
	0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
	0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
	0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
	0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
	0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
	0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
	0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
	0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
	0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
	0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
	0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35,
};
FILE *fop_log;/*日志文件*/
FILE *send_fail_fop;
char log_name[30];/*日志文件名*/
char dir_log_name[40];
int portnumber;

/*ini相关的结构体*/
typedef struct item_t {
    char *key;
    char *value;
}ITEM;
char *strtrimr(char *pstr);//情字符串右空格
char *strtriml(char *pstr);//清字符串左空格
char *strtrim(char *pstr);//清字符串两边空格
int  get_item_from_line(char *line,  ITEM *item);//读行
int read_conf_value(const char *key, char *value,const char *file);//读文件
char value_port[10];/*存放读回来的值*/
char value_ipadd[30];/*存放读回来的值*/
char value_device_id[20];/*存放读回来的值*/
char real_ip[15];
unsigned long real_device_id;

char floor_buff[5];
int floor_value = 0;
/*循环队列*/
typedef struct
{
    char bitch[MAXSIZE][200];
    int font;
    int rear;
    int length[MAXSIZE]; 
}SqQueue;
/*初始化一个空队列*/
unsigned int InitQueue(SqQueue *Q)
{
    Q->font = 0;
    Q->rear = 0;
    return 0;
}

int QueueLength(SqQueue Q)
{
    return (Q.rear - Q.font)%MAXSIZE;
}

unsigned int ENQueue(SqQueue *Q,char *buffer,int len)
{
    if((Q->rear+1)%MAXSIZE == Q->font)
        return -1;

    memcpy(Q->bitch[Q->rear],buffer,len);
    Q->length[Q->rear] = len;
    Q->rear = (Q->rear+1)%MAXSIZE;
    return 0;
}

unsigned int DeQueue(SqQueue *Q,char *buffer,int len)
{
    if(Q->font == Q->rear)
        return -1;
	len = Q->length[Q->font];
        memcpy(buffer,Q->bitch[Q->font],len);
        Q->font = (Q->font+1)%MAXSIZE;
        return len;

}
SqQueue *MyQueue ;

sqlite3 *db = NULL;  
char *pErrMsg = 0;  
int main(int argc, char *argv[]) 
{ 
	printf("**********main*********************\n");
	time_t timep;
	char convet_buff[100];
	FILE* fd;
	int ret_read_conf;	
	//分配内存
	MyQueue = (SqQueue *)malloc(sizeof(SqQueue));
	
	struct timeval tv,time_out;
	struct tm *p;
	long time_second;

	time_out.tv_sec=1;
	time_out.tv_usec=0;
     /*程序一运行就要读配置文件*/
	/*portnum*/
	ret_read_conf = read_conf_value("portnum", value_port,"/root/config.ini");
	if(ret_read_conf)
	{
		send_fail_fop = openfile("/root/send_failed.log","a+");
		writeFile(send_fail_fop, "read config.ini error", 6);
		closeFile(send_fail_fop);
		
	}
	portnumber = atoi(value_port);
	printf("portnum = %d \n",portnumber);
	/*ipaddress*/
	ret_read_conf = read_conf_value("ipadd", value_ipadd,"/root/config.ini");
	if(ret_read_conf)
	{
		send_fail_fop = openfile("/root/send_failed.log","a+");
		writeFile(send_fail_fop, "read config.ini error", 6);
		closeFile(send_fail_fop);	
	}
	printf("ipadd = %s \n",value_ipadd);
	/*deviceID*/
	ret_read_conf = read_conf_value("device_id", value_device_id,"/root/config.ini");
	if(ret_read_conf)
	{
		send_fail_fop = openfile("/root/send_failed.log","a+");
		writeFile(send_fail_fop, "read config.ini error device_id", 6);
		closeFile(send_fail_fop);	
	}
	printf("device_id = %s ",value_device_id);
	//printf("device_id len is = %d \n",strlen(value_device_id));
	real_device_id = convert_atohex(value_device_id);
	printf("real_device_id = %x \n",real_device_id);
	/*DEBUG*/
	ret_read_conf = read_conf_value("DEBUG", debug_buf,"/root/config.ini");
	if(ret_read_conf)
	{
		send_fail_fop = openfile("/root/send_failed.log","a+");
		writeFile(send_fail_fop, "read config.ini error", 6);
		closeFile(send_fail_fop);	
	}
	DEBUG= atoi(debug_buf);
	printf("DEBUG = %d \n",DEBUG);
	/*floor*/
	ret_read_conf = read_conf_value("floor_flag", floor_buff,"/root/config.ini");
	if(ret_read_conf)
	{
		send_fail_fop = openfile("/root/send_failed.log","a+");
		writeFile(send_fail_fop, "read config.ini error", 6);
		closeFile(send_fail_fop);	
	}
	floor_value= atoi(floor_buff);
	printf("floor_value = %d \n",floor_value);
	/*配置文件中ip要顶格写*/
	if((host=gethostbyname(value_ipadd))==NULL)  
	{ 
		fprintf(stderr,"Gethostname error\n"); 
		exit(1); 
	} 
	/*创建套接字连接*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)   /*SOCK_STREAM  tcp*/
	{ 
		fprintf(stderr,"Socket Error:%s\a\n",strerror(errno)); 
		exit(1); 
	}  
	bzero(&server_addr,sizeof(server_addr)); 
	server_addr.sin_family=AF_INET;          // IPV4
	server_addr.sin_port=htons(portnumber); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr); 

	if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
	{ 
			fprintf(stderr,"Connect Error:%s\a\n",strerror(errno)); 
			exit(1); 
	} 
	/*串口连接*/
	uart_fd = open_uart();
	init_uart();
	printf("init is OK\r\n");
	if(InitQueue(MyQueue))
	{
		printf("InitQueue error \n");
	}
	buff[0]=0xf2;
	buff[1]=0x11;
	buff[2]=0x12;
	buff[3]=0x34;
	buff[4]=0x56;
	buff[5]=(unsigned char)(real_device_id>>24);   /*发送方id开始*/
	buff[6]=(unsigned char)(real_device_id>>16);
	buff[7]=(unsigned char)(real_device_id>>8);;
	buff[8]=(unsigned char)(real_device_id);
	buff[12] = 0x11;
	buff[13] = 0x11;
	buff[28]  = 0xf1;//f1时间请求帧
	printf("发送时间请求 \n");
	if((nbytes=write(sockfd,buff,29))==-1)
	{
		printf("Write Error!\n");
		exit(1);	
    }
	/*这里要把写的东西写到日志文件里面*/
	getTime(log_name, 3);
	sprintf(dir_log_name,"/root/%s.log",log_name);
	fop_log = openfile(dir_log_name,"a+");
	memset(convet_buff,0,100);
	convet(buff,convet_buff,29);
	writeFile(fop_log, convet_buff, 1);
	if(-1 == (nbytes=read(sockfd,buff,33)))  
 	{
   		printf("read data fail !\r\n");  
    	exit(1);  
	}  

	/*接收到服务器的信息开始时间校验*/
	if(buff[13]==0x12)    /*校验时间算法还有问题*/
	{
		time_second=(long)buff[25]+((long)buff[24]<<8)+((long)buff[23]<<16)+((long)buff[22]<<24);/*28800 = 8*3600区时8小时*/
		printf("second  =%d\n",time_second);
		tv.tv_sec=time_second;
		tv.tv_usec=0;
		settimeofday(&tv,NULL);
		memset(convet_buff,0,100);
		convet(buff,convet_buff,33);
		writeFile(fop_log, convet_buff, 2);
		closeFile(fop_log);
		/*时间校准之后重新打开文件如果是新的一天将会创建新的文档*/
		getTime(log_name, 3);
		sprintf(dir_log_name,"/root/%s.log",log_name);
		fop_log = openfile(dir_log_name,"a+");	
	}
	/*开始创建线程*/
	int pthread_Err = pthread_create(&save_msg_pthread,NULL,save_msg,NULL);
	if (pthread_Err != 0)
	{
	printf("Create thread Failed!\n");
	return EXIT_FAILURE;
	}
	pthread_Err = pthread_create(&send_msg_pthread,NULL,send_msg,NULL);
	if (pthread_Err != 0)
	{
	printf("Create thread Failed!\n");
	return EXIT_FAILURE;
	}
	pthread_Err = pthread_create(&rec_ask_pthread,NULL,rec_ask,NULL);
	if (pthread_Err != 0)
	{
	printf("Create thread Failed!\n");
	return EXIT_FAILURE;
	}

	pthread_Err = pthread_create(&info_from_service_and_send_to_uart_pthread,NULL,info_from_service_and_send_to_uart,NULL);
	if (pthread_Err != 0)
	{
	printf("Create thread Failed!\n");
	return EXIT_FAILURE;
	}



	
	int err=pthread_join(save_msg_pthread,NULL);/*阻塞等待线程退出*/
    	if(err!=0)  
  	{  
        	printf("can not join with thread1:%s\n",strerror(err));  
        	exit(1);  
   	}
	err=pthread_join(send_msg_pthread,NULL);
    	if(err!=0)  
  	{  
        	printf("can not join with thread2:%s\n",strerror(err));  
        	exit(1);  
   	}
	err=pthread_join(rec_ask_pthread,NULL);
    	if(err!=0)  
  	{  
        	printf("can not join with thread3:%s\n",strerror(err));  
        	exit(1);  
   	}

	err=pthread_join(info_from_service_and_send_to_uart_pthread,NULL);
    	if(err!=0)  
  	{  
        	printf("can not join with thread3:%s\n",strerror(err));  
        	exit(1);  
   	}
    	 
		close(sockfd);
		exit(0); 
} 



void *save_msg()
{
	printf("enter save_msg********************************************************\n");
	unsigned char store_save[250];
	unsigned char temp;
	jinchuzhan_beat= 0;
	time_t timep;
	data_num=0x00;/*流水号初始化*/
	while(1)
	{	
		/*这里应该加文件锁***********/		
		memset(store_save,0,250);
		memset(temp_buff,0,1024);
		data_length= nread(uart_fd,temp_buff);/*返回读回来的字节数这就是数据长度*/
		time(&timep);
		//printf("save_msg datalength = %02x \n",data_length);
		switch(temp_buff[0])   /*度数据内容的第一个字节*/
		{ 
			case 0x01:    /*OBU*/
				f_head=0xf2;/*帧头*/
				data_attr=0xd0;/*属性*/
				data_num++;/*流水号*/
				dev_id = real_device_id;/*设备id*/
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/				
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				
				break;
				
			case 0x04:		/*RSSI汇总帧*/
				f_head=0xf2;
				data_attr=0xd2;
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/						
				f_end=0xf1;		
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;

			case 0x02:/*司机资格证1*/
				f_head=0xf2;
				data_attr=0xd1;/*属性*/
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/				
				f_end=0xf1;			
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			case 0x05:/*司机资格证2*/
				f_head=0xf2;
				data_attr=0xd5;/*属性自己定义的*/
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/
				f_end=0xf1;				
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			case 0x06:/* 路由心跳*/
				f_head=0xf2;
				data_attr=0xd6;
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;

			case 0xec:  //远程查询应答帧
				printf("远程查询应答帧收到****\n");
				f_head=0xf2;
				data_attr=0xaa12;//unknown
				data_num = 43;
				dev_id = real_device_id;	
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			default:
				f_head=0xf2;
				data_attr=0xaa12;//unknown
				data_num = 0x38;
				dev_id = real_device_id;	
				data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
					
		}

			
		/*封装头*/
		store_save[0]=*((unsigned char *)&f_head);
		store_save[1]=*((unsigned char *)&data_attr+1);/*帧类型，先写高字节*/
		store_save[2]=*((unsigned char *)&data_attr);
		store_save[3]=*((unsigned char *)&data_num+1);
		store_save[4]=*((unsigned char *)&data_num);
		store_save[5]=*((unsigned char *)&dev_id+3);
		store_save[6]=*((unsigned char *)&dev_id+2);
		store_save[7]=*((unsigned char *)&dev_id+1);
		store_save[8]=*((unsigned char *)&dev_id);
		store_save[9]=*((unsigned char *)&data_rec+3);
		store_save[10]=*((unsigned char *)&data_rec+2);
		store_save[11]=*((unsigned char *)&data_rec+1);
		store_save[12]=*((unsigned char *)&data_rec);
		memcpy(store_save+13,temp_buff,data_length);
		
		/*开始写入时间四个字节*/
		store_save[data_length+13]=*((unsigned char *)&timep+3);
		store_save[data_length+14]=*((unsigned char *)&timep+2);
		store_save[data_length+15]=*((unsigned char *)&timep+1);
		store_save[data_length+16]=*((unsigned char *)&timep);
		
		/*校验码2个字节通过刚刚发过来的数据及长度进行校验*/	
		data_crc=crc16(store_save,17+data_length);
		store_save[data_length+17]=*((unsigned char *)&data_crc+1);
		store_save[data_length+18]=*((unsigned char *)&data_crc);
		store_save[data_length+19]=*((unsigned char *)&f_end);
		
		/*data_length是读回来的字节数*/
		if(data_length>0)
		{
			/*装进队列*/
			ENQueue(MyQueue,store_save,data_length+20);			
		}		
	}	
}
			
void *send_msg()
{
	printf("enter send_msg  ***********\n");
	char j;
	time_t timep;
	struct tm *p;
	char out[40];
	char rmlog[30];
	first = time(NULL);
	unsigned char buffer_send_rsu[200]; /*够用*/
	int buffer_send_rsu_len = 0;/*出队列的长度*/
	unsigned char error_buffer[200];
	while(1)
	{
		/*日志删除部分*/
		if(difftime(time(NULL),first) >= 3600)
		{  
		/*会保存一个小时的数据如果没有数据他也不会频繁的创建新的文件*/
		closeFile(fop_log);			
		getTime(log_name, 3);
		sprintf(dir_log_name,"/root/%s.log",log_name);
		fop_log = openfile(dir_log_name,"a+");
		first = time(NULL);
		}

		while(MyQueue->font !=MyQueue->rear)
		{
			memset(buffer_send_rsu,0,200);
			buffer_send_rsu_len = DeQueue(MyQueue,buffer_send_rsu,buffer_send_rsu_len);
			signal(SIGPIPE,SIG_IGN);			
			if((nbytes=write(sockfd,buffer_send_rsu,buffer_send_rsu_len))==-1)
			{	
				/*五次重写*/
				for(j=0;j<2;j++)                 
				{	
					signal(SIGPIPE,SIG_IGN);
					if((nbytes=write(sockfd,buffer_send_rsu,buffer_send_rsu_len))!=-1)
						break;
				}
				printf("send error!!  ");
				/*写到日志*//*单独写到一个文件里面比较好*/
				send_fail_fop = openfile("/root/send_failed.log","a+");
				convet(buffer_send_rsu, error_buffer,buffer_send_rsu_len);
				writeFile(send_fail_fop, error_buffer, 5);
				closeFile(send_fail_fop);
			}			
			printf("send nbytes = %d\n",nbytes);
		}
	}	
}


void *rec_ask()
{
	printf("enter rec_ask  ***********\n");
	unsigned char RSU_heart_beat[50];
	unsigned int RSU_serial_num = 0;
	heartbeat_t = time(NULL);
	int flag = 0;
	int val;
	unsigned char biaozhi_arm = 0x10;/*arm heartbeat biaozhi'*/
	struct timeval tm;
	fd_set set;
	RSU_heart_beat[0] = 0xf2;
	RSU_heart_beat[1] = 0x00;/*数据属性*/
	RSU_heart_beat[2] = 0x10;
	while(1)
	{
		/*心跳时间30分钟*/
		if(difftime(time(NULL),heartbeat_t) >= 1800)   
		{
			heartbeat_t  = time(NULL);
			RSU_serial_num++;
			if(RSU_serial_num ==65535)
			RSU_serial_num = 0x01;
			RSU_heart_beat[3]=*((unsigned char *)&RSU_serial_num+1);
			RSU_heart_beat[4]=*((unsigned char *)&RSU_serial_num);
			RSU_heart_beat[5]=*((unsigned char *)&real_device_id+3);
			RSU_heart_beat[6]=*((unsigned char *)&real_device_id+2);
			RSU_heart_beat[7]=*((unsigned char *)&real_device_id+1);
			RSU_heart_beat[8]=*((unsigned char *)&real_device_id);
			RSU_heart_beat[9]=*((unsigned char *)&data_rec+3);
			RSU_heart_beat[10]=*((unsigned char *)&data_rec+2);
			RSU_heart_beat[11]=*((unsigned char *)&data_rec+1);
			RSU_heart_beat[12]=*((unsigned char *)&data_rec);
			RSU_heart_beat[13]=*((unsigned char *)&biaozhi_arm);
			RSU_heart_beat[14]=*((unsigned char *)&real_device_id+3);/*for 8 bytes send id twice*/
			RSU_heart_beat[15]=*((unsigned char *)&real_device_id+2);
			RSU_heart_beat[16]=*((unsigned char *)&real_device_id+1);
			RSU_heart_beat[17]=*((unsigned char *)&real_device_id);
			RSU_heart_beat[18]=*((unsigned char *)&real_device_id+3);
			RSU_heart_beat[19]=*((unsigned char *)&real_device_id+2);
			RSU_heart_beat[20]=*((unsigned char *)&real_device_id+1);
			RSU_heart_beat[21]=*((unsigned char *)&real_device_id);
			/*开始写入时间四个字节*/
			RSU_heart_beat[22]=*((unsigned char *)&heartbeat_t+3);
			RSU_heart_beat[23]=*((unsigned char *)&heartbeat_t+2);
			RSU_heart_beat[24]=*((unsigned char *)&heartbeat_t+1);
			RSU_heart_beat[25]=*((unsigned char *)&heartbeat_t);
			RSU_heart_beat[26]=0x00;/*check code 0*/
			RSU_heart_beat[27]=0x00;
			RSU_heart_beat[28]=0xf1;		
			signal(SIGPIPE,SIG_IGN);/*导致程序退出的信号SIGPIPE*/
			if((nbytes=write(sockfd,RSU_heart_beat,29))!=29)//=!29  error occured
			{
				/*写入日志*/
				printf("write  is wrong ********************************************\n");
				close(sockfd);
				sockfd=socket(AF_INET,SOCK_STREAM,0) ;  /*SOCK_STREAM  tcp*/		
				bzero(&server_addr,sizeof(server_addr)); 
				server_addr.sin_family=AF_INET;          // IPV4
				server_addr.sin_port=htons(portnumber); 
				server_addr.sin_addr=*((struct in_addr *)host->h_addr); 					
				val =fcntl(sockfd,F_GETFL,0); /*获取标志位*/
				fcntl(sockfd,F_SETFL,val | O_NONBLOCK); /*改变标志位添加非阻塞属性*/
				int connect_flag;
				connect_flag = connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
				sleep(1);
				printf("lianjie fuzhang \n");
				FD_ZERO(&set); /*将set清零使集合中不含任何fd*/
				FD_SET(sockfd,&set);/*将sockfd加入set集合*/
				tm.tv_sec = TIME_OUT_TIME;
				tm.tv_usec = 0;
				flag = select(sockfd+1,NULL,NULL,NULL,&tm);
				printf("flag is %d \n",flag);
				if(-1 == flag)
				{
					printf("select error \n");
				}
				if(0 == flag)
				{
					printf("time out \n");
				}				
				sleep(3);			
				fcntl(sockfd,F_SETFL,val & (~O_NONBLOCK));				
			}			
		}
		usleep(100);		  
	}
}

void *info_from_service_and_send_to_uart()
{
	printf("**********info_from_service_and send to uart*********************\n");
	fd_set read_fd;
	struct timeval tm;
	int data_len;//服务器下发的数据长度
	int flag = 0;
	unsigned char sprintf_time_out[40];
	unsigned char buff_recv_service[200];
	unsigned char buff_send_uart[200];
	int nread;
	unsigned int serial_num_service = 1;	
	while(1)
	{
		//printf("info_from_service_and_send_to_uart \n");
		FD_ZERO(&read_fd);
		FD_SET(sockfd,&read_fd);	
		tm.tv_sec = 3;
		tm.tv_usec = 0;		
		flag = select(sockfd+1,&read_fd,NULL,NULL,&tm);
		switch(flag)
		{
			case 0:
				printf("wait service time out ");
				break;
			case -1:
				printf("select error occoured \n");
				break;
			default:     //select 返回会将未准备好的描述符清掉
				printf("enter default \n");
				if(FD_ISSET(sockfd,&read_fd))
				{
					ioctl(sockfd,FIONREAD,&nread);//测试缓冲区里面有多少个字节可以被读取，然后把字节数存放在nread里面
					if(nread==0)
					{
					break;
					}
					printf("nread service can be readed = %d \n",nread);
					memset(buff_recv_service,0,200);
					nread = read(sockfd,buff_recv_service,nread);
					buff_recv_service[nread] = 0;
					printf("read from service is :\n");
					for(k = 0;k<nread;k++)
					{
					printf(" %02x",buff_recv_service[k]);
					}
							
					switch(buff_recv_service[13]){
						case 0x13://RSU状态请求帧
							data_len = 8;//RSU_ID
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 12;//发给串口的长度
							buff_send_uart[2] = 0x13;					
							memcpy(buff_send_uart+3,buff_recv_service+14,8);
							buff_send_uart[11] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,12)) == -1)
							{
								printf("write error \n");
							}
							break;
				
						case 0x14://RSU信息设置帧
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 16;
							buff_send_uart[2] = 0x14;				
							memcpy(buff_send_uart+3,buff_recv_service+14,12);
							buff_send_uart[15] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,16)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<16;k++)
							{
							printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						
						case 0xec://远程查询帧
							printf("服务器远程查询\n");
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 12;
							buff_send_uart[2] = 0xec;							
							memcpy(buff_send_uart+3,buff_recv_service+14,8);
							buff_send_uart[11] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,12)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<12;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;

						case 0xe1:   //PANID设置帧
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 14;
							buff_send_uart[2] = 0xe1;
							memcpy(buff_send_uart+3,buff_recv_service+14,10);
							buff_send_uart[13] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,14)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<14;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						
						case 0xe2:   //信道设置帧
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 13;
							buff_send_uart[2] = 0xe2;
							memcpy(buff_send_uart+3,buff_recv_service+14,9);
							buff_send_uart[12] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,13)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<13;k++)
							{
							printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						case 0xe3:   //发送功率设置帧
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 13;
							buff_send_uart[2] = 0xe3;
							memcpy(buff_send_uart+3,buff_recv_service+14,9);
							buff_send_uart[12] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,13)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<13;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;

						case 0xe4:   //路由心跳时间间隔设置帧
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 14;
							buff_send_uart[2] = 0xe4;
							memcpy(buff_send_uart+3,buff_recv_service+14,10);
							buff_send_uart[13] = 0x00;//校验
							if((nbytes = write(uart_fd,buff_send_uart,14)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<14;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						
						default:
							break;					
					}
				}//end if FD_ISSET
				break;
		}
	usleep(100);
	}
}


/*从串口读固定长度的信息*/
ssize_t xread(int fd,void *ptr,size_t n) 
{
	size_t nleft;
	ssize_t n_read;
	nleft=n;
	while(nleft>0)
	{
		if((n_read=read(fd,ptr,nleft))<0)
		{
			printf("x_read   limian =%02x \n",n_read);
			if(nleft==n)
				return(-1);
			else
				break;
		}
		else if(n_read==0)
		{
			printf("xread n_read = = 0");
			break;		
		}
		nleft-=n_read;
		ptr+=n_read;
	}
	return (n-nleft);/*跳出循环nleft等于零*/
}


unsigned char checkcode_crc8(const unsigned char *ptr,int len)//CRC校验
{
  unsigned char crc  = 0x00;
  crc = s_crc8_table[0x7e];
  crc = s_crc8_table[(len+3)^ crc];
  while (len--)
  {
		crc = s_crc8_table[(*ptr++)^ crc];
  }
  return crc;
}


/*验证7e 和然后发送数据最后校验*/
ssize_t nread(int fd,unsigned char *ptr)
{
	int ret_sql = 0;
	ssize_t len;/*读回来的长度*/
	ssize_t datalen;
	ssize_t p;/*定义一个变量存放读到的数据*/
	//unsigned char virtral_data[48]={0x7e,0x30,0x04,0x01,0x02,0x61,0x02,0x00,0x00,0x03,0x81,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x11,0xbe,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x16,0xac,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x12,0xa5,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char data_from_uart[100];
	unsigned char uart_convet[200];
	unsigned char crc_8;
	unsigned char k;
	unsigned char t;
	
	p=0x00;
	char rssi[300];
	char obu_info[300];
	char driver[200];
	char obu_id[10];
	char obu_id_convert[20];
	/*sql*/
	char obu_id_sql[16];/*20150707*/
	//char route1_id[20];
	//char route2_id[20];
	//char route3_id[20];
	//char route4_id[20];
	int route1_id = 0;
	int route2_id = 0;
	int route3_id = 0;
	int route4_id = 0;
	int route1_val = 0;
	int route2_val = 0;
	int route3_val = 0;
	int route4_val = 0;
	char time_sql[30]= {0};
	while(p!=0x7e)
	{
		read(fd,&p,1);
	}
	read(fd,&p,1);
	
	datalen = p -2;/*data_len*/

	if((len = xread(fd,ptr,datalen))<0)  /*xread 返回零的问题 需要解决*/
	{
		printf("read error\n");
	}
	if(ptr[0] == 0x01)
	{
		//OBU信息帧 写入日志
		unsigned char obu_buff[8]={0};
		memset(data_from_uart,0,100);
		memset(uart_convet,0,200);
		data_from_uart[0] = 0x7e;
		data_from_uart[1] = p;
		memcpy(obu_buff,ptr+1,8);
		memcpy(data_from_uart+2,ptr,len);/*包含校验位*/
		convet(data_from_uart, uart_convet, p);/*uart_convet里面数据是ascii*/
		//转换成ascii码后，一位变成两位
		memcpy(obu_info,"OBU:",4);
		memcpy(obu_info+4,uart_convet+6,16);				
		memcpy(obu_info+20,";CardID:",8);
		memcpy(obu_info+28,uart_convet+22,16);
		memcpy(obu_info+44,";Other info:",12);
		memcpy(obu_info+56,uart_convet+38,8);
		writeFile(fop_log, obu_info, 15);
		printf("读到obu信息帧:%s\n",obu_info);
		
		/***如果是安检位置，则发送安检位置给服务器***/
		char anjian_obu[17];
		memcpy(anjian_obu,uart_convet+6,16);
		//熄火情况下obu前缀是01，安检时应该是熄火的
		if(floor_value==9&&anjian_obu[1]==0x31)
		{
			time_t timep;
			time(&timep);
			int anjian_pos=0;
			anjian_pos=002;			
			writeFile(fop_log, anjian_obu,14);
			writeFile(fop_log, NULL,16);
			/*队列*/
			f_head=0xf2;/*帧头*/
			data_attr=0x8888;/*属性*/
			data_num++;/*流水号*/
			dev_id = real_device_id;/*设备id*/
			data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/				
			f_end=0xf1;
			if(data_num==65535)
			{
				data_num=0x01;
			}
			/*封装头*/
			unsigned char pos_buff[50];
			memset(pos_buff,0,50);
			pos_buff[0]=*((unsigned char *)&f_head);
			pos_buff[1]=*((unsigned char *)&data_attr+1);/*帧类型，先写高字节*/
			pos_buff[2]=*((unsigned char *)&data_attr);
			pos_buff[3]=*((unsigned char *)&data_num+1);
			pos_buff[4]=*((unsigned char *)&data_num);
			pos_buff[5]=*((unsigned char *)&dev_id+3);
			pos_buff[6]=*((unsigned char *)&dev_id+2);
			pos_buff[7]=*((unsigned char *)&dev_id+1);
			pos_buff[8]=*((unsigned char *)&dev_id);
			pos_buff[9]=*((unsigned char *)&data_rec+3);
			pos_buff[10]=*((unsigned char *)&data_rec+2);
			pos_buff[11]=*((unsigned char *)&data_rec+1);
			pos_buff[12]=*((unsigned char *)&data_rec);
			pos_buff[13]=0x20;
			memcpy(pos_buff+14,obu_buff,8);
			pos_buff[22]=0x00;
			pos_buff[23]=0x00;
			pos_buff[24]=0x00;
			pos_buff[25]=0x02;			
			/*开始写入时间四个字节*/
			pos_buff[26]=*((unsigned char *)&timep+3);
			pos_buff[27]=*((unsigned char *)&timep+2);
			pos_buff[28]=*((unsigned char *)&timep+1);
			pos_buff[29]=*((unsigned char *)&timep);
			
			/*校验码2个字节通过刚刚发过来的数据及长度进行校验*/	
			data_crc=crc16(pos_buff,30);
			pos_buff[30]=*((unsigned char *)&data_crc+1);
			pos_buff[31]=*((unsigned char *)&data_crc);
			pos_buff[32]=*((unsigned char *)&f_end);
			
			/*装进队列*/
			ENQueue(MyQueue,pos_buff,33);
			int key=0;
			for(key=0;key<33;key++)
				printf("%x ",pos_buff[key]);
		}
	}
	/*构造打印数组*/      /*打印rssi信息*/
	if(ptr[0] == 0x04)
	{
		//writeFile(fop_log, NULL,7);
		int n;
		printf("读到rssi 信息!\n");		
		printf("**************************************rssi=");
		for(n=0;n<46;n++)
		{
		printf("%x",ptr[n]);
		}
		printf("\n");
		memset(data_from_uart,0,100);
		memset(uart_convet,0,200);
		data_from_uart[0] = 0x7e;
		data_from_uart[1] = p;
		memcpy(data_from_uart+2,ptr,len);/*包含校验位*/
		route1_val = data_from_uart[19];/*sql*/
		route2_val = data_from_uart[28];/*sql*/
		route3_val = data_from_uart[37];/*sql*/
		route4_val = data_from_uart[46];/*sql*/
		
		route1_id = data_from_uart[18];/*sql*/
		route2_id = data_from_uart[27];/*sql*/
		route3_id = data_from_uart[36];/*sql*/
		route4_id = data_from_uart[45];/*sql*/

		convet(data_from_uart, uart_convet, p);/*uart_convet里面数据是ascii*/
		//转换成ascii码后，一位变成两位
		memset(obu_id_sql,0,16);
		memcpy(obu_id_sql,uart_convet+6,16);/*sql*/

		
		memcpy(rssi,"OBU:",4);
		memcpy(rssi+4,uart_convet+6,16);
		
		memcpy(rssi+20,";rssi1:",7);
		memcpy(rssi+27,uart_convet+22,16);

		memcpy(rssi+43,";qd:",4);
		memcpy(rssi+47,uart_convet+38,2);
		memcpy(rssi+49,";rssi2:",7);
		memcpy(rssi+56,uart_convet+40,16);
		
		memcpy(rssi+72,";qd:",4);
		memcpy(rssi+76,uart_convet+56,2);
		memcpy(rssi+78,";rssi3:",7);
		memcpy(rssi+85,uart_convet+58,16);
		
		memcpy(rssi+101,";qd:",4);
		memcpy(rssi+105,uart_convet+74,2);
		memcpy(rssi+107,";rssi4:",7);
		memcpy(rssi+114,uart_convet+76,16);
		
		memcpy(rssi+130,";qd:",4);
		memcpy(rssi+134,uart_convet+92,2);
		memcpy(rssi+136,"\n\0",2);
		writeFile(fop_log, rssi, 8);

	}/*end of if*/
	return len-1;/*len-1是真正的数据长度 减去校验位*/
}

unsigned int crc16(unsigned char *buf, unsigned int len)
{
    unsigned int val = 0,i;
    for (i=0; i<len; i++) 
	{val=CRC_TAB[(val^=buf[i])&0xFF]^(val>>8);}
    return val;
}


unsigned int crc16_serial(unsigned char *buf, unsigned int len)
{
    unsigned int val = 0,i;
    val = CRC_TAB[0x7e];
    val = CRC_TAB[(unsigned char)(len+3)] ^(val>>8);
    for (i=0; i<len; i++)
	{val=CRC_TAB[(val^=buf[i])&0xFF]^(val>>8);}
    return val;
}



int open_uart(void)
{
	int fd;
	if(!DEBUG){
	printf("open usb /tts/5 \n");
	if((fd = open("/dev/usb/tts/5",O_RDWR | O_NOCTTY)) == -1)
	{
		perror("open uart");
	}
	else
	{
		printf("open_uart success\n");
	}
}
else	
{
	printf("open /dev/ttyAMA1 \n");
	if((fd = open("/dev/ttyAMA1",O_RDWR | O_NOCTTY)) == -1)
	{
		perror("open uart");
	}
	else
	{
		printf("open_uart success\n");
	}
}
	
	return (fd);
}
//--------------------------------------------------------------------------------

//---------------------------------------------------------------------------------

void init_uart()
{
	//printf("1\n");
	struct termios newtio,oldtio;
	if(fcntl(uart_fd, F_SETFL,0) < 0)
	{
		perror("fcntl uart ");
	}
	//printf("2\n");
	if(isatty(STDIN_FILENO) == 0)
	{
		printf("standard input is not a terminal device\n");
	}
	//printf("3\n");
	if(tcgetattr(uart_fd, &oldtio) != 0)
	{
		perror("tcgetattr ");
	}
	//printf("4\n");
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;//first clear the CSIZE flag and then set the CSIZE
	newtio.c_cflag |= CS8;//8 bites
	newtio.c_cflag &= ~PARENB;// no parity check;
	newtio.c_cflag &= ~CSTOPB;//one stop btie
	newtio.c_lflag &= ~ICANON;// informal mode
	if(cfsetispeed(&newtio, B115200) != 0)
	{
		perror("cfsetispeed");
	}
	//printf("5\n");
	

	newtio.c_cc[VTIME] = 1;/*指定读取第一个字符的等待时间*/
	newtio.c_cc[VMIN] = 0;/*指定所要读取字符的最小长度*/
	tcflush(uart_fd,TCIFLUSH);
	if(tcsetattr(uart_fd, TCSANOW, &newtio) != 0)
	{
		perror("tcsetaddr uart ");
	}	
	//printf("success!\n");

}



/*有关日志功能的子函数*/
/*获取当前系统的时间 */
int getTime_bark(char *out,int fmt)
{
    if(out == NULL)
        return -1;
    time_t t;
    struct tm *tp;
   // t = time(NULL);
    time(&t);
    tp = localtime(&t);
    if(fmt == 0)
        sprintf(out, "%2.2d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    else if(fmt == 1)
        sprintf(out, "%2.2d-%2.2d-%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday);
    else if(fmt == 2)
        sprintf(out, "%2.2d:%2.2d:%2.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
    else if(fmt ==3 )
		   sprintf(out, "%2.2d%2.2d%2.2d_%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour);
    else if(fmt ==4 )
		   sprintf(out, "%2.2d%2.2d%2.2d_%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday-2, tp->tm_hour);
	
    return 0;

}

int getTime(char *out,int fmt)
{
    if(out == NULL)
        return -1;
    time_t t;
    struct tm *tp;
 
    time(&t);
    tp = localtime(&t);
    if(fmt == 0){
        sprintf(out, "%2.2d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
		return 0;
    	}

	else if(fmt == 1){
        sprintf(out, "%2.2d-%2.2d-%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday);
		return 0;
		}
    else if(fmt == 2){
        sprintf(out, "%2.2d:%2.2d:%2.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
		return 0;
    	}
    else if(fmt ==3 ){
		   sprintf(out, "%2.2d%2.2d%2.2d_%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour);
		   return 0;
    	}
    else if(fmt ==4 ){
		   sprintf(out, "%2.2d%2.2d%2.2d_%2.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday-2, tp->tm_hour);
		   return 0;
    	}
	return 0;
	
}


FILE* openfile(const char *fileName,const char *mode)
{
    /*以时间命名的*/
   /*追加.log*/
    FILE *fp = fopen(fileName,mode);
    return fp;
}

int writeFile(FILE *fp,const char *str,int blog)
{
  
    assert(fp != NULL );
    char curTime[100] = {0};
    int ret = -1;

getTime(curTime,0);
 switch(blog)
 	{
	case 1:
		ret = fprintf(fp, "[%s]:{与服务器通信}send: %s\n", curTime, str);
		break;
	case 2:
		ret = fprintf(fp, "[%s]:{与服务器通信}recv: %s\n", curTime, str);
		break;
	case 3:
		 ret = fprintf(fp, "[%s]:{与安全模块通信}send: %s\n", curTime, str);
		break;
	case 4:
		 ret = fprintf(fp, "[%s]:{与安全模块通信}recv: %s\n", curTime, str);
		break;
	case 5:
		ret = fprintf(fp, "[%s]:{发送失败}error: %s\n", curTime, str);
		break;
	case 6:
		ret = fprintf(fp, "[%s]:{读配置文件失败}error: %s\n", curTime, str);
		break;
	case 7:
		ret = fprintf(fp, "        {RSSI 信息解析}: \n");
		break;
	case 8:
		ret = fprintf(fp, "[%s]: rssi:%s\n", curTime,str);
		break;
	case 9:
		ret = fprintf(fp, "{司机信息:}: %s\n", str);
		break;
	case 10:
		ret = fprintf(fp, "[%s]:{taxier 其他信息}: %s\n", curTime, str);
		break;
	case 11:
		ret = fprintf(fp, "  {进站!}\n");
		break;
	case 12:
		ret = fprintf(fp, "  {出站!}\n");
		break;	
	case 13:
		ret = fprintf(fp, "  POS=%s\n",str);
		break;	
	case 14:
		ret = fprintf(fp, "[%s]:OBU=%s  ",curTime,str);
		break;	
	case 15:
		ret = fprintf(fp, "[%s]:OBU信息帧 %s\n",curTime,str);
		break;	
	case 16:
		ret = fprintf(fp, "  {车辆来到安检点!}\n");
		break;
	default:
		return -1;
		break;

 }
    if(ret >= 0)
    {
        fflush(fp);
        return 0;
    }
    else
        return -1;

}

int closeFile(FILE *fp)
{
    return fclose(fp);
}


int convet(char *src,char *dest,int length)
{
    int i = 0;
    int k = length;

    for(i = 0;i<k;i++)
    {
       unsigned char temp;  /*定义成unsigned 型的否则会溢出*/
        temp = src[i]&0xf0;
        temp = temp >> 4;      
       if((temp>9)&&(temp<16))
        {
            temp = temp+0x37;
        }
        else{
            temp = temp+0x30;
        }
        dest[2*i] =temp;/*需要有*号*/
        temp = src[i]&0x0f;
           if((temp>9)&&(temp<16))
        {
            temp = temp+0x37;
        }
        else{
            temp = temp+0x30;
        }
        dest[2*i+1] = temp;
    }
    return 2*i+1;
}

/*与配置文件相关的函数定义*/
/*
 *去除字符串右端空格
 */
char *strtrimr(char *pstr)
{
    int i;
    i = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i >= 0))
        pstr[i--] = '\0';
    return pstr;
}
/*
 *去除字符串左端空格
 */
char *strtriml(char *pstr)
{
    int i = 0,j;
    j = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i <= j))
        i++;
    if (0<i)
        strcpy(pstr, &pstr[i]);
    return pstr;
}
/*
 *去除字符串两端空格
 */
char *strtrim(char *pstr)
{
    char *p;
    p = strtrimr(pstr);
    return strtriml(p);
}


/*
 *从配置文件的一行读出key或value,返回item指针
 *line--从配置文件读出的一行
 */
int  get_item_from_line(char *line,  ITEM *item)
{
    char *p = strtrim(line);
    int len = strlen(p);
    if(len <= 0){
        return 1;//空行
    }
    else if(p[0]=='#'){
        return 2;
    }else{
        char *p2 = strchr(p, '=');
        *p2++ = '\0';
        item->key = (char *)malloc(strlen(p)+1 );
        item->value = (char *)malloc(strlen(p2) + 1);
        strcpy(item->key,p);
        strcpy(item->value,p2);

        }
    return 0;//查询成功
}

/*
 *读取value
 */
int read_conf_value(const char *key, char *value,const char *file)
{
    char line[1024];
    FILE *fp;
    fp = fopen(file,"r");
    if(fp == NULL)
        return 1;//文件打开错误    
    while (fgets(line, 1023, fp)){
        ITEM item;
        get_item_from_line(line,&item);
        if(!strncmp(item.key,key,4)){      
            strcpy(value,item.value);
            fclose(fp);
            free(item.key);
            free(item.value);
            break;
        }

    }
    return 0;//成功

}
/*字符串转16进制*/
unsigned long convert_atohex(char* str)
{
 unsigned long var=0;
 unsigned long t;
 int len = strlen(str);

 if (len > 8) //最长8位
  return -1;
// strupr(str);//统一大写
 for (; *str; str++)
 {
  if (*str>='A' && *str <='F')
   t = *str-55;//a-f之间的ascii与对应数值相差55如'A'为65,65-55即为A
  else
   t = *str-48;
  var<<=4;
  var|=t;
 }
 return var;
}

unsigned long convert_atohex1(char* str,char *buff_hex)
{
unsigned long var=0;
 unsigned long t;
 int len = strlen(str);
 int i=0;

 for (i=0; i<len; str++,i++)
 {
	  if (*str>='A' && *str <='F')
	   t = *str-55;//a-f之间的ascii与对应数值相差55如'A'为65,65-55即为A
	  else
	   t = *str-48;
	  
	  if(i%2==0)
	  	{
		var = t;;
		continue;
	  }
	  var<<=4;
	  var|=t;
	  buff_hex[i/2] = (char)var;
	  var=0;
	  t=0;
 }
return 0;
}	

/*数据库操作的回调函数*/
static int _sql_callback(void *notused, int argc, char **argv, char **szColName)  
{  
    int i = 0;  
      
    printf("notused:0x%x, argc:%d\n", notused, argc);  
    for (i = 0; i < argc; i++)  
    {  
        printf("%s = %s\n", szColName[i], argv[i] == 0 ? "NULL" : argv[i]);  
    }  
    printf("\n");  
      
    return 0;  
}  

int local_callback(void * data, int col_count, char ** col_values, char ** col_Name)
{


if(col_count==0){
	printf("没有取到数据\n");

	return 0;
}
printf("进入回调函数\n");
  // 每条记录回调一次该函数,有多少条就回调多少次
  int i;
  char buff_send_local[13];
  char buff_obu[8];
  int pos = 0;
  int k=0;
  int ret_sql = 0;
  time_t time_local;/*上传位置信息的时间*/
  int serial_num_to_del = 0;
 
  data_rec=0xc0a80002; 

  serial_num_to_del = convert_atohex(col_values[0]);
  buff_send_local[0] = 0x20;
  convert_atohex1(col_values[1], buff_obu);
  memcpy(buff_send_local+1,buff_obu,8);
  pos = convert_atohex(col_values[2]);	
  //pos=(pos%16)+(pos>>4)%16*10+(pos>>8)%16*100;
  printf("pos = 0x%x*************插入~~~~!!!!******************************* \n",pos);
  buff_send_local[9] = 0;
  buff_send_local[10] = 0;
  buff_send_local[11] =( pos>>8)&0xff;
  buff_send_local[12] = (pos&0xff);

 
	local_num++;
	if(local_num == 65535)
		local_num = 1;
	time_local = time(NULL);
	unsigned char posInfomation[50];
	posInfomation[0] = 0xf2;
	posInfomation[1] = 0x88;
	posInfomation[2] = 0x88;
	posInfomation[3] = *((unsigned char *)&local_num+1);
	posInfomation[4] = *((unsigned char *)&local_num);
	posInfomation[5] = *((unsigned char *)&real_device_id+3);
	posInfomation[6] = *((unsigned char *)&real_device_id+2);
	posInfomation[7] = *((unsigned char *)&real_device_id+1);
	posInfomation[8] = *((unsigned char *)&real_device_id+0);
	posInfomation[9] = *((unsigned char *)&data_rec+3);
	posInfomation[10] = *((unsigned char *)&data_rec+2);
	posInfomation[11] = *((unsigned char *)&data_rec+1);
	posInfomation[12] = *((unsigned char *)&data_rec+0);
	memcpy(posInfomation+13,buff_send_local,13);
	posInfomation[26] = *((unsigned char *)&time_local+3);
	posInfomation[27] = *((unsigned char *)&time_local+2);
	posInfomation[28] =*((unsigned char *)&time_local+1);
	posInfomation[29] = *((unsigned char *)&time_local+0);
	posInfomation[30] = 0x00;/**check*/
	posInfomation[31] = 0x00;
	posInfomation[32] = 0xF1;
  
  
  	ENQueue(MyQueue,posInfomation,33);

  	memset(SQL_Delete,0,200);

	sprintf(SQL_Delete,"delete from postable where serial_num = %02x;",serial_num_to_del);
	

    ret_sql = sqlite3_exec(db, SQL_Delete, NULL, NULL, &pErrMsg);  

    if (ret_sql!= SQLITE_OK)  
    {  
        fprintf(stderr, "SQL Delete error: %s\n", pErrMsg);  
        sqlite3_free(pErrMsg); //这个要的哦，要不然会内存泄露的哦！！！  
        sqlite3_close(db);  
        return 1;  
    }  
    printf("数据库删除数据成功！\n");  

  return 0;
}



/*一下locate算法相关函数
src最后t位与dest最后t位对比
*/
bool  cmpstr(char *src,char *dest,int t)
{
	int i = 0;
	int len = strlen(src);
	char *src1 = src+len-1;
	char *dest1 = dest+t-1;
	for(i=0;i<t;i++)
		{
			if(*src1!=*dest1)
				return false;
			src1--;
			dest1--;
		}
	return true;
}

bool cmp_obu(char *obu,char *dest,int t)
{
	int i=0;
	int len = strlen(dest); 
//	printf("len = %d \n",len);
	char *dest1 = dest+len-1;
	int lenobu=strlen(obu)-1;
//	printf("lenobu = %d \n",lenobu);
	for(i=0;i<t;i++)
	{
		if(obu[lenobu]!=*dest1)
			{
//			printf("obu改变\n");
//			printf("obu[lenobu] = %c \n",obu[lenobu]);
//			printf("*dest1 = %c \n",*dest1);
			return false;
			}
		lenobu--;
		dest1--;
	}
//	printf("obu 一样没有改变\n");
	return true;

}

/*****************************************************************
传入数组及路由编号
返回对应的信号强度
******************************************************************/
int findRssi(int buff[11],int routeid)
{
	int rssi;
	if(buff[3]==routeid)
		rssi=buff[4];
	else if(buff[5]==routeid)
		rssi=buff[6];
	else if(buff[7]==routeid)
		rssi=buff[8];
	else if(buff[9]==routeid)
		rssi=buff[10];
	else rssi=0;
	return rssi;                                                               
}
/*****************************************************************
传入用于比较的二维数组以及要求差值的路由编号
返回最大值与最小值的差
******************************************************************/
int makesub(int tempbuff[acc][11],int routeID)                ////////if it ==0;    neeed debug
{
	int max,min,temp;
	temp=findRssi(tempbuff[0],routeID);
	//if(temp!=0)
	//{
		int x;
		max=temp;
		min=temp;
		for(x=1;x<acc;x++)
		{
			temp=findRssi(tempbuff[x],routeID);
			if(temp!=0)
			{
				if(temp<min)
					min=temp;
				else if (temp>max)
					max=temp;
			}
		}
		return max-min;
	//}
	//else return 0xff;               //表示数据不可用。这种情况很少见
}
/*****************************************************************
传入一帧数据，找到最大的Rssi
返回最大值所在路由的编号
******************************************************************/
int findmaxRssiaddr(int buff[11])
{
	int max,addr;
	max=buff[4];
	addr=buff[3];
	if(max<buff[6])
	{
		max=buff[6];
		addr=buff[5];
	}
	if(max<buff[8])
	{
		max=buff[8];
		addr=buff[7];
	}
	if(max<buff[10])
	{
		max=buff[10];
		addr=buff[9];
	}
	return addr;
}
/*****************************************************************
传入一个路由编号，找到它所出现的次数
返回他所出现的次数
******************************************************************/
int findRsutimes(int tempbuff[acc][11],int routeID)
{
	int i=0,times=0;
	for(i=0;i<acc;i++)
	{
		if(tempbuff[i][3]==routeID)
			times++;
		else if(tempbuff[i][5]==routeID)
			times++;
		else if(tempbuff[i][7]==routeID)
			times++;
		else if(tempbuff[i][9]==routeID)
			times++;
	}
	return times;
}
/*****************************************************************
传入一个二维数组和指定路由编号
返回指定路由所上的的平均值
******************************************************************/
int getAverRssi(int tempbuff[acc][11],int routeID)
{
	int i=0,rssi=0,sum=0,t=0;
	for(i=0;i<acc;i++)
	{
		rssi=findRssi(tempbuff[i],routeID);
		if(rssi!=0)
		{
			sum+=rssi;
			t++;
		}
			
	}
	if(t!=0)
		return (int)sum/t;
	else return 0;
}





