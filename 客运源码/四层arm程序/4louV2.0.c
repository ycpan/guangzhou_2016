/*2016-03-14 ������¥��*/
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
/*sqlite3 ͷ�ļ�*/
#include <sqlite3.h>  
#include <stddef.h>
/*FIONREAD*/
#include <sys/ioctl.h>

#define TIME_OUT_TIME 10    //time_out 10s
#define MAXSIZE 1024

int Serial_num=0;

#define R1 0    //����ÿ��·���������е�λ��11
#define R2 1	//12
#define R3 2	//13
#define R4 3	//14
#define R5 4	//15
#define R6 5	//16
#define R7 6	//17
#define R8 7	//18	
#define R9 8	//19
#define acc 6  			//��λ���ȣ��ռ����ź�ǿ�Ȏ���
/*****************************************************************
�������鼰·�ɱ��
���ض�Ӧ���ź�ǿ��
******************************************************************/
int findRssi(int buff[11],int routeid);
/*****************************************************************
�������ڱȽϵĶ�ά�����Լ�Ҫ���ֵ��·�ɱ��
�������ֵ����Сֵ�Ĳ�
******************************************************************/
int makesub(int tempbuff[acc][11],int routeID);                ////////if it ==0;    neeed debug

/*****************************************************************
����һ֡���ݣ��ҵ�����Rssi
�������ֵ����·�ɵı��
******************************************************************/
int findmaxRssiaddr(int buff[11]);

/*****************************************************************
����һ��·�ɱ�ţ��ҵ��������ֵĴ���
�����������ֵĴ���
******************************************************************/
int findRsutimes(int tempbuff[acc][11],int routeID);

/*****************************************************************
����һ����ά�����ָ��·�ɱ��
����ָ��·�����ϵĵ�ƽ��ֵ
******************************************************************/
int getAverRssi(int tempbuff[acc][11],int routeID);

unsigned char coach_obu[10][8]={0};
int coach_flag[10]={0};
int couch_num=0;

unsigned char buffer[1024],buff[1024],temp_buff[1024],data_length,buff_rec[20];
unsigned char lenbuffer[100];/*�洢��ά���鳤�ȵ�����*/
int uart_fd;
void init_uart(void);
int open_uart(void);
/*���ݿ�����Ļص�����*/
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
/*�й���־���ܵĺ�������*/
FILE* openfile(const char *fileName,const char *mode);
int getTime(char *out,int fmt);
int writeFile(FILE *fp,const char *str,int blog);
int closeFile(FILE *fp);
int convet(char *src,char *dest,int length);/*ת��16���Ƶ��ַ���*/
unsigned long convert_atohex(char* str);/*�ַ���ת16����*/
unsigned long convert_atohex1(char* str,char *buff_hex);
time_t first;/*��ȡ��ʼ��ʱ��*/
time_t heartbeat_t;/*���������*/
time_t jinchuzhan_beat;
time_t coach_time[10]={0};


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

/*����վ*/
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
void *bus_guard();

void *info_from_service_and_send_to_uart();
void *locate_process();
unsigned int crc16_serial(unsigned char *buf, unsigned int len);
ssize_t xread(int fd,void *ptr,size_t n) ;


unsigned int CRC,k;
unsigned int dev_id;
unsigned int data_attr,data_num,data_crc,f_head,f_end;
unsigned long data_rec,data_send;
int buff_num=0,ask_num=0,sockfd,nbytes,nNetTimeout=1;
pthread_t save_msg_pthread,send_msg_pthread,rec_ask_pthread,locate_process_pthread,info_from_service_and_send_to_uart_pthread,bus_guard_pthread;
struct sockaddr_in server_addr; 
struct hostent *host; //  host  主机�?
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
FILE *fop_log;/*��־�ļ�*/
FILE *send_fail_fop;
char log_name[30];/*��־�ļ���*/
char dir_log_name[40];
int portnumber;

/*ini��صĽṹ��*/
typedef struct item_t {
    char *key;
    char *value;
}ITEM;
char *strtrimr(char *pstr);//���ַ����ҿո�
char *strtriml(char *pstr);//���ַ�����ո�
char *strtrim(char *pstr);//���ַ������߿ո�
int  get_item_from_line(char *line,  ITEM *item);//����
int read_conf_value(const char *key, char *value,const char *file);//���ļ�
char value_port[10];/*��Ŷ�������ֵ*/
char value_ipadd[30];/*��Ŷ�������ֵ*/
char value_device_id[20];/*��Ŷ�������ֵ*/
char real_ip[15];
unsigned long real_device_id;

char floor_buff[5];
int floor_value = 0;
/*ѭ������*/
typedef struct
{
    char bitch[MAXSIZE][200];
    int font;
    int rear;
    int length[MAXSIZE]; 
}SqQueue;
/*��ʼ��һ���ն���*/
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
	//�����ڴ�
	MyQueue = (SqQueue *)malloc(sizeof(SqQueue));
	
	struct timeval tv,time_out;
	struct tm *p;
	long time_second;

	time_out.tv_sec=1;
	time_out.tv_usec=0;
     /*����һ���о�Ҫ�������ļ�*/
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
	/*�����ļ���ipҪ����д*/
	if((host=gethostbyname(value_ipadd))==NULL)  
	{ 
		fprintf(stderr,"Gethostname error\n"); 
		exit(1); 
	} 
	/*�����׽�������*/
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
	/*��������*/
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
	buff[5]=(unsigned char)(real_device_id>>24);   /*���ͷ�id��ʼ*/
	buff[6]=(unsigned char)(real_device_id>>16);
	buff[7]=(unsigned char)(real_device_id>>8);;
	buff[8]=(unsigned char)(real_device_id);
	buff[12] = 0x11;
	buff[13] = 0x11;
	buff[28]  = 0xf1;//f1ʱ������֡
	//printf("begin write \n");
	if((nbytes=write(sockfd,buff,29))==-1)
	{
		printf("Write Error!\n");
		exit(1);	
    }
	//printf("end of write \n");
	/*����Ҫ��д�Ķ���д����־�ļ�����*/
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

	/*���յ�����������Ϣ��ʼʱ��У��*/
	if(buff[13]==0x12)    /*У��ʱ���㷨��������*/
	{
		time_second=(long)buff[25]+((long)buff[24]<<8)+((long)buff[23]<<16)+((long)buff[22]<<24);/*28800 = 8*3600��ʱ8Сʱ*/
		printf("second  =%d\n",time_second);
		tv.tv_sec=time_second;
		tv.tv_usec=0;
		settimeofday(&tv,NULL);
		memset(convet_buff,0,100);
		convet(buff,convet_buff,33);
		writeFile(fop_log, convet_buff, 2);
		closeFile(fop_log);
		/*ʱ��У׼֮�����´��ļ�������µ�һ�콫�ᴴ���µ��ĵ�*/
		getTime(log_name, 3);
		sprintf(dir_log_name,"/root/%s.log",log_name);
		fop_log = openfile(dir_log_name,"a+");	
	}
	/*��ʼ�����߳�*/
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



	
	int err=pthread_join(save_msg_pthread,NULL);/*�����ȴ��߳��˳�*/
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
	data_num=0x00;/*��ˮ�ų�ʼ��*/
	while(1)
	{	
		/*����Ӧ�ü��ļ���***********/		
		memset(store_save,0,250);
		memset(temp_buff,0,1024);
		data_length= nread(uart_fd,temp_buff);/*���ض��������ֽ�����������ݳ���*/
		time(&timep);
		//printf("save_msg datalength = %02x \n",data_length);
		switch(temp_buff[0])   /*���������ݵĵ�һ���ֽ�*/
		{ 
			case 0x01:    /*OBU*/
				f_head=0xf2;/*֡ͷ*/
				data_attr=0xd0;/*����*/
				data_num++;/*��ˮ��*/
				dev_id = real_device_id;/*�豸id*/
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/				
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			case 0x04:		/*RSSI����֡*/
				f_head=0xf2;
				data_attr=0xd2;
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/						
				f_end=0xf1;		
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;

			case 0x02:/*˾���ʸ�֤1*/
				f_head=0xf2;
				data_attr=0xd1;/*����*/
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/				
				f_end=0xf1;			
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			case 0x05:/*˾���ʸ�֤2*/
				f_head=0xf2;
				data_attr=0xd5;/*�����Լ������*/
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/
				f_end=0xf1;				
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;
				
			case 0x06:/* ·������*/
				f_head=0xf2;
				data_attr=0xd6;
				data_num++;
				dev_id = real_device_id;
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				break;

			case 0xec:  //Զ�̲�ѯӦ��֡
				printf("Զ�̲�ѯӦ��֡�յ�****\n");
				f_head=0xf2;
				data_attr=0xaa12;//unknown
				data_num = 43;
				dev_id = real_device_id;	
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/
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
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
					
		}

			
		/*��װͷ*/
		store_save[0]=*((unsigned char *)&f_head);
		store_save[1]=*((unsigned char *)&data_attr+1);/*֡���ͣ���д���ֽ�*/
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
		
		/*��ʼд��ʱ���ĸ��ֽ�*/
		store_save[data_length+13]=*((unsigned char *)&timep+3);
		store_save[data_length+14]=*((unsigned char *)&timep+2);
		store_save[data_length+15]=*((unsigned char *)&timep+1);
		store_save[data_length+16]=*((unsigned char *)&timep);
		
		/*У����2���ֽ�ͨ���ոշ����������ݼ����Ƚ���У��*/	
		data_crc=crc16(store_save,17+data_length);
		store_save[data_length+17]=*((unsigned char *)&data_crc+1);
		store_save[data_length+18]=*((unsigned char *)&data_crc);
		store_save[data_length+19]=*((unsigned char *)&f_end);
		
		/*data_length�Ƕ��������ֽ���*/
		if(data_length>0)
		{
			/*װ������*/
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
	unsigned char buffer_send_rsu[200]; /*����*/
	int buffer_send_rsu_len = 0;/*�����еĳ���*/
	unsigned char error_buffer[200];
	while(1)
	{
		/*��־ɾ������*/
		if(difftime(time(NULL),first) >= 3600)
		{  
		/*�ᱣ��һ��Сʱ���������û��������Ҳ����Ƶ���Ĵ����µ��ļ�*/
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
				/*�����д*/
				for(j=0;j<2;j++)                 
				{	
					signal(SIGPIPE,SIG_IGN);
					if((nbytes=write(sockfd,buffer_send_rsu,buffer_send_rsu_len))!=-1)
						break;
				}
				printf("send error!!  ");
				/*д����־*//*����д��һ���ļ�����ȽϺ�*/
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
	RSU_heart_beat[1] = 0x00;/*��������*/
	RSU_heart_beat[2] = 0x10;
	while(1)
	{
		/*����ʱ��30����*/
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
			/*��ʼд��ʱ���ĸ��ֽ�*/
			RSU_heart_beat[22]=*((unsigned char *)&heartbeat_t+3);
			RSU_heart_beat[23]=*((unsigned char *)&heartbeat_t+2);
			RSU_heart_beat[24]=*((unsigned char *)&heartbeat_t+1);
			RSU_heart_beat[25]=*((unsigned char *)&heartbeat_t);
			RSU_heart_beat[26]=0x00;/*check code 0*/
			RSU_heart_beat[27]=0x00;
			RSU_heart_beat[28]=0xf1;		
			signal(SIGPIPE,SIG_IGN);/*���³����˳����ź�SIGPIPE*/
			if((nbytes=write(sockfd,RSU_heart_beat,29))!=29)//=!29  error occured
			{
				/*д����־*/
				printf("write  is wrong ********************************************\n");
				close(sockfd);
				sockfd=socket(AF_INET,SOCK_STREAM,0) ;  /*SOCK_STREAM  tcp*/		
				bzero(&server_addr,sizeof(server_addr)); 
				server_addr.sin_family=AF_INET;          // IPV4
				server_addr.sin_port=htons(portnumber); 
				server_addr.sin_addr=*((struct in_addr *)host->h_addr); 					
				val =fcntl(sockfd,F_GETFL,0); /*��ȡ��־λ*/
				fcntl(sockfd,F_SETFL,val | O_NONBLOCK); /*�ı��־λ��ӷ���������*/
				int connect_flag;
				connect_flag = connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
				sleep(1);
				printf("lianjie fuzhang \n");
				FD_ZERO(&set); /*��set����ʹ�����в����κ�fd*/
				FD_SET(sockfd,&set);/*��sockfd����set����*/
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
		//ʱ���ز���
		int i=0;
		int now_obu=0;
		for(i=0;i<10;i++)
		{
			if(coach_obu[i][7]!=0)
			{
				if(difftime(time(NULL),coach_time[i]) >= 30&&coach_flag[i]==0x401)   
				{			
					//printf("the difftime is %f\n",difftime(time(NULL),four_time[now_obu]));
					unsigned char coach_char[16]={0};
					convet(coach_obu[i], coach_char,8);
					writeFile(fop_log, coach_char,14);
					writeFile(fop_log, NULL,17);

					time_t timep;
					time(&timep);	
					/*����*/
					f_head=0xf2;/*֡ͷ*/
					data_attr=0x8888;/*����*/
					data_num++;/*��ˮ��*/
					dev_id = real_device_id;/*�豸id*/
					data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/				
					f_end=0xf1;
					if(data_num==65535)
					{
						data_num=0x01;
					}
					/*��װͷ*/
					unsigned char pos_buff[50];
					memset(pos_buff,0,50);
					pos_buff[0]=*((unsigned char *)&f_head);
					pos_buff[1]=*((unsigned char *)&data_attr+1);/*֡���ͣ���д���ֽ�*/
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
					memcpy(pos_buff+14,coach_obu[i],8);
					pos_buff[22]=0x00;
					pos_buff[23]=0x00;
					pos_buff[24]=0x04;
					pos_buff[25]=0x09;			
					/*��ʼд��ʱ���ĸ��ֽ�*/
					pos_buff[26]=*((unsigned char *)&timep+3);
					pos_buff[27]=*((unsigned char *)&timep+2);
					pos_buff[28]=*((unsigned char *)&timep+1);
					pos_buff[29]=*((unsigned char *)&timep);
					
					/*У����2���ֽ�ͨ���ոշ����������ݼ����Ƚ���У��*/	
					data_crc=crc16(pos_buff,30);
					pos_buff[30]=*((unsigned char *)&data_crc+1);
					pos_buff[31]=*((unsigned char *)&data_crc);
					pos_buff[32]=*((unsigned char *)&f_end);
					
					/*װ������*/
					ENQueue(MyQueue,pos_buff,33);
					
					//������¥���������
					coach_flag[i]=0;
					coach_time[i]=0;
					memset(coach_obu[i],0,8);
					for(i=0;i<33;i++)
						printf("%x",pos_buff[i]);
				}
				else sleep(5);
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
	int data_len;//�������·������ݳ���
	int flag = 0;
	unsigned char sprintf_time_out[40];
	unsigned char buff_recv_service[200];
	unsigned char buff_send_uart[200];
	int nread;
	unsigned int serial_num_service = 1;	
	while(1)
	{
		printf("info_from_service_and_send_to_uart \n");
		FD_ZERO(&read_fd);
		FD_SET(sockfd,&read_fd);	
		tm.tv_sec = 3;
		tm.tv_usec = 0;		
		flag = select(sockfd+1,&read_fd,NULL,NULL,&tm);
		switch(flag)
		{
			case 0:
				printf("time out ");
				break;
			case -1:
				printf("select error occoured \n");
				break;
			default:     //select ���ػὫδ׼���õ����������
				printf("enter default \n");
				if(FD_ISSET(sockfd,&read_fd))
				{
					ioctl(sockfd,FIONREAD,&nread);//���Ի����������ж��ٸ��ֽڿ��Ա���ȡ��Ȼ����ֽ��������nread����
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
						case 0x13://RSU״̬����֡
							data_len = 8;//RSU_ID
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 12;//�������ڵĳ���
							buff_send_uart[2] = 0x13;					
							memcpy(buff_send_uart+3,buff_recv_service+14,8);
							buff_send_uart[11] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,12)) == -1)
							{
								printf("write error \n");
							}
							break;
				
						case 0x14://RSU��Ϣ����֡
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 16;
							buff_send_uart[2] = 0x14;				
							memcpy(buff_send_uart+3,buff_recv_service+14,12);
							buff_send_uart[15] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,16)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<16;k++)
							{
							printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						
						case 0xec://Զ�̲�ѯ֡
							printf("������Զ�̲�ѯ\n");
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 12;
							buff_send_uart[2] = 0xec;							
							memcpy(buff_send_uart+3,buff_recv_service+14,8);
							buff_send_uart[11] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,12)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<12;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;

						case 0xe1:   //PANID����֡
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 14;
							buff_send_uart[2] = 0xe1;
							memcpy(buff_send_uart+3,buff_recv_service+14,10);
							buff_send_uart[13] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,14)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<14;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						
						case 0xe2:   //�ŵ�����֡
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 13;
							buff_send_uart[2] = 0xe2;
							memcpy(buff_send_uart+3,buff_recv_service+14,9);
							buff_send_uart[12] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,13)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<13;k++)
							{
							printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;
						case 0xe3:   //���͹�������֡
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 13;
							buff_send_uart[2] = 0xe3;
							memcpy(buff_send_uart+3,buff_recv_service+14,9);
							buff_send_uart[12] = 0x00;//У��
							if((nbytes = write(uart_fd,buff_send_uart,13)) == -1)
							{
								printf("write error \n");
							}
							for(k = 0;k<13;k++)
							{
								printf("buff_send_uart = %02x \n",buff_send_uart[k]);
							}
							break;

						case 0xe4:   //·������ʱ��������֡
							memset(buff_send_uart,0,200);
							buff_send_uart[0] = 0x7E;
							buff_send_uart[1] = 14;
							buff_send_uart[2] = 0xe4;
							memcpy(buff_send_uart+3,buff_recv_service+14,10);
							buff_send_uart[13] = 0x00;//У��
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


//������
int huadulist[28]={0x0096,0x0078,0x0080,0x0053,0x0051,0x0066,0x0065,
					  0x0059,0x0092,0x0085,0x0058,0x0069,0x0077,0x0061,
					  0x0070,0x0067,0x0089,0x0060,0x00,0x00,0x00,
					  0x00,0x00,0x00,0x00,0x00,0x00,0x00};
int huadu[6][14]={0};
//int huadubuf[6][acc][11];
//int huadudatebar[6][9][200];
int huadudatebar[6][4]={0};// ��¼·��˳��
int huadu_route_rssi[6][8]={0};// ��¼·��ֵ
//int pos_book[1024];

int huadu_obu_num=0;//���廨��·�ߵ�obu���Ա����huadu��ά����
/*����������ÿ��λ��Ԫ�صĹ���*/
#define AOBU 					0
#define	Astateflag				1	//1��ʾ��վ��2ͣ����3���࣬4��վ
#define	Astopnum				2	//��ʾͣ��˳�򡣳���������ѭ�����ȳ�ԭ��
#define Apos_old 				3
#define Adatebar				4
#define APOS_VALUE105			5	// ÿ��λ�õ�Ȩֵ
#define APOS_VALUE108			6
#define APOS_VALUE110			7
#define APOS_VALUE138			8	
#define APOS_VALUE999			9	
#define Atemp					10
#define Atemp2					11
#define Atemp3					12
#define Abuf					13

void *locate_process()
{
	printf("**********locate_process*********************\n"); 
	int pos_book[1024]={0};
	char obu_id[17] ;
	unsigned char pos_change[3];
	int rssi_data[11]; // 3 5 7 9λΪ·��id
	int locate_i=0;//��ǰ֡��������λ�ã�0-5�ֻ�
	int locate_p=0;//�ϴ������ִΣ�5֡һ��,�����ۼ����ݣ�����Ϊ��
	int locate_p1=0;//��ǰ�ִΣ�����locate_p
	int unget_t=0;//������λ�ô���
	rssi_data[0]=00;
	rssi_data[1]=00;
	rssi_data[2]=00;
	//int databar[9][200];
	//int ifstopbuf[acc][11];
	//int missfireflag=1;//��һ��Ϩ���жϣ�1Ϊ��Ϩ��0ΪϨ��shang yi ci xi huo pan duan biao zhi ,0 wei xihuo 1 wei bu xihuo
	
    int ret = 0;  
	int Serial_Id_Sql_local=0;	
	int row=0;
	int col=0;
	int i=0;
	char time_sql[30]= {0};
	for(i=0;i<=5;i++)
	{
		memset(huadu[i],0,14);
		memset(huadudatebar[i],0,4);
	}
	while(1)
	{
		char **result;
		char *first;/*ȡ��һ����¼*/
		row=0;
		col = 0;
		int count = 0;
		
		
		char obu_id_print[17] ;
		sleep(20);
		/*order by obu*/
		while(row<2)
		{
			printf("***************row<1\n");
			sqlite3_get_table(db,SQL_order,&result,&row,&col,&pErrMsg);
			//Rssi_table���е������obu_id���򣬲��ص���result��
			if(NULL!=pErrMsg)
			{
				sqlite3_free_table(result);
				pErrMsg=NULL;
				return ;
			}
			printf("***���ݿ�����û�����ݵȴ�5��***\n");
			sleep(5);
		}
		printf("***��������***\n");

    	ret = sqlite3_exec(db, sSQL_delete, _sql_callback, 0, &pErrMsg);  
  		if (ret != SQLITE_OK)  
   		{  
      	  	fprintf(stderr, "SQL error: %s\n", pErrMsg);  
     	  	sqlite3_free(pErrMsg);  
			sqlite3_close(db);  
			return ;  
		}  
		memcpy(obu_id_print,result[12],16);
		printf("****************obu_id  = %s \n",obu_id_print);										
		for(i=0;i<(row+1)*col;i++)
			printf("****************result[%d]=%s\n",i,result[i]);		
		count = 12;
		/*û�н���ѭ�����µ�bug*/
		while((count<(row+1)*col))
		{
			int pos=0;	
			int pos_old=0;
			int posvalue_105=0,posvalue_108=0,posvalue_110=0,posvalue_138=0,posvalue_999=0;
			int eff=3;
			int k=0;
			int huadu_temp_obu_num=0;
			unsigned char pos_log[3];
			char obu_temp_id[17] ;
			int obu_last3;
			//�ж���·

			//������·					
			if(cmp_obu(obu_id, result[count], 3))
			{
				printf("!!!!!!!!!!!!!!!!!!!�ɵ�obu!!!!!!!!!!!!!!!!!!\n");										
				memcpy(obu_temp_id,result[count],16);
				obu_last3=atoi(obu_temp_id+13);	
				printf("  					obu  =%d\n",obu_last3);
				//���obu�Ƿ���ڶ�ά�����У���ȷ��huadu_temp_obu_num
				for(k=0;k<=5;k++)//�鿴�˳����
				{
					if(obu_last3==huadu[k][AOBU])
					{
						huadu_temp_obu_num=k;
						printf("*********��������֮�У��������Ϊ:   %d  \n",huadu_temp_obu_num);
						break;
					}
					//else huadu_temp_obu_num=huadu_obu_num;						
				}
				if(huadu_temp_obu_num==0)
					printf("*********����������֮��!!!!!!!!!��bug!!!\n*********����������֮��!!!!!!!!!��bug!!! \n \n");
				pos_old=huadu[huadu_temp_obu_num][Apos_old];
				posvalue_105=huadu[huadu_temp_obu_num][APOS_VALUE105];
				posvalue_108=huadu[huadu_temp_obu_num][APOS_VALUE108];
				posvalue_110=huadu[huadu_temp_obu_num][APOS_VALUE110];
				posvalue_138=huadu[huadu_temp_obu_num][APOS_VALUE138];
				posvalue_999=huadu[huadu_temp_obu_num][APOS_VALUE999];
				pos=pos_old;
				
				/**************���´��봦��λ����**************/
				memset(huadu_route_rssi[huadu_temp_obu_num],0,9);
				for(i=count+1,k=0;k<4;k++,i+=2)
				{								
					rssi_data[eff]=atoi(result[i]);
					rssi_data[eff+1]=atoi(result[i+1]);
					printf("the rssi_data is %x  %x\n",rssi_data[eff],rssi_data[eff+1]);
					huadudatebar[huadu_temp_obu_num][k]=rssi_data[eff];
					eff+=2;
					if(rssi_data[eff]==0)
						continue;
					else if(rssi_data[eff]==0x11)
						huadu_route_rssi[huadu_temp_obu_num][R1]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x12)
						huadu_route_rssi[huadu_temp_obu_num][R2]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x13)
						huadu_route_rssi[huadu_temp_obu_num][R3]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x14)
						huadu_route_rssi[huadu_temp_obu_num][R4]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x15)
						huadu_route_rssi[huadu_temp_obu_num][R5]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x16)
						huadu_route_rssi[huadu_temp_obu_num][R6]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x17)
						huadu_route_rssi[huadu_temp_obu_num][R7]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x18)
						huadu_route_rssi[huadu_temp_obu_num][R8]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x19)
						huadu_route_rssi[huadu_temp_obu_num][R9]=rssi_data[eff+1];
				}						
				int route_num=0;
				for(k=0;k<4;k++)
				{
					if(huadudatebar[huadu_temp_obu_num][k]>0)
						route_num++;
				}
				/****************��λ�Լ��ж�����******************/
				//printf("************��%d��·��\n",route_num);
				switch(route_num)
				{
					case 0x01://1��·�����
		 				printf("**********1��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12)
							posvalue_108+=2;					
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x13||huadudatebar[huadu_temp_obu_num][R1]==0x14)
							posvalue_105+=2;									
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x19)
							posvalue_999+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x02://2��·�����
		 				printf("**********2��·��\n");
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12||huadudatebar[huadu_temp_obu_num][R1]==0x13)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14||huadudatebar[huadu_temp_obu_num][R1]==0x18)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x15)
							posvalue_110+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11&&huadudatebar[huadu_temp_obu_num][R2]==0x12)
							posvalue_138+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x19)
							posvalue_999+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x03://3��·�����
		 				printf("**********3��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16&&huadudatebar[huadu_temp_obu_num][R2]==0x11&&huadudatebar[huadu_temp_obu_num][R3]==0x12)						
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x04://4��·�����
		 				printf("**********4��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x12||huadudatebar[huadu_temp_obu_num][R1]==0x13)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14||huadudatebar[huadu_temp_obu_num][R1]==0x18)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
							{
								posvalue_108+=1;
								posvalue_138+=1;
							}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;						
					default: printf("*********·�ɸ������ԣ�����bug!!!!\n");
						break;
				}
			}
			else
			{		//****************************************************************************************************************************************
				printf("***************��obu*************\n");
				memcpy(obu_id,result[count],16);//��ǰ�ִ�obu
				memcpy(obu_temp_id,result[count],16);
				obu_last3=atoi(obu_temp_id+13);	
				printf("*********obu_last3=%d\n",obu_last3);
				int first_in_flag=0;
				for(k=0;k<=5;k++)//�鿴�˳��Ƿ��Ѿ�����
				{
					if(obu_last3==huadu[k][AOBU])
					{
						huadu_temp_obu_num=k;
						first_in_flag=0;
						printf("*********��������֮�У��������Ϊ:   %d  \n",huadu_temp_obu_num);
						break;
					}
				}
				if(huadu_temp_obu_num==0)//�˳�����������
				{
					if(huadu_obu_num==5)
						huadu_obu_num=0;
					else huadu_obu_num+=1;
					huadu_temp_obu_num=huadu_obu_num;
					printf("*********����������֮�У��������Ϊ:   %d  \n",huadu_temp_obu_num);					
					memset(huadu[huadu_temp_obu_num],0,14);		
					huadu[huadu_temp_obu_num][AOBU]=obu_last3;
					first_in_flag=1;
				}							
								
				pos_old=huadu[huadu_temp_obu_num][Apos_old];
				posvalue_105=huadu[huadu_temp_obu_num][APOS_VALUE105];
				posvalue_108=huadu[huadu_temp_obu_num][APOS_VALUE108];
				posvalue_110=huadu[huadu_temp_obu_num][APOS_VALUE110];
				posvalue_138=huadu[huadu_temp_obu_num][APOS_VALUE138];
				posvalue_999=huadu[huadu_temp_obu_num][APOS_VALUE999];
				pos=pos_old;
				if(first_in_flag==1)
				{
					pos=001;
					huadu[huadu_temp_obu_num][Astateflag]=1;
					huadu[huadu_temp_obu_num][Apos_old]=pos;
					writeFile(fop_log, obu_id,14);
					writeFile(fop_log, NULL,11);
					/*д�����ݿ�*/
					Serial_Id_Sql_local++;
					if (Serial_Id_Sql_local==0xFFFFF)/*���ݿ��ID*/
						Serial_Id_Sql_local=1;
					getTime(time_sql,0);
					memset(SQL_insert_local,0,100);
					sprintf(SQL_insert_local,"insert into postable values(%d,'%s',%d,'%s');",Serial_Id_Sql_local,obu_id,pos,time_sql);
  					printf("SQL_Insert_local:%s::::**\n",SQL_insert_local);
	 				ret = sqlite3_exec(db, SQL_insert_local, _sql_callback, 0, &pErrMsg);  
    				if (ret!= SQLITE_OK)  
    				{  
						fprintf(stderr, "SQL insert error: %s\n", pErrMsg);  
						sqlite3_free(pErrMsg); //���Ҫ��Ŷ��Ҫ��Ȼ���ڴ�й¶��Ŷ������  
						sqlite3_close(db);  
						return ;  
					}  	
					continue;
				}
				/**************���´��봦��λ����**************/
				memset(huadu_route_rssi[huadu_temp_obu_num],0,9);
				for(i=count+1,k=0;k<4;k++,i+=2)
				{								
					rssi_data[eff]=atoi(result[i]);
					rssi_data[eff+1]=atoi(result[i+1]);
					printf("the rssi_data is %x  %x\n",rssi_data[eff],rssi_data[eff+1]);
					huadudatebar[huadu_temp_obu_num][k]=rssi_data[eff];
					eff+=2;
					if(rssi_data[eff]==0)
						continue;
					else if(rssi_data[eff]==0x11)
						huadu_route_rssi[huadu_temp_obu_num][R1]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x12)
						huadu_route_rssi[huadu_temp_obu_num][R2]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x13)
						huadu_route_rssi[huadu_temp_obu_num][R3]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x14)
						huadu_route_rssi[huadu_temp_obu_num][R4]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x15)
						huadu_route_rssi[huadu_temp_obu_num][R5]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x16)
						huadu_route_rssi[huadu_temp_obu_num][R6]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x17)
						huadu_route_rssi[huadu_temp_obu_num][R7]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x18)
						huadu_route_rssi[huadu_temp_obu_num][R8]=rssi_data[eff+1];
					else if(rssi_data[eff]==0x19)
						huadu_route_rssi[huadu_temp_obu_num][R9]=rssi_data[eff+1];
				}						
				int route_num=0;
				for(k=0;k<4;k++)
				{
					if(huadudatebar[huadu_temp_obu_num][k]>0)
						route_num++;
				}
				/****************��λ�Լ��ж�����******************/
				//printf("************��%d��·��\n",route_num);
				switch(route_num)
				{
					case 0x01://1��·�����
		 				printf("**********1��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12)
							posvalue_108+=2;					
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x13||huadudatebar[huadu_temp_obu_num][R1]==0x14)
							posvalue_105+=2;									
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x19)
							posvalue_999+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x02://2��·�����
		 				printf("**********2��·��\n");
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12||huadudatebar[huadu_temp_obu_num][R1]==0x13)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14||huadudatebar[huadu_temp_obu_num][R1]==0x18)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x15)
							posvalue_110+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11&&huadudatebar[huadu_temp_obu_num][R2]==0x12)
							posvalue_138+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x19)
							posvalue_999+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x03://3��·�����
		 				printf("**********3��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x12)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16&&huadudatebar[huadu_temp_obu_num][R2]==0x11&&huadudatebar[huadu_temp_obu_num][R3]==0x12)						
						{	
							posvalue_138+=1;
							posvalue_108+=1;
						}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;
					case 0x04://4��·�����
		 				printf("**********4��·��\n");	
						if(huadudatebar[huadu_temp_obu_num][R1]==0x11)
							posvalue_138+=1;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x12||huadudatebar[huadu_temp_obu_num][R1]==0x13)
							posvalue_108+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x14||huadudatebar[huadu_temp_obu_num][R1]==0x18)
							posvalue_105+=2;
						else if(huadudatebar[huadu_temp_obu_num][R1]==0x16)
							{
								posvalue_108+=1;
								posvalue_138+=1;
							}
						else printf("*********��Ч���ݣ�������Ȩ\n");
				 		break;						
					default: printf("*********·�ɸ������ԣ�����bug!!!!\n");
						break;
				}					
			}
			/****************��λ����ж�����******************/	
			printf("*********�ж���ϣ�����鿴��λ��Ȩ��!!!!\n");
			printf("************��Ȩ��posvalue_105 = %d \n",posvalue_105);
			printf("************��Ȩ��posvalue_108 = %d \n",posvalue_108);
			printf("************��Ȩ��posvalue_110 = %d \n",posvalue_110);
			printf("************��Ȩ��posvalue_138 = %d \n",posvalue_138);
			int location_flag=0;// ��λ��ʶ��1 �жϳ�λ�ã�0û�жϳ�λ��
			if(posvalue_999>=4)
			{
				location_flag=1;
				printf("************Ȩ���жϳ�999��ͬʱ����״̬Ϊ %d \n",huadu[huadu_temp_obu_num][Astateflag]);
				printf("************֮ǰ����λ��Ϊ %d \n",pos);
				if(huadu[huadu_temp_obu_num][Astateflag]>=3)
				{
					pos=999;
					huadu[huadu_temp_obu_num][Astateflag]=4;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}else if(huadu[huadu_temp_obu_num][Astateflag]==1)
				{
					huadu[huadu_temp_obu_num][Astateflag]=2;
					pos=105;
					//pos_book[pos]=1;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				else if(huadu[huadu_temp_obu_num][Astateflag]==2)
				{
					pos=pos_old;
					huadu[huadu_temp_obu_num][Astateflag]=2;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				printf("************��������λ��Ϊ %d \n",pos);
			}
			else if(posvalue_108>=5&&posvalue_108> posvalue_105)
			{
				location_flag=1;
				printf("************Ȩ���жϳ�108��ͬʱ����״̬Ϊ %d \n",huadu[huadu_temp_obu_num][Astateflag]);
				printf("************֮ǰ����λ��Ϊ %d \n",pos);
				if(huadu[huadu_temp_obu_num][Astateflag]==3)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]=3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}else if(huadu[huadu_temp_obu_num][Astateflag]<=2)
				{
					pos=108;
					huadu[huadu_temp_obu_num][Astateflag]=2;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				else if(huadu[huadu_temp_obu_num][Astateflag]==4)
				{
					pos=999;
					huadu[huadu_temp_obu_num][Astateflag]=4;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				printf("************��������λ��Ϊ %d \n",pos);
			}
			else if(posvalue_105>=5)
			{
				location_flag=1;
				printf("************Ȩ���жϳ�105��ͬʱ����״̬Ϊ %d \n",huadu[huadu_temp_obu_num][Astateflag]);
				printf("************֮ǰ����λ��Ϊ %d \n",pos);
				if(huadu[huadu_temp_obu_num][Astateflag]==3)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]=3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}else if(huadu[huadu_temp_obu_num][Astateflag]<=2)
				{
					pos=105;
					huadu[huadu_temp_obu_num][Astateflag]=2;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				else if(huadu[huadu_temp_obu_num][Astateflag]==4)
				{
					pos=999;
					huadu[huadu_temp_obu_num][Astateflag]=4;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				printf("************��������λ��Ϊ %d \n",pos);
			}
			else if(posvalue_138>=7)
			{
				location_flag=1;
				printf("************Ȩ���жϳ�138��ͬʱ����״̬Ϊ %d \n",huadu[huadu_temp_obu_num][Astateflag]);
				printf("************138λ�õ�Ȩ��Ϊ   %d \n",posvalue_138);
				printf("************֮ǰ����λ��Ϊ %d \n",pos);
				if(huadu[huadu_temp_obu_num][Astateflag]==1&&posvalue_138>=25)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]==3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}else if(huadu[huadu_temp_obu_num][Astateflag]==2&&posvalue_138>=10)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]=3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}			
				else if(huadu[huadu_temp_obu_num][Astateflag]==3)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]=3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}			
				else if(huadu[huadu_temp_obu_num][Astateflag]==4)
				{
					pos=999;
					huadu[huadu_temp_obu_num][Astateflag]=4;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}	
				else printf("************���۲��㣬�������� \n",pos);
				printf("************��������λ��Ϊ %d \n",pos);
			}
			else if(posvalue_110>=8)
			{
				location_flag=1;
				printf("************Ȩ���жϳ�110��ͬʱ����״̬Ϊ %d \n",huadu[huadu_temp_obu_num][Astateflag]);
				printf("************֮ǰ����λ��Ϊ %d \n",pos);
				if(huadu[huadu_temp_obu_num][Astateflag]==3)
				{
					pos=138;
					huadu[huadu_temp_obu_num][Astateflag]=3;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}else if(huadu[huadu_temp_obu_num][Astateflag]<=2)
				{
					pos=110;
					huadu[huadu_temp_obu_num][Astateflag]=2;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				else if(huadu[huadu_temp_obu_num][Astateflag]==4)
				{
					pos=999;
					huadu[huadu_temp_obu_num][Astateflag]=4;
					posvalue_105=0;
					posvalue_108=0;
					posvalue_110=0;
					posvalue_138=0;
					posvalue_999=0;
				}
				printf("************��������λ��Ϊ %d \n",pos);
			}
			else printf("************û�ﵽȨ��Ҫ��location_flag=0,���ϴ�λ������ \n");
			if(location_flag==1)
			{
				printf("\n>>>>>�ﵽ��λ����λ��Ϊ:[obu:%d]: Final Result: pos = %d\n",obu_last3,pos);
				printf("************��Ȩ��posvalue_105 = %d \n",posvalue_105);
				printf("************��Ȩ��posvalue_108 = %d \n",posvalue_108);
				printf("************��Ȩ��posvalue_110 = %d \n",posvalue_110);
				printf("************��Ȩ��posvalue_138 = %d \n",posvalue_138);
				/*if(pos!=pos_old&&pos!=1&&pos!=999)
				{
					pos_book[pos_old]=0;
					if(pos_book[pos]==0)
						pos_book[pos]=obu_last3;
					else if(pos==108&&pos_book[138]==0)
					{
						pos=138;
						huadu[huadu_temp_obu_num][Astateflag]=3;
						pos_book[pos]=obu_last3;
					}else if(pos==105&&pos_book[108]==0)
					{
						pos=108;
						pos_book[pos]=obu_last3;
					}else if(pos==138&&pos_book[108]==0)	
					{
						pos=pos_old;
						if(pos_old==1)
							huadu[huadu_temp_obu_num][Astateflag]=1;
						else huadu[huadu_temp_obu_num][Astateflag]=2;
						pos_book[pos]=obu_last3;
					}
				}*/
				//printf("*********���ݷ���λ�������λ��Ϊ:   %d  \n",pos);
				huadu[huadu_temp_obu_num][Apos_old]=pos;
				huadu[huadu_temp_obu_num][APOS_VALUE105]=posvalue_105;
				huadu[huadu_temp_obu_num][APOS_VALUE108]=posvalue_108;
				huadu[huadu_temp_obu_num][APOS_VALUE110]=posvalue_110;
				huadu[huadu_temp_obu_num][APOS_VALUE138]=posvalue_138;			
				huadu[huadu_temp_obu_num][APOS_VALUE999]=posvalue_999;
				if(pos==999)
				{
					printf("��վ��վ��վ!!!!!!!!!!!!!!!!!!!");
					huadu[huadu_temp_obu_num][Astateflag]=4;					
					writeFile(fop_log, obu_id,14);
					writeFile(fop_log, NULL,12);
					/*д�����ݿ�*/
					Serial_Id_Sql_local++;
					if (Serial_Id_Sql_local==0xFFFFF)/*���ݿ��ID*/
						Serial_Id_Sql_local=1;
					getTime(time_sql,0);
					memset(SQL_insert_local,0,100);
					sprintf(SQL_insert_local,"insert into postable values(%d,'%s',%d,'%s');",Serial_Id_Sql_local,obu_id,pos,time_sql);	
	  				printf("SQL_Insert_local:%s::::**\n",SQL_insert_local);
		 			ret = sqlite3_exec(db, SQL_insert_local, _sql_callback, 0, &pErrMsg);  
	    			if (ret!= SQLITE_OK)  
	    			{  
						fprintf(stderr, "SQL insert error: %s\n", pErrMsg);  
						sqlite3_free(pErrMsg); //���Ҫ��Ŷ��Ҫ��Ȼ���ڴ�й¶��Ŷ������  
						sqlite3_close(db);  
						return ;  
					}  
					printf("λ����Ϣ�������ݿ�ɹ�\n"); 
				}				
				else if(pos!=0)
				{
					/*  ��posת�����ַ���д����־��*/
					pos_change[0]=pos/100;
					//printf("pos_change[0]=0x%x\n",pos_change[0]);
					pos_change[1]=(pos%100)/10<<4;
					//printf("pos_change[1]=0x%x\n",pos_change[1]);
					pos_change[2]=pos%10;
					//printf("pos_change[2]=0x%x\n",pos_change[2]);
					pos_change[1]=pos_change[1]+pos_change[2];
					//printf("pos_change[1]=0x%x\n",pos_change[1]);
					convet(pos_change, pos_log, 2);
					printf("pos_log=%s\n",pos_log);
					writeFile(fop_log, obu_id,14);	
					writeFile(fop_log, pos_log,13);		
					/*д�����ݿ�*/
					Serial_Id_Sql_local++;
					if (Serial_Id_Sql_local==0xFFFFF)/*���ݿ��ID*/
						Serial_Id_Sql_local=1;
					getTime(time_sql,0);
					memset(SQL_insert_local,0,100);
					sprintf(SQL_insert_local,"insert into postable values(%d,'%s',%d,'%s');",Serial_Id_Sql_local,obu_id,pos,time_sql);
	  				printf("SQL_Insert_local:%s::::**\n",SQL_insert_local);
		 			ret = sqlite3_exec(db, SQL_insert_local, _sql_callback, 0, &pErrMsg);  
	    			if (ret!= SQLITE_OK)  
	    			{  
						fprintf(stderr, "SQL insert error: %s\n", pErrMsg);  
						sqlite3_free(pErrMsg); //���Ҫ��Ŷ��Ҫ��Ȼ���ڴ�й¶��Ŷ������  
						sqlite3_close(db);  
						return ;  
					}  
					printf("λ����Ϣ�������ݿ�ɹ�\n");  
				}	
			}
			else
			{
				printf("************û�ﵽȨ��Ҫ��location_flag=0,�����ݴ������ \n");
				printf("\n>>>>>�ϵ�λ��Ϊpos = %d\n",pos);
				printf("************��Ȩ��posvalue_105 = %d \n",posvalue_105);
				printf("************��Ȩ��posvalue_108 = %d \n",posvalue_108);
				printf("************��Ȩ��posvalue_110 = %d \n",posvalue_110);
				printf("************��Ȩ��posvalue_138 = %d \n",posvalue_138);
				huadu[huadu_temp_obu_num][Apos_old]=pos;
				huadu[huadu_temp_obu_num][APOS_VALUE105]=posvalue_105;
				huadu[huadu_temp_obu_num][APOS_VALUE108]=posvalue_108;
				huadu[huadu_temp_obu_num][APOS_VALUE110]=posvalue_110;
				huadu[huadu_temp_obu_num][APOS_VALUE138]=posvalue_138;			
				huadu[huadu_temp_obu_num][APOS_VALUE999]=posvalue_999;
			}
			count  = count+col;
		}//while	
		/*����obu��ͬ�������Ѿ��ɼ���*/													
		/*ɾ�����ݿ��Ӧobu*/
		sqlite3_free_table(result);
    	printf("�ȴ�10 ����\n");/*�޸ĵĵط�*/
		sleep(10);		
	}
    //�ر����ݿ�  
    sqlite3_close(db);  
    db = 0;      
    return 0;  
}

/*�Ӵ��ڶ��̶����ȵ���Ϣ*/
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
	return (n-nleft);/*����ѭ��nleft������*/
}


unsigned char checkcode_crc8(const unsigned char *ptr,int len)//CRCУ��
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


/*��֤7e ��Ȼ�����������У��*/
ssize_t nread(int fd,unsigned char *ptr)
{
	int ret_sql = 0;
	ssize_t len;/*�������ĳ���*/
	ssize_t datalen;
	ssize_t p;/*����һ��������Ŷ���������*/
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

	if((len = xread(fd,ptr,datalen))<0)  /*xread ����������� ��Ҫ���*/
	{
		printf("read error\n");
	}
	if(ptr[0] == 0x01)
	{
		//OBU��Ϣ֡ д����־
		memset(data_from_uart,0,100);
		memset(uart_convet,0,200);
		data_from_uart[0] = 0x7e;
		data_from_uart[1] = p;
		memcpy(data_from_uart+2,ptr,len);/*����У��λ*/
		convet(data_from_uart, uart_convet, p);/*uart_convet����������ascii*/
		//ת����ascii���һλ�����λ
		memcpy(obu_info,"OBU:",4);
		memcpy(obu_info+4,uart_convet+6,16);				
		memcpy(obu_info+20,";CardID:",8);
		memcpy(obu_info+28,uart_convet+22,16);
		memcpy(obu_info+44,";Other info:",12);
		memcpy(obu_info+56,uart_convet+38,8);
		writeFile(fop_log, obu_info, 15);
		printf("����obu��Ϣ֡:%s\n",obu_info);
		int last_obu=0;// obu���һλ
		last_obu=ptr[8];	
		
		/***����ǰ���λ�ã����Ͱ���λ�ø�������***/
		if(floor_value==9)
		{
			unsigned char obu_buff[8];
			time_t timep;
			time(&timep);	
			char anjian_obu[17];
			memcpy(anjian_obu,uart_convet+6,16);
			writeFile(fop_log, anjian_obu,14);
			writeFile(fop_log, NULL,16);
			/*����*/
			f_head=0xf2;/*֡ͷ*/
			data_attr=0x8888;/*����*/
			data_num++;/*��ˮ��*/
			dev_id = real_device_id;/*�豸id*/
			data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/				
			f_end=0xf1;
			if(data_num==65535)
			{
				data_num=0x01;
			}
			/*��װͷ*/
			unsigned char pos_buff[50];
			memset(pos_buff,0,50);
			pos_buff[0]=*((unsigned char *)&f_head);
			pos_buff[1]=*((unsigned char *)&data_attr+1);/*֡���ͣ���д���ֽ�*/
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
			/*��ʼд��ʱ���ĸ��ֽ�*/
			pos_buff[26]=*((unsigned char *)&timep+3);
			pos_buff[27]=*((unsigned char *)&timep+2);
			pos_buff[28]=*((unsigned char *)&timep+1);
			pos_buff[29]=*((unsigned char *)&timep);
			
			/*У����2���ֽ�ͨ���ոշ����������ݼ����Ƚ���У��*/	
			data_crc=crc16(pos_buff,30);
			pos_buff[30]=*((unsigned char *)&data_crc+1);
			pos_buff[31]=*((unsigned char *)&data_crc);
			pos_buff[32]=*((unsigned char *)&f_end);
			
			/*װ������*/
			ENQueue(MyQueue,pos_buff,33);
			int key=0;
			for(key=0;key<33;key++)
				printf("%x ",pos_buff[key]);
		}

		/***�������¥�����жϽ���վ***/
		if(floor_value==4)
		{
			char coach_char[17]={0};
			memcpy(coach_char,uart_convet+6,16);
			int i=0;
			int coach_temp_num=0;
			int couch_in_flag=0;
			int in_flag=0;
			for(i=0;i<10;i++)
			{
				if(last_obu==coach_obu[i][7])
				{
					coach_temp_num=i;
					in_flag=1;
					break;
				}
			}
			printf("in_flag=%d\n",in_flag);
			if(coach_temp_num==0&&in_flag==0)
			{
				if(couch_num==9)
					couch_num=0;
				else couch_num++;
				coach_temp_num=couch_num;
				memcpy(coach_obu[coach_temp_num],ptr+1,8);
				coach_flag[coach_temp_num]=0x401;
				couch_in_flag=1;
			}
			coach_time[coach_temp_num]=time(NULL);
			if(couch_in_flag==1)
			{			
				writeFile(fop_log, coach_char,14);
				writeFile(fop_log, NULL,18);
				time_t timep;
				time(&timep);	
				/*����*/
				f_head=0xf2;/*֡ͷ*/
				data_attr=0x8888;/*����*/
				data_num++;/*��ˮ��*/
				dev_id = real_device_id;/*�豸id*/
				data_rec=0xc0a80002;  /*���շ���ip��ַ192.168.0.2*/				
				f_end=0xf1;
				if(data_num==65535)
				{
					data_num=0x01;
				}
				/*��װͷ*/
				unsigned char pos_buff[50];
				memset(pos_buff,0,50);
				pos_buff[0]=*((unsigned char *)&f_head);
				pos_buff[1]=*((unsigned char *)&data_attr+1);/*֡���ͣ���д���ֽ�*/
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
				memcpy(pos_buff+14,coach_obu[coach_temp_num],8);
				pos_buff[22]=0x00;
				pos_buff[23]=0x00;
				pos_buff[24]=0x04;
				pos_buff[25]=0x01;			
				/*��ʼд��ʱ���ĸ��ֽ�*/
				pos_buff[26]=*((unsigned char *)&timep+3);
				pos_buff[27]=*((unsigned char *)&timep+2);
				pos_buff[28]=*((unsigned char *)&timep+1);
				pos_buff[29]=*((unsigned char *)&timep);
				
				/*У����2���ֽ�ͨ���ոշ����������ݼ����Ƚ���У��*/	
				data_crc=crc16(pos_buff,30);
				pos_buff[30]=*((unsigned char *)&data_crc+1);
				pos_buff[31]=*((unsigned char *)&data_crc);
				pos_buff[32]=*((unsigned char *)&f_end);
				
				/*װ������*/
				ENQueue(MyQueue,pos_buff,33);
				int key=0;
				for(key=0;key<33;key++)
					printf("%x ",pos_buff[key]);
			}

		}
	}
	/*�����ӡ����*/      /*��ӡrssi��Ϣ*/
	if(ptr[0] == 0x04)
	{		
		memset(data_from_uart,0,100);
		memset(uart_convet,0,200);
		data_from_uart[0] = 0x7e;
		data_from_uart[1] = p;
		memcpy(data_from_uart+2,ptr,len);/*����У��λ*/
		route1_id = data_from_uart[18];/*sql*/
		route2_id = data_from_uart[27];/*sql*/
		route3_id = data_from_uart[36];/*sql*/
		route4_id = data_from_uart[45];/*sql*/
		
		route1_val = data_from_uart[19];/*sql*/
		route2_val = data_from_uart[28];/*sql*/
		route3_val = data_from_uart[37];/*sql*/
		route4_val = data_from_uart[46];/*sql*/

		convet(data_from_uart, uart_convet, p);/*uart_convet����������ascii*/
		//ת����ascii���һλ�����λ
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
		printf("����rssi ��Ϣ!\n");		
		printf("**************************************rssi=%s\n",rssi);
		
		int last_obu=0;// obu���һλ
		last_obu=ptr[8];
		if(floor_value==4)
		{
			int i=0;
			int coach_temp_num=0;
			for(i=0;i<10;i++)
			{
				if(last_obu==coach_obu[i][7])
				{
					coach_temp_num=i;
					break;
				}
			}
			coach_time[coach_temp_num]=time(NULL);			
		}
		
	}/*end of if*/
	return len-1;/*len-1�����������ݳ��� ��ȥУ��λ*/
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
	

	newtio.c_cc[VTIME] = 1;/*ָ����ȡ��һ���ַ��ĵȴ�ʱ��*/
	newtio.c_cc[VMIN] = 0;/*ָ����Ҫ��ȡ�ַ�����С����*/
	tcflush(uart_fd,TCIFLUSH);
	if(tcsetattr(uart_fd, TCSANOW, &newtio) != 0)
	{
		perror("tcsetaddr uart ");
	}	
	//printf("success!\n");

}



/*�й���־���ܵ��Ӻ���*/
/*��ȡ��ǰϵͳ��ʱ�� */
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
    /*��ʱ��������*/
   /*׷��.log*/
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
		ret = fprintf(fp, "[%s]:{�������ͨ��}send: %s\n", curTime, str);
		break;
	case 2:
		ret = fprintf(fp, "[%s]:{�������ͨ��}recv: %s\n", curTime, str);
		break;
	case 3:
		 ret = fprintf(fp, "[%s]:{�밲ȫģ��ͨ��}send: %s\n", curTime, str);
		break;
	case 4:
		 ret = fprintf(fp, "[%s]:{�밲ȫģ��ͨ��}recv: %s\n", curTime, str);
		break;
	case 5:
		ret = fprintf(fp, "[%s]:{����ʧ��}error: %s\n", curTime, str);
		break;
	case 6:
		ret = fprintf(fp, "[%s]:{�������ļ�ʧ��}error: %s\n", curTime, str);
		break;
	case 7:
		ret = fprintf(fp, "        {RSSI ��Ϣ����}: \n");
		break;
	case 8:
		ret = fprintf(fp, "[%s]: rssi:%s\n", curTime,str);
		break;
	case 9:
		ret = fprintf(fp, "{˾����Ϣ:}: %s\n", str);
		break;
	case 10:
		ret = fprintf(fp, "[%s]:{taxier ������Ϣ}: %s\n", curTime, str);
		break;
	case 11:
		ret = fprintf(fp, "  {��վ!}\n");
		break;
	case 12:
		ret = fprintf(fp, "  {��վ!}\n");
		break;	
	case 13:
		ret = fprintf(fp, "  POS=%s\n",str);
		break;	
	case 14:
		ret = fprintf(fp, "[%s]:OBU=%s  ",curTime,str);
		break;	
	case 15:
		ret = fprintf(fp, "[%s]:OBU��Ϣ֡ %s\n",curTime,str);
		break;	
	case 16:
		ret = fprintf(fp, "  {�������������!}\n");
		break;
	case 17:
		ret = fprintf(fp, "  {�����Ĳ��ս!}\n");
		break;
	case 18:
		ret = fprintf(fp, "  {�����Ĳ��վ!}\n");
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
       unsigned char temp;  /*�����unsigned �͵ķ�������*/
        temp = src[i]&0xf0;
        temp = temp >> 4;      
       if((temp>9)&&(temp<16))
        {
            temp = temp+0x37;
        }
        else{
            temp = temp+0x30;
        }
        dest[2*i] =temp;/*��Ҫ��*��*/
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

/*�������ļ���صĺ�������*/
/*
 *ȥ���ַ����Ҷ˿ո�
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
 *ȥ���ַ�����˿ո�
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
 *ȥ���ַ������˿ո�
 */
char *strtrim(char *pstr)
{
    char *p;
    p = strtrimr(pstr);
    return strtriml(p);
}


/*
 *�������ļ���һ�ж���key��value,����itemָ��
 *line--�������ļ�������һ��
 */
int  get_item_from_line(char *line,  ITEM *item)
{
    char *p = strtrim(line);
    int len = strlen(p);
    if(len <= 0){
        return 1;//����
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
    return 0;//��ѯ�ɹ�
}

/*
 *��ȡvalue
 */
int read_conf_value(const char *key, char *value,const char *file)
{
    char line[1024];
    FILE *fp;
    fp = fopen(file,"r");
    if(fp == NULL)
        return 1;//�ļ��򿪴���    
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
    return 0;//�ɹ�

}
/*�ַ���ת16����*/
unsigned long convert_atohex(char* str)
{
 unsigned long var=0;
 unsigned long t;
 int len = strlen(str);

 if (len > 8) //�8λ
  return -1;
// strupr(str);//ͳһ��д
 for (; *str; str++)
 {
  if (*str>='A' && *str <='F')
   t = *str-55;//a-f֮���ascii���Ӧ��ֵ���55��'A'Ϊ65,65-55��ΪA
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
	   t = *str-55;//a-f֮���ascii���Ӧ��ֵ���55��'A'Ϊ65,65-55��ΪA
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

/*���ݿ�����Ļص�����*/
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
	printf("û��ȡ������\n");

	return 0;
}
printf("����ص�����\n");
  // ÿ����¼�ص�һ�θú���,�ж������ͻص����ٴ�
  int i;
  char buff_send_local[13];
  char buff_obu[8];
  int pos = 0;
  int k=0;
  int ret_sql = 0;
  time_t time_local;/*�ϴ�λ����Ϣ��ʱ��*/
  int serial_num_to_del = 0;
 
  data_rec=0xc0a80002; 

  serial_num_to_del = convert_atohex(col_values[0]);
  buff_send_local[0] = 0x20;
  convert_atohex1(col_values[1], buff_obu);
  memcpy(buff_send_local+1,buff_obu,8);
  pos = convert_atohex(col_values[2]);	
  //pos=(pos%16)+(pos>>4)%16*10+(pos>>8)%16*100;
  printf("pos = 0x%x*************����~~~~!!!!******************************* \n",pos);
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
        sqlite3_free(pErrMsg); //���Ҫ��Ŷ��Ҫ��Ȼ���ڴ�й¶��Ŷ������  
        sqlite3_close(db);  
        return 1;  
    }  
    printf("���ݿ�ɾ�����ݳɹ���\n");  

  return 0;
}



/*һ��locate�㷨��غ���
src���tλ��dest���tλ�Ա�
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
//			printf("obu�ı�\n");
//			printf("obu[lenobu] = %c \n",obu[lenobu]);
//			printf("*dest1 = %c \n",*dest1);
			return false;
			}
		lenobu--;
		dest1--;
	}
//	printf("obu һ��û�иı�\n");
	return true;

}

/*****************************************************************
�������鼰·�ɱ��
���ض�Ӧ���ź�ǿ��
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
�������ڱȽϵĶ�ά�����Լ�Ҫ���ֵ��·�ɱ��
�������ֵ����Сֵ�Ĳ�
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
	//else return 0xff;               //��ʾ���ݲ����á�����������ټ�
}
/*****************************************************************
����һ֡���ݣ��ҵ�����Rssi
�������ֵ����·�ɵı��
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
����һ��·�ɱ�ţ��ҵ��������ֵĴ���
�����������ֵĴ���
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
����һ����ά�����ָ��·�ɱ��
����ָ��·�����ϵĵ�ƽ��ֵ
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





