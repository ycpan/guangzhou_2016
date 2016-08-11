/*2016-03-14 客运四楼版*/
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
#define P101 0    //定义每个路由在数组中的位置11
#define P102 1	//12
#define P103 2	//13
#define P104 3	//14
#define P105 4	//15
#define P106 5	//16
#define P107 6	//17
#define P108 7	//18	
#define P109 8	//19
#define P110 9
#define P111 10
#define P112 11
#define P131 12
#define P132 13
#define P133 14
#define P134 15
#define P135 16
#define P136 17
#define P137 18
#define P138 19
#define P999 20
#define APOS_VALUE105			8	// 每个位置的权值
#define APOS_VALUE108			8
#define APOS_VALUE110			10
#define APOS_VALUE138			20	
#define APOS_VALUE999			9

#define acc 6  			//定位精度，收集的信号强度幀数
//大改添加
int huadulist[28]={0x0096,0x0078,0x0080,0x0053,0x0051,0x0066,0x0065,
					  0x0059,0x0092,0x0085,0x0058,0x0069,0x0077,0x0061,
					  0x0070,0x0067,0x0089,0x0060,0x00,0x00,0x00,
					  0x00,0x00,0x00,0x00,0x00,0x00,0x00};

int findRssi(int buff[11],int routeid);
int makesub(int tempbuff[acc][11],int routeID);                ////////if it ==0;    neeed debug
int findmaxRssiaddr(int buff[11]);
int findRsutimes(int tempbuff[acc][11],int routeID);
int getAverRssi(int tempbuff[acc][11],int routeID);
int getRssi(int num_in_buff,unsigned char src[]);

unsigned char getPos(int num_in_buff,unsigned char pos_old);
void send_position(unsigned char obu_buf[8],unsigned char pos[2]);


unsigned char coach_obu[10][8]={0};
unsigned char coach_rssi[10][8]={0};
unsigned char coach_posvalue[10][20]={0};
unsigned char coach_pos[10][2]={0};
int coach_flag[10]={0};
int couch_num=0;

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
int zhuanyi(char *src,char *dest,int length);

unsigned long convert_atohex(char* str);/*字符串转16进制*/
unsigned long convert_atohex1(char* str,char *buff_hex);
time_t first;/*获取初始的时间*/
time_t heartbeat_t;/*汇聚心跳包*/
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
bool getPosvalue(int num_in_buff,int num);

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






//FILE *file_csv;/*日志文件*/
float obu_renew_max_time=80.0;
float obu_renew_max_time_tmp=80.0;
float park_to_138_time=50.0;
float park_to_138_time_base=50.0;
float park_to_138_time_last=0.0;
float park_to_138_time_coach=0.0;
float delay_out_station=240.0;
int only_renew_obu_info=0;
time_t go_to_security_set_time=0;
int prev_analy_pos=0;
unsigned char send_position_input_obu[8]={0x00};
const char comp_source[5] = "0004";
char comp_target[5] ="0000";
struct single_pos
{
	int pos_pos;
	int pos_weight;
	unsigned char pos_obu[17];
	unsigned char pos_unchar_pos[2];
	time_t pos_time;
	time_t com_138_time;
};
struct all_pos_info
{
	struct single_pos pos_1;
	struct single_pos pos_105;
	struct single_pos pos_108;
	struct single_pos pos_109;
	struct single_pos pos_110;
	struct single_pos pos_138;
	struct single_pos pos_999;
};
struct all_pos_info all_pos={{1,0,"00 00",{0x00,0x01},0,0},{105,0,"00 00",{0x01,0x05},0,0},{108,0,"00 00",{0x01,0x08},0,0},{109,0,"00 00",{0x01,0x09},0,0},{110,0,"00 00",{0x01,0x10},0,0},{138,0,"00 00",{0x01,0x38},0,0},{999,0,"00 00",{0x09,0x99},0,0}};
struct struct_last_statu
{
        int last_pos;
        int pos_138_count;
        int pos_sum_set;
        int pos_park;
        int pos_park_count;
        unsigned char last_obu[17];
        time_t last_time;
        time_t com_station_last_time;
        time_t xiake_last_time;
        time_t wait_locate_time;
        float diff_max_time;
};
struct struct_last_statu last_statu[60]={{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0}, 
{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},
{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0},{0,0,0,0,0,"00 00",0,0,0,0,0.0}};

struct struct_huadu_to_138_tmp_pos 
{
	int tmp_pos;
	unsigned char tmp_obu[17];
	unsigned char tmp_unchar_pos[2];
	time_t tmp_time;
};
struct struct_huadu_to_138_tmp_pos huadu_to_138_tmp_pos={0,"00 00",{0x00,0x00},0};

char* substr_get_fun(const char*str,unsigned start, unsigned end)
{
   unsigned n = end - start;
   static char stbuf[256];
   strncpy(stbuf, str + start, n);
   stbuf[n] = 0;
   return stbuf;
}

char* time_get_fun() {
     time_t timer;
     struct tm *tblock;
     timer=time(NULL);
     tblock=localtime(&timer);
     return substr_get_fun(asctime(tblock),11, 19); 
} 


int tr_route(int a)
{
 int b,c;
 b=a;
 //c=b-0x47;
 //c=b-11;
 c=b-0x11;
 return c;
}
int tr_rssi(int a)
{
 int b,c;
 b=a;
 c=b-0x9e;
 return c;
}
int tr_pos(int a)
{
 int b,c;
 b=a;
 //if(b==1)
 //  c=0;
 //if(b >= 104 && b <= 112 ) 
   //c=b-104;
 //if(b==138)
  // c=9;
// if(b==999)
 //  c=11;
 switch(b)
 {
  case 105: 
  c = 0;
  break;
  case 108: 
  c = 1;
  break;
  case 109:
  c = 2;
  break;
  case 110: 
  c = 3;
  break;
  case 138:
  c = 4;
  break;
  default: 
  c = 0;
  break;
  }
 return c;
}


int tab(int x,int y,int z)
{
int row, column, table;
 row=tr_route(x);
 column=tr_rssi(y);
 table=tr_pos(z);


int pp;
int values[9][38][5] = {
{{6,7,5,1,7},
{7,8,6,2,8},
{8,9,7,3,9},
{9,10,8,4,10},
{10,11,9,5,11},
{11,12,10,6,12},
{12,13,11,7,13},
{13,14,12,8,14},
{14,15,13,9,15},
{15,16,14,10,16},
{16,17,15,11,17},
{17,18,16,12,18},
{18,19,17,13,19},
{19,20,18,14,20},
{20,21,19,15,21},
{19,22,20,16,22},
{18,23,21,17,23},
{17,24,22,18,24},
{16,23,23,19,25},
{15,24,24,20,26},
{14,25,25,21,27},
{13,26,26,22,28},
{12,27,25,23,29},
{11,28,24,24,28},
{10,29,23,25,27},
{9,28,22,26,26},
{8,27,21,27,25},
{7,26,20,28,24},
{6,25,19,27,23},
{5,24,18,26,22},
{4,23,17,25,21},
{3,22,16,24,20},
{2,21,15,23,19},
{1,20,14,22,18},
{0,19,13,37,37},
{0,18,12,37,37},
{0,17,11,37,37},
{0,16,10,37,37}},
{{10,3,0,12,11},
{11,4,0,13,12},
{12,5,0,14,13},
{13,6,0,15,14},
{14,7,0,16,15},
{15,8,0,17,16},
{16,9,1,18,17},
{17,10,2,19,18},
{18,11,3,18,19},
{19,12,4,17,18},
{18,13,5,16,17},
{17,14,6,15,16},
{16,13,7,14,15},
{15,12,8,13,14},
{14,11,9,12,13},
{13,10,10,11,12},
{12,9,11,10,11},
{11,14,12,9,10},
{10,15,13,8,9},
{9,16,14,7,8},
{8,17,15,6,7},
{7,18,16,5,6},
{6,19,17,4,5},
{5,18,18,3,4},
{4,17,19,2,3},
{3,16,18,1,2},
{2,15,17,0,1},
{1,14,16,0,0},
{0,13,15,0,0},
{0,12,14,0,0},
{0,11,13,0,0},
{0,10,12,0,0},
{0,9,11,0,0},
{0,8,10,0,0},
{0,7,9,0,0},
{0,6,8,0,0},
{0,5,7,0,0},
{0,4,6,0,0}},
{{8,10,11,14,0},
{9,11,12,15,16},
{10,12,13,16,17},
{11,13,14,17,18},
{12,14,15,18,19},
{13,15,16,19,18},
{14,16,17,18,17},
{15,17,18,17,16},
{16,18,19,16,15},
{17,19,18,15,14},
{18,18,17,14,13},
{19,17,16,13,12},
{18,16,15,12,11},
{17,15,14,11,10},
{16,14,13,10,9},
{15,13,12,9,8},
{14,12,11,8,7},
{13,11,10,7,6},
{12,10,9,6,5},
{11,9,8,5,4},
{10,8,7,4,3},
{9,7,6,3,2},
{8,6,5,2,1},
{7,5,4,1,0},
{6,4,3,0,0},
{5,3,2,0,0},
{4,2,1,0,0},
{3,1,0,0,0},
{2,0,0,0,0},
{1,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},
{{9,8,10,16,11},
{10,9,11,17,12},
{11,10,12,18,13},
{12,11,13,19,14},
{13,12,14,18,15},
{14,13,15,17,16},
{15,14,16,16,17},
{16,15,17,15,18},
{17,16,18,14,19},
{18,17,19,13,18},
{19,18,18,12,17},
{20,19,17,11,16},
{21,20,16,10,15},
{22,21,15,9,14},
{23,22,14,8,13},
{24,21,13,7,12},
{25,19,12,6,11},
{26,18,11,5,10},
{25,17,10,4,9},
{24,16,9,3,8},
{23,15,8,2,7},
{22,14,7,1,6},
{21,13,6,0,5},
{20,12,5,0,4},
{19,11,4,0,3},
{18,10,3,0,2},
{17,9,2,0,1},
{16,8,1,0,0},
{15,7,0,0,0},
{14,6,0,0,0},
{13,5,0,0,0},
{12,4,0,0,0},
{11,3,0,0,0},
{10,2,0,0,0},
{9,1,0,0,0},
{8,0,0,0,0},
{7,0,0,0,0},
{6,0,0,0,0}},
{{17,15,11,13,13},
{18,16,12,14,14},
{19,17,13,15,15},
{18,18,14,16,16},
{17,19,15,17,17},
{16,18,16,18,18},
{15,17,17,19,19},
{14,16,18,18,18},
{13,15,19,17,17},
{12,14,18,16,16},
{11,13,17,15,15},
{10,12,16,14,14},
{9,11,15,13,13},
{8,10,14,12,12},
{7,9,13,11,11},
{6,8,12,10,10},
{5,7,11,9,9},
{4,6,10,8,8},
{3,5,9,7,7},
{2,4,8,6,6},
{1,3,7,5,5},
{0,2,6,4,4},
{0,1,5,3,3},
{0,0,4,2,2},
{0,0,3,1,1},
{0,0,2,0,0},
{0,0,1,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},
{{9,9,4,10,4},
{10,10,5,11,5},
{11,11,6,12,6},
{12,12,7,13,7},
{13,13,8,14,8},
{14,14,9,15,9},
{15,15,10,16,10},
{16,16,11,17,11},
{17,17,12,18,12},
{18,18,13,19,13},
{19,19,14,20,14},
{20,20,15,21,15},
{21,21,16,22,16},
{22,22,17,23,17},
{23,23,18,24,18},
{24,24,19,23,19},
{25,25,20,22,20},
{24,26,21,21,21},
{23,27,22,20,22},
{22,28,23,19,23},
{21,29,24,18,24},
{20,30,25,17,23},
{19,29,26,16,22},
{18,28,25,15,21},
{17,27,24,14,20},
{16,26,23,13,19},
{15,25,22,12,18},
{14,24,21,11,17},
{13,23,20,10,16},
{12,22,19,9,15},
{11,21,18,8,14},
{10,20,17,7,13},
{9,19,16,6,12},
{8,18,15,5,11},
{7,17,14,4,10},
{6,16,13,3,9},
{5,15,12,2,8},
{4,14,11,1,7}},
{{16,10,14,16,12},
{17,11,15,17,13},
{18,12,16,18,14},
{19,13,17,19,15},
{20,14,18,18,16},
{21,15,19,17,17},
{22,16,18,16,18},
{23,17,17,15,19},
{22,18,16,14,18},
{21,19,15,13,17},
{20,18,14,12,16},
{19,17,13,11,15},
{18,16,12,10,14},
{17,15,11,9,13},
{16,14,10,8,12},
{15,13,9,7,11},
{14,12,8,6,10},
{13,11,7,5,9},
{12,10,6,4,8},
{11,9,5,3,7},
{10,8,4,2,6},
{9,7,3,1,5},
{8,6,2,0,4},
{7,5,1,0,3},
{6,4,0,0,2},
{5,3,0,0,1},
{4,2,0,0,0},
{3,1,0,0,0},
{2,0,0,0,0},
{1,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},
{{13,9,13,13,17},
{14,10,14,12,18},
{15,11,15,11,19},
{16,12,16,10,18},
{17,13,17,9,17},
{18,14,18,8,16},
{19,15,19,7,15},
{20,16,18,6,14},
{21,17,17,5,13},
{22,18,16,4,12},
{23,19,15,3,11},
{22,18,14,2,10},
{21,17,13,1,9},
{20,16,12,0,8},
{19,15,11,0,7},
{18,14,10,0,6},
{17,13,9,0,5},
{16,12,8,0,4},
{15,11,7,0,3},
{14,10,6,0,2},
{13,9,5,0,1},
{12,8,4,0,0},
{11,7,3,0,0},
{10,6,2,0,0},
{9,5,1,0,0},
{8,4,0,0,0},
{7,3,0,0,0},
{6,2,0,0,0},
{5,1,0,0,0},
{4,0,0,0,0},
{3,0,0,0,0},
{2,0,0,0,0},
{1,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0},
{0,0,0,0,0}},
{{12,10,7,11,0},
{13,11,8,12,17},
{14,12,9,13,18},
{15,13,10,14,19},
{16,14,11,15,18},
{17,15,12,16,17},
{18,16,13,17,16},
{19,17,14,18,15},
{20,18,15,19,14},
{21,19,16,18,13},
{22,18,17,17,12},
{23,17,18,16,11},
{24,16,19,15,10},
{25,15,18,14,9},
{26,14,17,13,8},
{25,13,16,12,7},
{24,12,15,11,6},
{23,11,14,10,5},
{22,10,13,9,4},
{21,9,12,8,3},
{20,8,11,7,2},
{19,7,10,6,1},
{18,6,9,5,0},
{17,5,8,4,0},
{16,4,7,3,0},
{15,3,6,2,0},
{14,2,5,1,0},
{13,1,4,0,0},
{12,0,3,0,0},
{11,0,2,0,0},
{10,0,1,0,0},
{9,0,0,0,0},
{8,0,0,0,0},
{7,0,0,0,0},
{6,0,0,0,0},
{5,0,0,0,0},
{4,0,0,0,0},
{3,0,0,0,0}}
};


pp=values[row][column][table];

return pp;
}
int string_to_integer(unsigned char string_to_integer_input[17] ,unsigned char string_to_integer_output[8])
{
	
	unsigned char dest_obu[8]={0x00};
	
	int i,j;
	j=0;
	unsigned char tmp;
	for(i=0;i<16;i++)
	{
		tmp=string_to_integer_input[i];
		
		if(tmp>=48&&tmp<=57)
		{
			tmp=tmp-48;
		}
		if(tmp>=97&&tmp<=102)
		{
			tmp=tmp-87;
		}
		if(tmp>=65&&tmp<=70)
		{
			tmp=tmp-55;
		}
				
		if(2*j==i)
		{	
			dest_obu[j]=tmp;
			dest_obu[j]=dest_obu[j]<<4;
		}
		if((2*j+1)==i)
		{
			dest_obu[j]=dest_obu[j]|tmp;
			
			j++;
			
		}
	}
	memcpy(string_to_integer_output,dest_obu,8);
	return 0;
	
}

int pos_set(int poss_pos,int poss_weight,char *poss_obu)
{

	int pos_set_pos;
	char *pos_set_obu;
	int pos_set_weight;
	pos_set_pos=poss_pos;
	pos_set_obu=poss_obu;
	pos_set_weight=poss_weight;
	
	
	
		
	switch(pos_set_pos)
	{
		case 1:
		memcpy(all_pos.pos_1.pos_obu,pos_set_obu,17);
		all_pos.pos_1.pos_weight=pos_set_weight;
		all_pos.pos_1.pos_time=time(NULL);
		break;
		
		case 105:
		memcpy(all_pos.pos_105.pos_obu,pos_set_obu,17);
		all_pos.pos_105.pos_weight=pos_set_weight;
		all_pos.pos_105.pos_time=time(NULL);
		break;
		
		case 108:
                	memcpy(all_pos.pos_108.pos_obu,pos_set_obu,17);
		all_pos.pos_108.pos_weight=pos_set_weight;
		all_pos.pos_108.pos_time=time(NULL);
		break;
		
		case 109:
                	memcpy(all_pos.pos_109.pos_obu,pos_set_obu,17);
		all_pos.pos_109.pos_weight=pos_set_weight;
		all_pos.pos_109.pos_time=time(NULL);
		break;
               	
               	case 110:
                	memcpy(all_pos.pos_110.pos_obu,pos_set_obu,17);
		all_pos.pos_110.pos_weight=pos_set_weight;
		all_pos.pos_110.pos_time=time(NULL);
		break;
		
		case 138:
                	memcpy(all_pos.pos_138.pos_obu,pos_set_obu,17);
		all_pos.pos_138.pos_weight=pos_set_weight;
		all_pos.pos_138.pos_time=time(NULL);
		break;
		
		case 999:
		memcpy(all_pos.pos_999.pos_obu,pos_set_obu,17);
		all_pos.pos_999.pos_weight=pos_set_weight;
		all_pos.pos_999.pos_time=time(NULL);
		break;
		
		default:
		printf("the possible position:%d is innormal in function pos_set",pos_set_pos);
		return -1;
		
	}
	
	return 0;
}

int verify_last_statu(char *verify_last_statu_obu)
{

	int last_statu_i;
	char *last_statu_obu;
	last_statu_obu=verify_last_statu_obu;
	
	for(last_statu_i=0;last_statu_i<60;last_statu_i++)
	{
             
		
		if(strcmp(last_statu[last_statu_i].last_obu,last_statu_obu)==0)
		{
			
			return last_statu[last_statu_i].last_pos;

		}
	}
	
	return 0;
}

void preve_repeat_location(int prev_poss_pos,int *prev_poss_weight,char *prev_poss_obu)
{
	
	int preve_repeat_pos;
	preve_repeat_pos=prev_poss_pos;

	switch(preve_repeat_pos)
	{
		
		case 105:
		*prev_poss_weight=all_pos.pos_105.pos_weight;
		memcpy(prev_poss_obu,all_pos.pos_105.pos_obu,17);
		break;
		
		case 108:
		*prev_poss_weight=all_pos.pos_108.pos_weight;
		memcpy(prev_poss_obu,all_pos.pos_108.pos_obu,17);
                	break;
               	
               	case 109:
		*prev_poss_weight=all_pos.pos_109.pos_weight;
		memcpy(prev_poss_obu,all_pos.pos_109.pos_obu,17);
               	break;
               	
               	case 110:
		*prev_poss_weight=all_pos.pos_110.pos_weight;
		memcpy(prev_poss_obu,all_pos.pos_110.pos_obu,17);
               	break;
		
		default:
		printf("preve_repeat_location happend error ,input pos is %d,not specify",preve_repeat_pos);
		
	}
		
}

int clear_obu_last_statu(char *clear_obu_last_obu)
{
	int clear_obu_i;
	char *clear_obu;
	clear_obu=clear_obu_last_obu;
	for(clear_obu_i=0;clear_obu_i<60;clear_obu_i++)	
	{
		
		if(strcmp(clear_obu,last_statu[clear_obu_i].last_obu)==0)
		{
			last_statu[clear_obu_i].last_pos=0;
			
			sprintf(last_statu[clear_obu_i].last_obu,"00 00");
			
		}
	}
	
	return 0;
}

/******************
func:verify_set_position
input:weight,pos,obu
output:0,this postion process have done .-1,this postion process not done;
range:105-110
*******************/
int verify_set_position(int verify_set_position_weight,int verify_set_position_pos,char *verify_set_position_obu)
{
	int verify_pos_weight;
	int verify_pos_pos;
	char *verify_pos_obu;
	int return_repeat_weight=0;
	
	char return_repeat_obu[17]="00 00";
	verify_pos_weight=verify_set_position_weight;
	verify_pos_pos=verify_set_position_pos;
	verify_pos_obu=verify_set_position_obu;
	if(strcmp(huadu_to_138_tmp_pos.tmp_obu,"00 00")!=0&&huadu_to_138_tmp_pos.tmp_pos==verify_pos_pos)
	{
		return -1;
	}
	
	if(verify_pos_pos>103 && verify_pos_pos<112)
	{	
		preve_repeat_location(verify_pos_pos,&return_repeat_weight,return_repeat_obu);
		
		if(strcmp(return_repeat_obu,"00 00")==0)
		{

			
			
			if(pos_set(verify_pos_pos,verify_pos_weight,verify_pos_obu)==0)
			{
				
				write_last_data(verify_pos_obu,verify_pos_pos);
				return 0;
			}
			else
			{
				printf("\n\n\n ERROR happend in pos_set function,verify_pos_pos:%d,verify_pos_weight:%d,verify_pos_obu:%s\n\n\n",verify_pos_pos,verify_pos_weight,verify_pos_obu);
				return -2;//set value,avoid return null,self killed.-2 reponse do nothing
			}
			
		}
		else
		{
			
			if(return_repeat_weight<verify_pos_weight)
			{
				pos_set(verify_pos_pos,verify_pos_weight,verify_pos_obu);//写新obu信息，包括all_pos ,last_statu.
				write_last_data(verify_pos_obu,verify_pos_pos);
				write_last_data(return_repeat_obu,-1);//重置last_statu状态
				return 0;	
			}
		
		
			else
			{
				
				return -1;
				
			}
		}	
	}
	else if(verify_pos_pos==138)
	{
		
		return -1;
	}
	
}

int avoid_can_not_local_105_to_110(char *avoid_can_not_local_105_to_110_obu)
{
	
	if(strcmp(all_pos.pos_105.pos_obu,"00 00")==0)
	{
		printf("avoid_can_not_local_105_to_110,locat pos is 105\n");
		pos_set(105,1,avoid_can_not_local_105_to_110_obu);//写新obu信息，包括all_pos ,last_statu.
		write_last_data(avoid_can_not_local_105_to_110_obu,105);
		return 0;
	}
	if(strcmp(all_pos.pos_108.pos_obu,"00 00")==0)
	{
		printf("avoid_can_not_local_105_to_110,locat pos is 108\n");
		pos_set(108,1,avoid_can_not_local_105_to_110_obu);//写新obu信息，包括all_pos ,last_statu.
		write_last_data(avoid_can_not_local_105_to_110_obu,108);
		return 0;
	}
	if(strcmp(all_pos.pos_109.pos_obu,"00 00")==0)
	{
		printf("avoid_can_not_local_105_to_110,locat pos is 109\n");
		pos_set(109,1,avoid_can_not_local_105_to_110_obu);//写新obu信息，包括all_pos ,last_statu.
		write_last_data(avoid_can_not_local_105_to_110_obu,109);
		return 0;
	}
	if(strcmp(all_pos.pos_105.pos_obu,"00 00")==0)
	{
		printf("avoid_can_not_local_105_to_110,locat pos is 110\n");
		pos_set(110,1,avoid_can_not_local_105_to_110_obu);//写新obu信息，包括all_pos ,last_statu.
		write_last_data(avoid_can_not_local_105_to_110_obu,110);
		return 0;
	}
	return -1;

}


void locat_105_to_110_position(int locat_105_to_110_weight_0,int locat_105_to_110_pos_0,int locat_105_to_110_weight_1,int locat_105_to_110_pos_1,int locat_105_to_110_weight_2,int locat_105_to_110_pos_2,int locat_105_to_110_weight_3,int locat_105_to_110_pos_3,char *locat_105_to_110_obu)
{

	
	int ver_pos_res;
	int locat_weight_0,locat_weight_1,locat_weight_2,locat_weight_3;
	int locat_pos_0,locat_pos_1,locat_pos_2,locat_pos_3;
	char *locat_obu;
	locat_weight_0=locat_105_to_110_weight_0;
	locat_pos_0=locat_105_to_110_pos_0;
	locat_weight_1=locat_105_to_110_weight_1;
	locat_pos_1=locat_105_to_110_pos_1;
	locat_weight_2=locat_105_to_110_weight_2;
	locat_pos_2=locat_105_to_110_pos_2;
	locat_weight_3=locat_105_to_110_weight_3;
	locat_pos_3=locat_105_to_110_pos_3;
	locat_obu=locat_105_to_110_obu;
		
	ver_pos_res=verify_set_position(locat_weight_0,locat_pos_0,locat_obu);
	
	if(ver_pos_res==0)
	{
		
		ver_pos_res=1;
		
	}
	else if(ver_pos_res==-1)
	{
		printf("  1  possible position %d 已经被占用，正在计算新的位置。。。\n",locat_pos_0);
		
		ver_pos_res=verify_set_position(locat_weight_1,locat_pos_1,locat_obu);
		
	}
	if(ver_pos_res==0)
	{
		
		ver_pos_res=1;
		
	}
	else if(ver_pos_res==-1) 
	{
		printf(" 2  possible position %d 已经被占用，正在计算新的位置。。。\n",locat_pos_1);
		
		ver_pos_res=verify_set_position(locat_weight_2,locat_pos_2,locat_obu);
		
	}
	if(ver_pos_res==0)
	{
		
		ver_pos_res=1;
		
	}
	else if(ver_pos_res==-1)
	{
		printf(" 3  possible position %d 已经被占用，正在计算新的位置。。。\n",locat_pos_2);
		
		ver_pos_res=verify_set_position(locat_weight_3,locat_pos_3,locat_obu);
		
	}
	if(ver_pos_res==0)
	{
		
		ver_pos_res=1;
		
	}
	else if(ver_pos_res==-1)
	{
		printf(" 4  possible position %d 已经被占用，正在计算新的位置。。。\n",locat_pos_3);
		ver_pos_res=avoid_can_not_local_105_to_110(locat_obu);
		//printf(" 很遗憾，车太多，车位太少，无法定位OBU:%s\n",locat_obu);
		
		
	}
	if(ver_pos_res==-1)
	{
		printf(" 很遗憾，车太多，车位太少，无法定位OBU:%s\n",locat_obu);
		
	}
	
	
		
}

int clean_prev_105_to_110_pos(int clean_prev_pos)
{
	int clean_pos;
	clean_pos=clean_prev_pos;
	switch(clean_pos)
	{
		case 105:
		sprintf(all_pos.pos_105.pos_obu,"00 00");
		all_pos.pos_105.pos_weight=0;
		return 1;
		
		case 108:
		sprintf(all_pos.pos_108.pos_obu,"00 00");
		all_pos.pos_108.pos_weight=0;
		return 1;
		
		case 109:
		sprintf(all_pos.pos_109.pos_obu,"00 00");
		all_pos.pos_109.pos_weight=0;
		return 1;
		
		case 110:
		sprintf(all_pos.pos_110.pos_obu,"00 00");
		all_pos.pos_110.pos_weight=0;
		return 1;
		
		default :
		return 0;
	}
}

int verify_last_obu(char *para_verify_obu,int *null_space_number)
{
	char *verify_obu;
	int ver_i=0,ver_j=0;
	verify_obu=para_verify_obu;
	for(ver_i=0;ver_i<60;ver_i++)
	{
		
		if(strcmp(last_statu[ver_i].last_obu,verify_obu)==0)
		{
			return ver_i;
				
		}
		
		if(strcmp(last_statu[ver_i].last_obu,"00 00")==0)
		{
			ver_j=ver_i;	
		}
		
	}
	*null_space_number=ver_j;
	
	return -1;
}
void renew_diff_time_last_obu(char *renew_diff_time_para_verify_obu)
{
	char *verify_obu;
	int ver_i=0,ver_j=0;
	verify_obu=renew_diff_time_para_verify_obu;
	for(ver_i=0;ver_i<60;ver_i++)
	{
		
		if(strcmp(last_statu[ver_i].last_obu,verify_obu)==0)
		{
			//float diff_time_comp1=0.0;
			float diff_time_comp=difftime(time(NULL),last_statu[ver_i].xiake_last_time);
			last_statu[ver_i].xiake_last_time=time(NULL);
			//float diff_time=diff_time_comp2-diff_time_comp1;
			printf("the diff_time_comp is %f\n",last_statu[ver_i].diff_max_time);
			if((diff_time_comp>0.0&&diff_time_comp<179.0)&&diff_time_comp >last_statu[ver_i].diff_max_time)
			{
				last_statu[ver_i].diff_max_time=diff_time_comp;
				//diff_time_comp1=diff_time_comp2;
				printf("the %s diff_max time  RENEW is %f \n",last_statu[ver_i].last_obu,last_statu[ver_i].diff_max_time);
			}
				
		}
	}
}
int write_last_data(char *poss_last_obu,int poss_last_pos)
{
	int write_last_data_i=0,write_last_data_j=0;
	int last_pos;
	char *last_obu;
	last_obu=poss_last_obu;
	last_pos=poss_last_pos;
	write_last_data_i=verify_last_obu(last_obu,&write_last_data_j);
	
	if(write_last_data_i!=-1)
	{	
		memcpy(last_statu[write_last_data_i].last_obu,last_obu,17);
		last_statu[write_last_data_i].last_pos=last_pos;
		// if(last_pos==-1)
		// {
		// 	last_statu[write_last_data_i].wait_locate_time=time(NULL);
		// }
		//float diff_time=30.0;
		float diff_time=difftime(time(NULL),last_statu[write_last_data_i].last_time);
		last_statu[write_last_data_i].last_time=time(NULL);
		//printf("the diff_time is %f\n",diff_time);
		//printf("the last_statu[write_last_data_i].diff_max_time is %f\n",last_statu[write_last_data_i].diff_max_time);
		if(last_statu[write_last_data_i].last_pos==138&&(diff_time>0.0&&diff_time<179.0)&&diff_time >last_statu[write_last_data_i].diff_max_time)
		{
			last_statu[write_last_data_i].diff_max_time=diff_time;
			printf("the %s diff_max time  RENEW is %f \n",last_statu[write_last_data_i].last_obu,last_statu[write_last_data_i].diff_max_time);
		}
		
	}			
	else
	{	
		memcpy(last_statu[write_last_data_j].last_obu,last_obu,17);
		last_statu[write_last_data_j].last_pos=last_pos;
		last_statu[write_last_data_j].last_time=time(NULL);
		last_statu[write_last_data_j].com_station_last_time=time(NULL);//为空，刚进站，这是进站的时间，不会被刷新
		last_statu[write_last_data_i].xiake_last_time=time(NULL);
		last_statu[write_last_data_i].diff_max_time=0.0;
		
	}
	
		
					
}

int analy_pos_result(char *analy_pos_obu, int pos1,int pos2,int pos3,int count_set,int *analy_pos_138_count)
{
	int analy_i=0;
	char *  analy_obu;
	analy_obu=analy_pos_obu;
	//int analy_i=verify_last_obu(analy_obu,&analy_j);
	for(analy_i=0;analy_i<60;analy_i++)
	{
	              if(strcmp(last_statu[analy_i ].last_obu,analy_obu)==0)
	        	{
			printf("the last_statu[analy_i ].last_obu is %s\n",last_statu[analy_i ].last_obu);
			printf("last_statu[analy_i ].pos_138_count is %d\n",last_statu[analy_i ].pos_138_count);
			printf("count_set is %d\n",count_set);
	        		(pos1==138||pos2==138||pos3==138)?(last_statu[analy_i ].pos_138_count++):(last_statu[analy_i ].pos_138_count);
	        		if(last_statu[analy_i ].pos_park_count<=0)
	        		{
	        			last_statu[analy_i ].pos_park=pos1;
	        			last_statu[analy_i ].pos_park_count++;
	        		}
	        		else if(last_statu[analy_i ].pos_park==pos1)
	        		{
	        			last_statu[analy_i ].pos_park_count++;
	        		}
	        		else
	        		{
	        			last_statu[analy_i ].pos_park_count--;
	        		}
	        		last_statu[analy_i ].pos_sum_set++;
	        		// printf("pos_138_count is %d\n",last_statu[analy_i ].pos_138_count);
	        		// printf("last_statu[analy_i ].pos_park %d\n",last_statu[analy_i ].pos_park);
	        		// printf("last_statu[analy_i ].pos_park_count is %d\n",last_statu[analy_i ].pos_park_count);
	        		printf("last_statu[analy_i ].pos_sum_set is %d\n",last_statu[analy_i ].pos_sum_set);
	        		*analy_pos_138_count=last_statu[analy_i ].pos_138_count;
	        		if(last_statu[analy_i ].pos_sum_set>=count_set)//finsh require count
	        		{	
	        			

	        			// if(last_statu[analy_i ].pos_138_count>7)
	        			// {
	        			// 	printf("pos_138_count is %d,return success\n",last_statu[analy_i ].pos_138_count);
	        			// 	last_statu[analy_i ].pos_park=0;
			        	// 	last_statu[analy_i ].pos_park_count=0;
			        	// 	last_statu[analy_i ].pos_138_count=0;
			        	// 	last_statu[analy_i ].pos_sum_set=0;
	        			// 	return 138;
	        			// }
	        			//else if(confirm_138==0&&last_statu[analy_i ].pos_park_count>1&&last_statu[analy_i ].pos_park>100&&last_statu[analy_i ].pos_park<113)
	        			if(last_statu[analy_i ].pos_park_count>7&&last_statu[analy_i ].pos_park>100&&last_statu[analy_i ].pos_park<139)
	        			{	
	        				printf("last_statu[analy_i ].pos_park is %d,return success\n",last_statu[analy_i ].pos_park);
	        				//last_statu[analy_i ].pos_138_count=0;
				        	//last_statu[analy_i ].pos_sum_set=0;
				        	int pos_park_tmp=last_statu[analy_i ].pos_park;
					//*analy_pos_138_count=last_statu[analy_i ].pos_138_count;
					last_statu[analy_i ].pos_park=0;
		        			last_statu[analy_i ].pos_park_count=0;
			        		last_statu[analy_i ].pos_138_count=0;
			        		last_statu[analy_i ].pos_sum_set=0;
			        		printf("CLEAN last_statu[analy_i ].pos_138_count is %d\n",last_statu[analy_i ].pos_138_count);
	        				return pos_park_tmp;
	        			}
	        			// if(last_statu[analy_i ].pos_park_count>(count_set))
	        			// {
		        			last_statu[analy_i ].pos_138_count=0;
				        	last_statu[analy_i ].pos_sum_set=0;
				        	last_statu[analy_i ].pos_park=0;
		        			last_statu[analy_i ].pos_park_count=0;
	        			// }
	        			// if(last_statu[analy_i ].pos_park_count>(count_set+1))
	        			// {
		        		// 	last_statu[analy_i ].pos_138_count=0;
		        		// 	last_statu[analy_i ].pos_sum_set=0;
		        		// 	last_statu[analy_i ].pos_park=0;
		        		// 	last_statu[analy_i ].pos_park_count=0;
				        	
	        			// }

	        		}
	              }
        		// else 
        		// {
        		// 	printf("add count is not finshed,the current count is %d\n",last_statu[analy_i ].pos_sum_set);
        		// 	return -1;
        		// }
	}
	return 0;
	
	

	
}


void set_park_to_138(char *set_park_to_138_obu,int arg_pos_138_sum_count,int arg_analy_pos )
{
	char *weight_obu=set_park_to_138_obu;
	int pos_138_sum_count=arg_pos_138_sum_count;
	int analy_pos=arg_analy_pos;
		
	pos_set(138,100,weight_obu);
	write_last_data(weight_obu,138);
	//all_pos.pos_138.com_138_time=time(NULL);
	//clean_analy_pos_result(weight_obu);


	all_pos.pos_138.com_138_time=time(NULL);

	huadu_to_138_tmp_pos.tmp_pos=0;
	sprintf(huadu_to_138_tmp_pos.tmp_obu,"00 00");
	memset(huadu_to_138_tmp_pos.tmp_unchar_pos,0x00,2);
	huadu_to_138_tmp_pos.tmp_time=time(NULL);


	
	
}
int weight_pos(int a_1,int b_1,int a_2,int b_2,int a_3,int b_3,int a_4,int b_4,char *weight_pos_obu )
{

	int weight_x_1,weight_y_1,weight_x_2,weight_y_2,weight_x_3,weight_y_3,weight_x_4,weight_y_4,i,j;
	char *weight_obu;

	 weight_x_1=a_1;
	 weight_y_1=b_1;
	 weight_x_2=a_2;
	 weight_y_2=b_2;
	 weight_x_3=a_3;
	 weight_y_3=b_3;
	 weight_x_4=a_4;
	 weight_y_4=b_4;
	 weight_obu=weight_pos_obu;


	 int pos_1,pos_104,pos_105,pos_106,pos_107,pos_108;
	 int pos_109,pos_110,pos_111,pos_112,pos_138,pos_999;

	 int pos_1_1,pos_104_1,pos_105_1,pos_106_1,pos_107_1,pos_108_1;
	 int pos_109_1,pos_110_1,pos_111_1,pos_112_1,pos_138_1,pos_999_1;

	 int pos_1_2,pos_104_2,pos_105_2,pos_106_2,pos_107_2,pos_108_2;
	 int pos_109_2,pos_110_2,pos_111_2,pos_112_2,pos_138_2,pos_999_2;

	 int pos_1_3,pos_104_3,pos_105_3,pos_106_3,pos_107_3,pos_108_3;
	 int pos_109_3,pos_110_3,pos_111_3,pos_112_3,pos_138_3,pos_999_3;

	 int pos_1_4,pos_104_4,pos_105_4,pos_106_4,pos_107_4,pos_108_4;
	 int pos_109_4,pos_110_4,pos_111_4,pos_112_4,pos_138_4,pos_999_4;

	 pos_1=0;
	 pos_104=0;
	 pos_105=0;
	 pos_106=0;
	 pos_107=0;
	 pos_108=0;
	 pos_109=0;
	 pos_110=0;
	 pos_111=0;
	 pos_112=0;
	 pos_138=0;
	 pos_999=0;

	 pos_1_1=0;
	 pos_104_1=0;
	 pos_105_1=0;
	 pos_106_1=0;
	 pos_107_1=0;
	 pos_108_1=0;
	 pos_109_1=0;
	 pos_110_1=0;
	 pos_111_1=0;
	 pos_112_1=0;
	 pos_138_1=0;
	 pos_999_1=0;

	 pos_1_2=0;
	 pos_104_2=0;
	 pos_105_2=0;
	 pos_106_2=0;
	 pos_107_2=0;
	 pos_108_2=0;
	 pos_109_2=0;
	 pos_110_2=0;
	 pos_111_2=0;
	 pos_112_2=0;
	 pos_138_2=0;
	 pos_999_2=0;

	 pos_1_3=0;
	 pos_104_3=0;
	 pos_105_3=0;
	 pos_106_3=0;
	 pos_107_3=0;
	 pos_108_3=0;
	 pos_109_3=0;
	 pos_110_3=0;
	 pos_111_3=0;
	 pos_112_3=0;
	 pos_138_3=0;
	 pos_999_3=0;

	 pos_1_4=0;
	 pos_104_4=0;
	 pos_105_4=0;
	 pos_106_4=0;
	 pos_107_4=0;
	 pos_108_4=0;
	 pos_109_4=0;
	 pos_110_4=0;
	 pos_111_4=0;
	 pos_112_4=0;
	 pos_138_4=0;
	 pos_999_4=0;


	// pos_1_1=tab(weight_x_1,weight_y_1,1);
	// pos_104_1=tab(weight_x_1,weight_y_1,104);
	 pos_105_1=tab(weight_x_1,weight_y_1,105);
	// pos_106_1=tab(weight_x_1,weight_y_1,106);
	// pos_107_1=tab(weight_x_1,weight_y_1,107);
	 pos_108_1=tab(weight_x_1,weight_y_1,108);
	 pos_109_1=tab(weight_x_1,weight_y_1,109);
	 pos_110_1=tab(weight_x_1,weight_y_1,110);
	// pos_111_1=tab(weight_x_1,weight_y_1,111);
	// pos_112_1=tab(weight_x_1,weight_y_1,112);
	 pos_138_1=tab(weight_x_1,weight_y_1,138);
	// pos_999_1=tab(weight_x_1,weight_y_1,999);

	// pos_1_2=tab(weight_x_2,weight_y_2,1);
	// pos_104_2=tab(weight_x_2,weight_y_2,104);
	 pos_105_2=tab(weight_x_2,weight_y_2,105);
	// pos_106_2=tab(weight_x_2,weight_y_2,106);
	// pos_107_2=tab(weight_x_2,weight_y_2,107);
	 pos_108_2=tab(weight_x_2,weight_y_2,108);
	 pos_109_2=tab(weight_x_2,weight_y_2,109);
	 pos_110_2=tab(weight_x_2,weight_y_2,110);
	// pos_111_2=tab(weight_x_2,weight_y_2,111);
	// pos_112_2=tab(weight_x_2,weight_y_2,112);
	 pos_138_2=tab(weight_x_2,weight_y_2,138);
	// pos_999_2=tab(weight_x_2,weight_y_2,999);

	// pos_1_3=tab(weight_x_3,weight_y_3,1);
	// pos_104_3=tab(weight_x_3,weight_y_3,104);
	 pos_105_3=tab(weight_x_3,weight_y_3,105);
	// pos_106_3=tab(weight_x_3,weight_y_3,106);
	// pos_107_3=tab(weight_x_3,weight_y_3,107);
	 pos_108_3=tab(weight_x_3,weight_y_3,108);
	 pos_109_3=tab(weight_x_3,weight_y_3,109);
	 pos_110_3=tab(weight_x_3,weight_y_3,110);
	// pos_111_3=tab(weight_x_3,weight_y_3,111);
	// pos_112_3=tab(weight_x_3,weight_y_3,112);
	 pos_138_3=tab(weight_x_3,weight_y_3,138);
	// pos_999_3=tab(weight_x_3,weight_y_3,999);

	// pos_1_4=tab(weight_x_4,weight_y_4,1);
	// pos_104_4=tab(weight_x_4,weight_y_4,104);
	 pos_105_4=tab(weight_x_4,weight_y_4,105);
	// pos_106_4=tab(weight_x_4,weight_y_4,106);
	// pos_107_4=tab(weight_x_4,weight_y_4,107);
	 pos_108_4=tab(weight_x_4,weight_y_4,108);
	 pos_109_4=tab(weight_x_4,weight_y_4,109);
	 pos_110_4=tab(weight_x_4,weight_y_4,110);
	// pos_111_4=tab(weight_x_4,weight_y_4,111);
	// pos_112_4=tab(weight_x_4,weight_y_4,112);
	 pos_138_4=tab(weight_x_4,weight_y_4,138);
	// pos_999_4=tab(weight_x_4,weight_y_4,999);

	 struct data
	 {
	 	int v;
		int p;
	 };
	 struct data pos[]={{0,1},{0,104},{0,105},{0,106},{0,107},{0,108},{0,109},{0,110},{0,111},{0,112},{0,138},{0,999} ,{0,0}};


	 pos_1=pos_1_1+pos_1_2+pos_1_3+pos_1_4;
	 pos_104=pos_104_1+pos_104_2+pos_104_3+pos_104_4;
	 pos_105=pos_105_1+pos_105_2+pos_105_3+pos_105_4;
	 pos_106=pos_106_1+pos_106_2+pos_106_3+pos_106_4;
	 pos_107=pos_107_1+pos_107_2+pos_107_3+pos_107_4;
	 pos_108=pos_108_1+pos_108_2+pos_108_3+pos_108_4;
	 pos_109=pos_109_1+pos_109_2+pos_109_3+pos_109_4;
	 pos_110=pos_110_1+pos_110_2+pos_110_3+pos_110_4;
	 pos_111=pos_111_1+pos_111_2+pos_111_3+pos_111_4;
	 pos_112=pos_112_1+pos_112_2+pos_112_3+pos_112_4;
	 pos_138=pos_138_1+pos_138_2+pos_138_3+pos_138_4;
	 pos_999=pos_999_1+pos_999_2+pos_999_3+pos_999_4;

	 pos[0].v=pos_1;
	 pos[1].v=pos_104;
	 pos[2].v=pos_105;
	 pos[3].v=pos_106;
	 pos[4].v=pos_107;
	 pos[5].v=pos_108;
	 pos[6].v=pos_109;
	 pos[7].v=pos_110;
	 pos[8].v=pos_111;
	 pos[9].v=pos_112;
	 pos[10].v=pos_138;
	 pos[11].v=pos_999;
	 
	for(i=0;i<12;i++)
	{
		for(j=i;j<12;j++)
		{
			if(pos[i].v < pos[j].v)
	 		{
				pos[12]=pos[i];
				pos[i]=pos[j];
				pos[j]=pos[12];
			}
		}
	}
	if(strcmp(weight_obu,"00 00")!=0)

	{
		 int last_statu_pos;
  		
 		int pos_138_sum_count=0;	
 
 		last_statu_pos=verify_last_statu(weight_obu);
 		//printf("%sthe last_statu_pos is %d\n",weight_obu,last_statu_pos);
 		int analy_pos=0;
 		if(last_statu_pos!=-2&&last_statu_pos!=-4)
 		{
 			analy_pos=analy_pos_result(weight_obu, pos[0].p,pos[1].p,pos[2].p,10,&pos_138_sum_count);
 		}
 		printf("pos_138_sum_count is %d\n",pos_138_sum_count );
 		if(last_statu_pos==0)
 		{
			printf("no this obu,this car in new come,this happend in weight_pos\n");
			pos_set(1,0,weight_obu);
			write_last_data(weight_obu,1);
		 }
		if(last_statu_pos==1 )//in case,obu renew very quickly,if no this ,this obu statu maybe last forever
		 {
 			printf("if you see this ,maybe innormal beause obu renew too fast,but have fixed.all_pos.pos_1.pos_obu=%s\n",all_pos.pos_1.pos_obu);
 			if(strcmp(all_pos.pos_1.pos_obu,"00 00")==0)
 			{	
 				printf("OBU编号为%s的车正在进站，请做好接车准备,this happend in weight_pos\n",weight_obu);
 				string_to_integer(weight_obu ,send_position_input_obu);
				send_position(send_position_input_obu,all_pos.pos_1.pos_unchar_pos);
				write_last_data(weight_obu,-2);//-2仅代表进站已读，未到停车位或待发班位的状态
				//sprintf(all_pos.pos_1.pos_obu,"00 00");
				//all_pos.pos_1.pos_weight=0;
			}
 		}
 		else if(last_statu_pos==-1)//1 and 105-110 reset,write directly
		{
  			 if(pos[0].v != 0)
 			{ 
				if(strcmp(all_pos.pos_138.pos_obu,"00 00")==0)
				{	
					printf("-1,locate 138,pos_138_sum_count,%d\n",pos_138_sum_count);
					if(pos_138_sum_count>9)
					{
						printf("pos_138_sum_count>9,enter location\n");
						pos_set(138,100,weight_obu);
						write_last_data(weight_obu,138);
						all_pos.pos_138.com_138_time=time(NULL);
						
					}
					else if(analy_pos>103&&analy_pos<139)
					{
						
						printf("-1,138 is null,start location 103-112,analy_pos is %d\n",analy_pos);
						locat_105_to_110_position(60,analy_pos,pos[1].v,pos[1].p,pos[2].v,pos[2].p,pos[3].v,pos[3].p,weight_obu);
					
					}

				}


			
				else  
				{
					printf("-1,138 not null,start location,analy_pos is %d\n",analy_pos);
					if(analy_pos>103&&analy_pos<113)
					{
						locat_105_to_110_position(60,analy_pos,pos[1].v,pos[1].p,pos[2].v,pos[2].p,pos[3].v,pos[3].p,weight_obu);
					}
				}
				
			}
 		}
		else if(last_statu_pos==-2)//renew last_statu time
 		{
 			//printf("this return -2\n");
 			//write_last_data(weight_obu,-2);
 			
 			renew_diff_time_last_obu(weight_obu);
 		}
 		else if(last_statu_pos==-3)//this coach have left,ignore it!
 		{
			printf("this coach %s have left,ignore it!\n",weight_obu);
			renew_diff_time_last_obu(weight_obu);
			if(strcmp(all_pos.pos_138.pos_obu,"0xff")==0&&difftime(time(NULL),all_pos.pos_138.pos_time)<95.0)
			{	
				printf(" PREV waiting time is %f!\n",difftime(time(NULL),all_pos.pos_138.pos_time));
				all_pos.pos_138.pos_time=time(NULL);
				printf("the waiting time is renew,time is %f!\n",difftime(time(NULL),all_pos.pos_138.pos_time));
			}

			//park_to_138_time=250.0;
			park_to_138_time=obu_renew_max_time;
			printf("-3 renew park_to_138_time is %f\n",park_to_138_time);
			

		 } 
		else if(last_statu_pos==-4)//the coach in 138 have left,specify later obu in this position.
 		{
			analy_pos=analy_pos_result(weight_obu, pos[0].p,pos[1].p,pos[2].p,20,&pos_138_sum_count);
			
 			only_renew_obu_info=0;
 			
 			(prev_analy_pos==0)?(prev_analy_pos=analy_pos):(prev_analy_pos);
 			(analy_pos==105)?(prev_analy_pos=analy_pos):(prev_analy_pos);
 			printf("prev_analy_pos is %d,analy_pos is %d\n",prev_analy_pos,analy_pos);
 			
 			if(prev_analy_pos!=analy_pos)
 			{
 				printf("prev_analy_pos is %d,analy_pos is %d\n",prev_analy_pos,analy_pos);
 				set_park_to_138(weight_obu,pos_138_sum_count,analy_pos );
 				prev_analy_pos=0;
 			}
			
 			if(analy_pos==138||pos_138_sum_count>18)
			{
				
				set_park_to_138(weight_obu,pos_138_sum_count,analy_pos );

				
			}
			
			
		 } 

               else if(last_statu_pos==-5){
                      printf("coach %s not 1 floor\n",weight_obu);
               }

 		else if(last_statu_pos==138)
		 {
			if(pos[0].p==138||pos[1].p==138||pos[2].p==138)
 			{
			
				if(pos[0].p==138)
				{
					pos_set(138,pos[0].v,weight_obu);
				}
				if(pos[1].p==138)
				{
					pos_set(138,pos[1].v,weight_obu);
			
				}
				if(pos[2].p==138)
				{
					pos_set(138,pos[2].v,weight_obu);
			
				}
				write_last_data(weight_obu,138);
		

			}
		
 		}

		else if(last_statu_pos<138&&last_statu_pos>103)
 		{


			if(clean_prev_105_to_110_pos(last_statu_pos)==1)
			{	
				
				

				if(strcmp(all_pos.pos_138.pos_obu,"00 00")==0)
				//if(pos_138_sum_count>9&&strcmp(all_pos.pos_138.pos_obu,"00 00")==0)
				{
					//int prev_analy_pos=0;

					//analy_pos;
					
					sprintf(all_pos.pos_138.pos_obu,"0xff");
					all_pos.pos_138.pos_time=time(NULL);
					park_to_138_time=10.0;
					printf("-4,138 is null,,park_to_138_time,%f\n",park_to_138_time);

				}
				
	  			else 
				{
					
					write_last_data(weight_obu,last_statu_pos);
					pos_set(last_statu_pos,60,weight_obu);
				}
								
			}
				
		}
 
	}

	 
		
}

void weight_report_position()
{
	
	if(strcmp(all_pos.pos_105.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车位置是105\n",all_pos.pos_105.pos_obu);
		string_to_integer(all_pos.pos_105.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_105.pos_unchar_pos);
	}
	
	if(strcmp(all_pos.pos_108.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车位置是108\n",all_pos.pos_108.pos_obu);
		string_to_integer(all_pos.pos_108.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_108.pos_unchar_pos);
		
	}
	
	if(strcmp(all_pos.pos_109.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车位置是109\n",all_pos.pos_109.pos_obu);
		string_to_integer(all_pos.pos_109.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_109.pos_unchar_pos);
		
	}
	
	if(strcmp(all_pos.pos_110.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车位置是110\n",all_pos.pos_110.pos_obu);
		string_to_integer(all_pos.pos_110.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_110.pos_unchar_pos);
		
	}
	
	if(strcmp(all_pos.pos_138.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_138.pos_time)<40.0&&strcmp(all_pos.pos_138.pos_obu,"0xff")!=0&&strcmp(all_pos.pos_138.pos_obu,"0xfe")!=0)
	{
		printf("OBU编号为%s的车位置是138\n",all_pos.pos_138.pos_obu);
		string_to_integer(all_pos.pos_138.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_138.pos_unchar_pos);
		
	}
	if(strcmp(huadu_to_138_tmp_pos.tmp_obu,"00 00")!=0&&difftime(time(NULL),huadu_to_138_tmp_pos.tmp_time)<800.0)
	{
		printf("this happend in report huadu_to_138_tmp_pos.tmp_pos \n");
		printf("this is last tmp OBU编号为%s的车位置是%d\n",huadu_to_138_tmp_pos.tmp_obu,huadu_to_138_tmp_pos.tmp_pos);
		string_to_integer(huadu_to_138_tmp_pos.tmp_obu ,send_position_input_obu);
		send_position(send_position_input_obu,huadu_to_138_tmp_pos.tmp_unchar_pos);
		
	}

		
	
	
}

start_renew_park_to_138_time()
{
	(obu_renew_max_time>50.0)?(park_to_138_time_last=-30.0):(park_to_138_time_last=0.0);
	printf("obu_renew_max_time is %f,park_to_138_time_last is %f\n",obu_renew_max_time,park_to_138_time_last);
	int park_car_number=0;
	(strcmp(all_pos.pos_105.pos_obu,"00 00")==0)?(park_car_number):(park_car_number++);
	(strcmp(all_pos.pos_108.pos_obu,"00 00")==0)?(park_car_number):(park_car_number++);
	(strcmp(all_pos.pos_109.pos_obu,"00 00")==0)?(park_car_number):(park_car_number++);
	(strcmp(all_pos.pos_110.pos_obu,"00 00")==0)?(park_car_number):(park_car_number++);
	switch(park_car_number)
	{
		case 0:
		park_to_138_time_coach=-60.0;
		break;
		case 1:
		park_to_138_time_coach=-20.0;
		break;
		case 2:
		park_to_138_time_coach=100.0;
		break;
		case 3:
		park_to_138_time_coach=150.0;
		break;
		case 4:
		park_to_138_time_coach=150.0;
		break;
		default :
		printf("park_car_number is invalid,please check it:%d\n",park_car_number);

	}
	printf("park_to_138_time_coach is %f,park_car_number is %d\n",park_to_138_time_coach,park_car_number);
	float park_to_138_time_tmp=park_to_138_time_base+park_to_138_time_last+park_to_138_time_coach;
	printf("park_to_138_time_tmp is %f\n",park_to_138_time_tmp);
	(park_to_138_time_tmp<=0.0)?(park_to_138_time=10.0):(park_to_138_time=park_to_138_time_tmp);
	printf("park_to_138_time is %f\n",park_to_138_time);




}

void only_in_out_report_station()//这个函数和上个report函数分开是为了及时刷新1和999的状态，并且不会使发班位和代发班位位置刷新太快。
{	
	if(strcmp(all_pos.pos_1.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车正在进站，请做好接车准备\n",all_pos.pos_1.pos_obu);
		string_to_integer(all_pos.pos_1.pos_obu ,send_position_input_obu);
		send_position(send_position_input_obu,all_pos.pos_1.pos_unchar_pos);

		memcpy(comp_target,all_pos.pos_1.pos_obu,4);
		printf("the comp_target is %s\n",comp_target);
		printf("the comp_source is %s\n",comp_source);
              if(strcmp(comp_source,comp_target) == 0)
              {
                write_last_data(all_pos.pos_1.pos_obu,-2);//-2仅代表进站已读，未到停车位或待发班位的状态
              }
              else
              {
               printf("this set -5\n");
                write_last_data(all_pos.pos_1.pos_obu,-5);//not 1 floor coach,report come in only
              }

		sprintf(all_pos.pos_1.pos_obu,"00 00");
		all_pos.pos_1.pos_weight=0;
	}
	if(strcmp(all_pos.pos_138.pos_obu,"00 00")!=0&&strcmp(all_pos.pos_138.pos_obu,"0xff")!=0&&strcmp(all_pos.pos_138.pos_obu,"0xfe")!=0&&difftime(time(NULL),all_pos.pos_138.com_138_time)>delay_out_station&&difftime(time(NULL),all_pos.pos_138.pos_time)>obu_renew_max_time)
	{	delay_out_station=240.0;
		printf("这是138大于80S的情况下发生的，代表这辆138已经开出。obu:%s out time is %f\n",all_pos.pos_138.pos_obu,difftime(time(NULL),all_pos.pos_138.pos_time));
		printf("这是138 start time is %f ，代表这辆138 CAN BE 999。\n",difftime(time(NULL),all_pos.pos_138.com_138_time));
		start_renew_park_to_138_time();
		
		
		pos_set(999,100,all_pos.pos_138.pos_obu);//for test
		sprintf(all_pos.pos_138.pos_obu,"0xff");//for avoid negtative num at obu and do not report this statu
		all_pos.pos_138.pos_time=time(NULL);//renew time,if this statu last 100S,start specify next car to 138
	}	

	if(strcmp(all_pos.pos_999.pos_obu,"00 00")!=0)
	{
		printf("OBU编号为%s的车出站。。。",all_pos.pos_999.pos_obu);
		string_to_integer(all_pos.pos_999.pos_obu ,send_position_input_obu);
		unsigned char out_report[2]={0x09,0x99};
		printf("%X,%X\n",out_report[0],out_report[1]);
		//send_position(send_position_input_obu,all_pos.pos_999.pos_unchar_pos);
		send_position(send_position_input_obu,out_report);
		write_last_data(all_pos.pos_999.pos_obu,-3);//-3仅代表出站已读，obu暂时保留，以防重复进站
		sprintf(all_pos.pos_999.pos_obu,"00 00");
		all_pos.pos_999.pos_weight=0;
	}

}
int force_locate(int arry_i)
{

	int com_in_station_i=arry_i;
	if(strcmp(all_pos.pos_138.pos_obu,"00 00")==0)
	{
		printf("-1,start force locate 138,time is %f\n",difftime(time(NULL),last_statu[com_in_station_i].wait_locate_time));
		pos_set(138,100,last_statu[com_in_station_i].last_obu);
		write_last_data(last_statu[com_in_station_i].last_obu,138);
		all_pos.pos_138.com_138_time=time(NULL);
		
	}
	else
	{
		printf("-1,start force locate 103-112,time is %f\n",difftime(time(NULL),last_statu[com_in_station_i].wait_locate_time));
		locat_105_to_110_position(10,108,10,105,10,109,10,110,last_statu[com_in_station_i].last_obu);
		
	}
	return 1;
}
void com_in_station_time()
{
	int com_in_station_i;
	for(com_in_station_i=0;com_in_station_i<60;com_in_station_i++)
	{
		
		if(last_statu[com_in_station_i].last_pos==-2&&difftime(time(NULL),last_statu[com_in_station_i].com_station_last_time)>120.0)
		{
			printf("这个函数实在进站大于120S的情况下，将obu状态从-2,写位-1,obu:%s income time is %f\n",last_statu[com_in_station_i].last_obu,difftime(time(NULL),last_statu[com_in_station_i].com_station_last_time));
			write_last_data(last_statu[com_in_station_i].last_obu,-1);//-1代表可以立即定位，针对进站和停车位重置情况。
			last_statu[com_in_station_i].wait_locate_time=time(NULL);
		}
		if(last_statu[com_in_station_i].last_pos==-1&&difftime(time(NULL),last_statu[com_in_station_i].wait_locate_time)>140.0&&difftime(time(NULL),last_statu[com_in_station_i].last_time)<15.0)
		{
			
			
			if(difftime(time(NULL),go_to_security_set_time)>25.0&&difftime(time(NULL),go_to_security_set_time)<50.0)
			{
				
				if(force_locate(com_in_station_i)==1)
				{
					printf("now to go_to_security_set_time is %f,locate success",difftime(time(NULL),go_to_security_set_time));
				}
			}
			else if(difftime(time(NULL),go_to_security_set_time)>80.0)
			{
				
				
				if(force_locate(com_in_station_i)==1)
				{
					printf("this car have not been to security,force locate success\n",go_to_security_set_time);
				}
			}
			
		}
		
	}

}
/****************************************
fun name:void verify_out_station()
use:1 avoid in station or out station again in a shot time
use:2 judge position 138 whther left,if this position data renew time exceed 1 min,then judge this car left;
reminder:-3 only response 999 statu,but the obu have not clear .-138 only response the last car at 138 have left,the specify new car have not com. 
****************************************/

void verify_out_station()
{	
	int out_station_i;
	for(out_station_i=0;out_station_i<60;out_station_i++)
	{

              
		
		if(last_statu[out_station_i].last_pos==-3&&difftime(time(NULL),last_statu[out_station_i].last_time)>1800.0)
		{
			printf("这是在出站大于1800S的情况下发生的，将该obu在last_statu状态清零。obu:%s out time is %f\n",last_statu[out_station_i].last_obu,difftime(time(NULL),last_statu[out_station_i].last_time));
			sprintf(last_statu[out_station_i].last_obu,"00 00");
			last_statu[out_station_i].last_pos=0;
			last_statu[out_station_i].last_time=0;
		}


		if(strcmp(last_statu[out_station_i].last_obu,"00 00")!=0&&strcmp(last_statu[out_station_i].last_obu,all_pos.pos_138.pos_obu)==0&&obu_renew_max_time_tmp!=last_statu[out_station_i].diff_max_time)
		{
			//printf("obu_renew_max_time is %f\n",obu_renew_max_time);
			//printf("last_statu[out_station_i].diff_max_time+10.0 is %f\n",last_statu[out_station_i].diff_max_time+30.0);
			obu_renew_max_time_tmp=last_statu[out_station_i].diff_max_time;
			if(obu_renew_max_time_tmp<30.0)
			{
				obu_renew_max_time=50.0;
			}
			else
			{
				obu_renew_max_time=obu_renew_max_time_tmp;
			}
			printf("the obu:%s obu_renew_max_time is %f\n",last_statu[out_station_i].last_obu,obu_renew_max_time);
			
		}


		if(last_statu[out_station_i].last_pos==-4&&difftime(time(NULL),last_statu[out_station_i].last_time)>850)
		{
			printf("last_statu[%d].last_pos is %d\n",out_station_i,last_statu[out_station_i].last_pos);
			printf("这是last_statu  -4  大于850S的情况下发生的，set obu:%s is 999， time is %f，这个应该不会发生。\n",last_statu[out_station_i].last_obu,difftime(time(NULL),last_statu[out_station_i].last_time));
			//if(clean_prev_105_to_110_pos(last_statu[out_station_i].last_pos)==1)
			//{
				printf("clean prev 850s status success\n" );
				pos_set(138,100,last_statu[out_station_i].last_obu);//this clean -4 statu,obu last_pos has cleaned in re_locat_138
				sprintf(last_statu[out_station_i].last_obu,"00 00");
				last_statu[out_station_i].last_pos=0;
				huadu_to_138_tmp_pos.tmp_pos=0;
			sprintf(huadu_to_138_tmp_pos.tmp_obu,"00 00");
			memset(huadu_to_138_tmp_pos.tmp_unchar_pos,0x00,2);
			huadu_to_138_tmp_pos.tmp_time=time(NULL);

			//}
		}
		if(strcmp(last_statu[out_station_i].last_obu,"00 00")!=0&& last_statu[out_station_i].last_pos !=-5&&difftime(time(NULL),last_statu[out_station_i].last_time)>1800.0)
		{
			printf("这是last_statu某个obu大于1800S的情况下发生的，清除obu:%s， time is %f，这个应该不会发生。\n",last_statu[out_station_i].last_obu,difftime(time(NULL),last_statu[out_station_i].last_time));

			pos_set(999,100,last_statu[out_station_i].last_obu);//this clean -4 statu,obu last_pos has cleaned in re_locat_138
			sprintf(last_statu[out_station_i].last_obu,"00 00");
			last_statu[out_station_i].last_pos=0;

			//sprintf(last_statu[out_station_i].last_obu,"00 00");
			last_statu[out_station_i].last_pos=0;
		}
              if(strcmp(last_statu[out_station_i].last_obu,"00 00")!=0&& last_statu[out_station_i].last_pos ==-5&&difftime(time(NULL),last_statu[out_station_i].last_time)>1800.0)
              {
                printf("这是not 1floor last_statu大于1800S的情况下发生的，清除obu:%s， time is %f\n",last_statu[out_station_i].last_obu,difftime(time(NULL),last_statu[out_station_i].last_time));

                
                sprintf(last_statu[out_station_i].last_obu,"00 00");
                last_statu[out_station_i].last_pos=0;

                //sprintf(last_statu[out_station_i].last_obu,"00 00");
                last_statu[out_station_i].last_pos=0;
              }
		
	}	
}

float get_obu_in_time(char *get_obu_in_time_obu)
{
	int get_obu_in_time_i;
	char *get_in_time_obu;
	float get_in_time;
	get_in_time_obu=get_obu_in_time_obu;
	for(get_obu_in_time_i=0;get_obu_in_time_i<60;get_obu_in_time_i++)
	{
		if(strcmp(last_statu[get_obu_in_time_i].last_obu,get_in_time_obu)==0)
		{
			get_in_time=difftime(time(NULL),last_statu[get_obu_in_time_i].com_station_last_time);	
			return get_in_time;
		}
	}
}
// re_locat_set_to_138(int re_locat_set_to_138_pos,int re_locat_set_to_138_obu, )
// {

// }
void re_locat_138()
{

	if(strcmp(all_pos.pos_138.pos_obu,"0xff")==0&&difftime(time(NULL),all_pos.pos_138.pos_time)>park_to_138_time)
	{	
		printf("enter re_locat_138!,difftime is %f\n",difftime(time(NULL),all_pos.pos_138.pos_time));
		float re_locat_a[4]={0.0,0.0,0.0,0.0},re_locat_max=0.0;
		int re_locat_i,re_locat_j;
		re_locat_j=-1;
		if(strcmp(all_pos.pos_105.pos_obu,"00 00")!=0)
		{
			re_locat_a[0]=get_obu_in_time(all_pos.pos_105.pos_obu);
		}
		if(strcmp(all_pos.pos_108.pos_obu,"00 00")!=0)
		{
			re_locat_a[1]=get_obu_in_time(all_pos.pos_108.pos_obu);
		}
		if(strcmp(all_pos.pos_109.pos_obu,"00 00")!=0)
		{
			re_locat_a[2]=get_obu_in_time(all_pos.pos_109.pos_obu);
		}
		if(strcmp(all_pos.pos_110.pos_obu,"00 00")!=0)
		{
			re_locat_a[3]=get_obu_in_time(all_pos.pos_110.pos_obu);
		}
		for(re_locat_i=0;re_locat_i<4;re_locat_i++)
		{
			if(re_locat_a[re_locat_i]>re_locat_max)
			{
				re_locat_max=re_locat_a[re_locat_i];
				re_locat_j=re_locat_i;
			}
		}
		

		switch(re_locat_j)
		{
			case -1:
			sprintf(all_pos.pos_138.pos_obu,"00 00");
			break;
			
			case 0:
			sprintf(all_pos.pos_138.pos_obu,"0xfe");
			write_last_data(all_pos.pos_105.pos_obu,-4);//138清空函数会将all_pos.pos_138的obu设置位-138,,其他后到车辆不会先霸占138位置了。-4可以重新指定obu来判断138位置了。如果设置了all_pos.pos_138,上一趟车还没完全离开，要到138车就已经开始报138站
			huadu_to_138_tmp_pos.tmp_pos=105;
			memcpy(huadu_to_138_tmp_pos.tmp_obu,all_pos.pos_105.pos_obu,17);
			memcpy(huadu_to_138_tmp_pos.tmp_unchar_pos,all_pos.pos_105.pos_unchar_pos,2);
			huadu_to_138_tmp_pos.tmp_time=time(NULL);
			sprintf(all_pos.pos_105.pos_obu,"00 00");
			all_pos.pos_105.pos_weight=0;
			break;	
			
			case 1:
			sprintf(all_pos.pos_138.pos_obu,"0xfe");
			write_last_data(all_pos.pos_108.pos_obu,-4);
			huadu_to_138_tmp_pos.tmp_pos=108;
			memcpy(huadu_to_138_tmp_pos.tmp_obu,all_pos.pos_108.pos_obu,17);
			memcpy(huadu_to_138_tmp_pos.tmp_unchar_pos,all_pos.pos_108.pos_unchar_pos,2);
			huadu_to_138_tmp_pos.tmp_time=time(NULL);
			sprintf(all_pos.pos_108.pos_obu,"00 00");
			all_pos.pos_108.pos_weight=0;
			break;	
			
			case 2:
			sprintf(all_pos.pos_138.pos_obu,"0xfe");
			write_last_data(all_pos.pos_109.pos_obu,-4);
			huadu_to_138_tmp_pos.tmp_pos=109;
			memcpy(huadu_to_138_tmp_pos.tmp_obu,all_pos.pos_109.pos_obu,17);
			memcpy(huadu_to_138_tmp_pos.tmp_unchar_pos,all_pos.pos_109.pos_unchar_pos,2);
			huadu_to_138_tmp_pos.tmp_time=time(NULL);
			sprintf(all_pos.pos_109.pos_obu,"00 00");
			all_pos.pos_109.pos_weight=0;
			break;	
			
			case 3:
			sprintf(all_pos.pos_138.pos_obu,"0xfe");
			write_last_data(all_pos.pos_110.pos_obu,-4);
			huadu_to_138_tmp_pos.tmp_pos=110;
			memcpy(huadu_to_138_tmp_pos.tmp_obu,all_pos.pos_110.pos_obu,17);
			memcpy(huadu_to_138_tmp_pos.tmp_unchar_pos,all_pos.pos_110.pos_unchar_pos,2);
			huadu_to_138_tmp_pos.tmp_time=time(NULL);
			sprintf(all_pos.pos_110.pos_obu,"00 00");
			all_pos.pos_110.pos_weight=0;
			break;
			
			default:
			printf(" re_locat_138 re_locat_j: %d innormal:",re_locat_j);
		}
	}
}

void watch_dog_105_to_138()
{
	
	if(strcmp(all_pos.pos_105.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_105.pos_time)>4500.0) //think coach may many,time set little long 4500
	{
		printf("105这是在停车位大于4500S的情况下发生的，要将停车位清零。OBU编号为%s的车位置超时%f\n",all_pos.pos_105.pos_obu,difftime(time(NULL),all_pos.pos_105.pos_time));//for test
		//clear_obu_last_statu(all_pos.pos_105.pos_obu);
		write_last_data(all_pos.pos_105.pos_obu,-1);
		sprintf(all_pos.pos_105.pos_obu,"00 00");
		all_pos.pos_105.pos_weight=0;
		
	}
	if(strcmp(all_pos.pos_108.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_108.pos_time)>4500.0)
	{
		printf("108这是在停车位大于4500S的情况下发生的，要将停车位清零。OBU编号为%s的车位置超时%f\n",all_pos.pos_108.pos_obu,difftime(time(NULL),all_pos.pos_108.pos_time));//for test
		//clear_obu_last_statu(all_pos.pos_108.pos_obu);
		write_last_data(all_pos.pos_108.pos_obu,-1);
		sprintf(all_pos.pos_108.pos_obu,"00 00");
		all_pos.pos_108.pos_weight=0;
	}
	if(strcmp(all_pos.pos_109.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_109.pos_time)>4500.0)
	{
		printf("109这是在停车位大于4500S的情况下发生的，要将停车位清零。OBU编号为%s的车位置超时%f\n",all_pos.pos_109.pos_obu,difftime(time(NULL),all_pos.pos_109.pos_time));//for test
		//clear_obu_last_statu(all_pos.pos_109.pos_obu);
		write_last_data(all_pos.pos_109.pos_obu,-1);
		sprintf(all_pos.pos_109.pos_obu,"00 00");
		all_pos.pos_109.pos_weight=0;
	}
	if(strcmp(all_pos.pos_110.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_110.pos_time)>4500.0)
	{
		printf("110这是在停车位大于4500S的情况下发生的，要将停车位清零。OBU编号为%s的车位置超时%f\n",all_pos.pos_110.pos_obu,difftime(time(NULL),all_pos.pos_110.pos_time));//for test
		//clear_obu_last_statu(all_pos.pos_110.pos_obu);
		write_last_data(all_pos.pos_110.pos_obu,-1);
		sprintf(all_pos.pos_110.pos_obu,"00 00");
		all_pos.pos_110.pos_weight=0;
	}
	if(strcmp(all_pos.pos_138.pos_obu,"00 00")!=0&&difftime(time(NULL),all_pos.pos_138.pos_time)>3000.0)//3000
	{
		printf("138这是在停车位大于3000S的情况下发生的,should never happen，要将138位清零。OBU编号为%s的车位置超时%f\n",all_pos.pos_138.pos_obu,difftime(time(NULL),all_pos.pos_138.pos_time));//for test
		clear_obu_last_statu(all_pos.pos_138.pos_obu);
		sprintf(all_pos.pos_138.pos_obu,"00 00");
		all_pos.pos_138.pos_weight=0;
	}
	if(strcmp(all_pos.pos_138.pos_obu,"0xfe")==0&&difftime(time(NULL),all_pos.pos_138.pos_time)>850.0)
	{
		printf("138 这是0xfe在大于850S的情况下发生的，要将138位0xff.超时%f\n",difftime(time(NULL),all_pos.pos_138.pos_time));
		sprintf(all_pos.pos_138.pos_obu,"0xff");
		all_pos.pos_138.pos_time=time(NULL);
		
		
	}
	if(strcmp(huadu_to_138_tmp_pos.tmp_obu,"00 00")!=0&&difftime(time(NULL),huadu_to_138_tmp_pos.tmp_time)>805.0)
	{
		printf("this is clean  huadu_to_138_tmp_pos.tmp_pos,time is %f \n",difftime(time(NULL),huadu_to_138_tmp_pos.tmp_time));
		printf("read to clean OBU编号为%s的车位置是%d\n",huadu_to_138_tmp_pos.tmp_obu,huadu_to_138_tmp_pos.tmp_pos);
		huadu_to_138_tmp_pos.tmp_pos=0;
		sprintf(huadu_to_138_tmp_pos.tmp_obu,"00 00");
		memset(huadu_to_138_tmp_pos.tmp_unchar_pos,0x00,2);
		huadu_to_138_tmp_pos.tmp_time=time(NULL);
		
	}
}

run_watch_dog()
{
	usleep(10000);
	only_in_out_report_station();
	com_in_station_time();
	verify_out_station();
	watch_dog_105_to_138();
	re_locat_138();
}

void weight_watch_dog()
{
	time_t time_30s_ago=0;
	while(1)
	{
		run_watch_dog();
		if(difftime(time(NULL),time_30s_ago)>10.0)
		{	
			weight_report_position();
			printf("\n\n\n");
			time_30s_ago=time(NULL);	


		}
	}
}










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
  //printf("begin write \n");
  if((nbytes=write(sockfd,buff,29))==-1)
  {
    printf("Write Error!\n");
    exit(1);  
    }
  //printf("end of write \n");
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
  pthread_t id_watch_dog;
  int pthread_Err = pthread_create(&save_msg_pthread,NULL,save_msg,NULL);
  if (pthread_Err != 0)
  {
  printf("Create thread Failed!\n");
  return EXIT_FAILURE;
  }

  

  int ret_new;

  ret_new=pthread_create(&id_watch_dog,NULL,(void *) weight_watch_dog,NULL);
  if(ret_new)
  {
  printf ("Create pthread error!\n");
  exit (1);
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



  pthread_join(id_watch_dog,NULL);
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
        
      case 0x04:    /*RSSI汇总帧*/
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
  unsigned char buffer_send_rsu_zy[200]; /*够用*/
  int buffer_send_rsu_len = 0;/*出队列的长度*/
  int buffer_send_rsu_lenzy = 0;/*出队列的长度*/
  unsigned char error_buffer[200];
  unsigned char send_convet_buffer[200];
  unsigned char send_convet_bufferzy[200];
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
		memset(buffer_send_rsu_zy,0,200);
		memset(send_convet_buffer,0,200);
		memset(send_convet_bufferzy,0,200);
		buffer_send_rsu_len = DeQueue(MyQueue,buffer_send_rsu,buffer_send_rsu_len);
		//add zhuayi		
		convet(buffer_send_rsu, send_convet_buffer,buffer_send_rsu_len);
		if(buffer_send_rsu[1]==0x88)
			writeFile(fop_log, send_convet_buffer, 19); 
		printf("*****before the send data is %s\n",send_convet_buffer );

		int i=0;
		for(i=1;i<buffer_send_rsu_len-1;)
		{
			if(buffer_send_rsu[i]==0xf1||buffer_send_rsu[i]==0xf0||buffer_send_rsu[i]==0xf2)
				buffer_send_rsu[i++]=0xf3;
			else if((buffer_send_rsu[i]&0x0f==0x0f)&&(buffer_send_rsu[i+1]>>4==0x00||buffer_send_rsu[i+1]>>4==0x01||buffer_send_rsu[i+1]>>4==0x02))
			{
				i++;
				buffer_send_rsu[i]=0x30+(buffer_send_rsu[i]&0x0f);
			}
			else i++;
		}
		//buffer_send_rsu_lenzy=zhuanyi(send_convet_buffer, send_convet_bufferzy,buffer_send_rsu_len*2);
		memset(send_convet_buffer,0,200);
		convet(buffer_send_rsu, send_convet_buffer,buffer_send_rsu_len);
		//writeFile(fop_log, send_convet_buffer, 19);
		//convert_atohex1(send_convet_bufferzy,buffer_send_rsu_zy);
		 
		printf("*****the send data is %s\n",send_convet_buffer);
		
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
        convet(buffer_send_rsu_zy, error_buffer,buffer_send_rsu_lenzy);
        writeFile(send_fail_fop, error_buffer, 5);
        closeFile(send_fail_fop);
      }     
      //printf("send nbytes = %d\n",nbytes);
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
    if(difftime(time(NULL),heartbeat_t) >= 900)   
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
        //if(0 == flag)
        //{
          //printf("time out \n");
          //;
        //}       
        sleep(3);     
        fcntl(sockfd,F_SETFL,val & (~O_NONBLOCK));        
      }     
    }
    //时间监控部分
    
    // if(floor_value==4)
    // {
    //  int i=0;
    //  for(i=0;i<10;i++)
    //  {
    //    if(coach_obu[i][7]!=0)
    //    {
    //      if(difftime(time(NULL),coach_time[i]) >= 60&&coach_flag[i]==0x401)   
    //      {     
    //        //printf("the difftime is %f\n",difftime(time(NULL),four_time[now_obu]));
    //        unsigned char coach_char[16]={0};
    //        convet(coach_obu[i], coach_char,8);
    //        writeFile(fop_log, coach_char,14);
    //        writeFile(fop_log, NULL,17);
    //        unsigned char pos[2]={0x04,0x09};
    //        send_position(coach_obu[i],pos);          
            
    //        //车出四楼，清空数据
    //        coach_flag[i]=0;
    //        coach_time[i]=0;
    //        memset(coach_obu[i],0,8);
    //        coach_time[i]=0;
    //      }
    //      else sleep(5);
    //    }
    //  }
    // }
    // if(floor_value==1)
    // {
    //  int i=0;
    //  for(i=0;i<10;i++)
    //  {
    //    if(coach_obu[i][7]!=0)
    //    {
    //      if(difftime(time(NULL),coach_time[i]) >= 60)   
    //      {     
    //        //printf("the difftime is %f\n",difftime(time(NULL),four_time[now_obu]));
    //        unsigned char coach_char[16]={0};
    //        convet(coach_obu[i], coach_char,8);
    //        writeFile(fop_log, coach_char,14);
    //        writeFile(fop_log, NULL,12);
    //        unsigned char pos[2]={0x09,0x99};
    //        send_position(coach_obu[i],pos);          
            
    //        //车出站，清空数据
    //        coach_flag[i]=0;
    //        coach_time[i]=0;
    //        memset(coach_obu[i],0,8);
    //        memset(coach_rssi[i],0,8);
    //        memset(coach_pos[i],0,2);
    //      }
    //      else sleep(5);
    //    }
    //  }
    // }
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
    FD_ZERO(&read_fd);
    FD_SET(sockfd,&read_fd);  
    tm.tv_sec = 3;
    tm.tv_usec = 0;   
    flag = select(sockfd+1,&read_fd,NULL,NULL,&tm);
    switch(flag)
    {
      case 0:
        //printf("time out ");
        break;
      case -1:
        printf("select error occoured ");
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
  ssize_t len;/*读回来的长度*/
  ssize_t datalen;
  ssize_t p;/*定义一个变量存放读到的数据*/
  //unsigned char virtral_data[48]={0x7e,0x30,0x04,0x01,0x02,0x61,0x02,0x00,0x00,0x03,0x81,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x11,0xbe,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x16,0xac,0x00,0x04,0x64,0x01,0x00,0x00,0x00,0x12,0xa5,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char data_from_uart[100];
  unsigned char uart_convet[200];
  
  p=0x00;
  char rssi[300];
  char obu_info[300]; 
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
   memset(data_from_uart,0,100);
   memset(uart_convet,0,200);
   data_from_uart[0] = 0x7e;
   data_from_uart[1] = p;
   memcpy(data_from_uart+2,ptr,len);/*包含校验位*/
   convet(data_from_uart, uart_convet, p);/*uart_convet里面数据是ascii*/
   //转换成ascii码后，一位变成两位
   memcpy(obu_info,"OBU:",4);
   memcpy(obu_info+4,uart_convet+6,16);        
   memcpy(obu_info+20,";CardID:",8);
   memcpy(obu_info+28,uart_convet+22,16);
   memcpy(obu_info+44,";Other info:",12);
   memcpy(obu_info+56,uart_convet+38,8);
   //writeFile(fop_log, obu_info, 15);
   //printf("读到obu信息帧:%s\n",obu_info);
   char obu_time_renew[17]={0};
    memcpy(obu_time_renew,uart_convet+6,16);
   // printf("the obu_time_renew is %s\n",obu_time_renew);
    int last_obu_time_renew_i;
    int last_statu_pos=0;
    last_statu_pos=verify_last_statu(obu_time_renew);
    if(last_statu_pos==0)
    {
    	printf("no this obu,this car in new come\n");
	pos_set(1,0,obu_time_renew);
	write_last_data(obu_time_renew,1);
    }
    else
    {
	for(last_obu_time_renew_i=0;last_obu_time_renew_i<20;last_obu_time_renew_i++)
	{
		if(strcmp(last_statu[last_obu_time_renew_i].last_obu,obu_time_renew)==0)
		{
			printf("renew obu %s info last_time success\n",obu_time_renew);
			if(last_statu[last_obu_time_renew_i].last_pos==-1)
			{
				if(difftime(time(NULL),last_statu[last_obu_time_renew_i].last_time)>60.0)
				{
					printf("this car have gone to security\n");
					//float verify_renew_security_time=difftime(time(NULL),go_to_security_set_time);
					if(difftime(time(NULL),go_to_security_set_time)>50.0)
					{
					go_to_security_set_time=time(NULL);
					}
				}
				

			}
			last_statu[last_obu_time_renew_i].last_time=time(NULL);

			if(last_statu[last_obu_time_renew_i].last_pos==-4)
		    	{
		    		only_renew_obu_info++;
		    		printf("only_renew_obu_info is %d\n",only_renew_obu_info);
		    		if(only_renew_obu_info>3)
		    		{
		    			printf("-4,to 138,only_renew_obu_info force to 138\n");
		    			pos_set(138,100,obu_time_renew);
					write_last_data(obu_time_renew,138);
					//all_pos.pos_138.com_138_time=time(NULL);
					//clean_analy_pos_result(weight_obu);


					all_pos.pos_138.com_138_time=time(NULL);

					huadu_to_138_tmp_pos.tmp_pos=0;
					sprintf(huadu_to_138_tmp_pos.tmp_obu,"00 00");
					memset(huadu_to_138_tmp_pos.tmp_unchar_pos,0x00,2);
					huadu_to_138_tmp_pos.tmp_time=time(NULL);


					
		    		}
		    		
		    	}
		}
			
	}
	 if(strcmp(all_pos.pos_138.pos_obu,obu_time_renew)==0)
	 {
	 	all_pos.pos_138.pos_time=time(NULL);
	 	if(difftime(time(NULL),all_pos.pos_138.com_138_time)<240.0)
	 	{
	 		renew_diff_time_last_obu(obu_time_renew);
	 	}
	 	
	 	//delay_out_station+=30.0;
	 	//printf("delay_out_station is %f\n",delay_out_station);
	 }
	//if(strcmp(all_pos.pos_138.pos_obu,obu_time_renew)==0||
	if(strcmp(all_pos.pos_138.pos_obu,"0xff")==0&&verify_last_statu(obu_time_renew)==-3)
	{
		all_pos.pos_138.pos_time=time(NULL);

		//printf("renew 138 time is success,time is %f\n",difftime(time(NULL),all_pos.pos_138.pos_time));
	}
     }

   // int last_obu=0;// obu最后一位
   // last_obu=ptr[8];  
    
   // /***如果是安检位置，则发送安检位置给服务器***/
   // if(floor_value==9)
   // {
   //   unsigned char obu_buff[8];
   //   memcpy(obu_buff,ptr+1,8);
   //   char anjian_obu[17];
   //   memcpy(anjian_obu,uart_convet+6,16);
   //   writeFile(fop_log, anjian_obu,14);
   //   writeFile(fop_log, NULL,16);
   //   unsigned char pos[2]={0x00,0x02};
   //   send_position(obu_buff,pos);
   // }   /***如果是四楼\一楼，则判断进出站***/
   // else if(floor_value==4||floor_value==1)
   // {
   //   char coach_char[17]={0};
   //   memcpy(coach_char,uart_convet+6,16);
   //   int i=0;
   //   int coach_temp_num=99;
   //   int couch_in_flag=0;
   //   for(i=0;i<10;i++)
   //   {
   //     if(last_obu==coach_obu[i][7])
   //     {
   //       coach_temp_num=i;
   //       break;
   //     }
   //   }
   //   if(coach_temp_num==99)
   //   {
   //     if(couch_num==9)
   //       couch_num=0;
   //     else couch_num++;
   //     coach_temp_num=couch_num;
   //     memcpy(coach_obu[coach_temp_num],ptr+1,8);
   //     couch_in_flag=1;
   //   }
   //   coach_time[coach_temp_num]=time(NULL);
   //   if(couch_in_flag==1&&floor_value==4)
   //   {     
   //     coach_flag[coach_temp_num]=0x401;
   //     writeFile(fop_log, coach_char,14);
   //     writeFile(fop_log, NULL,18);
   //     unsigned char pos[2]={0x04,0x01};
   //     send_position(coach_obu[coach_temp_num],pos);
   //   }
   //   else if(couch_in_flag==1&&floor_value==1)
   //   {     
   //     coach_pos[coach_temp_num][1]=0x01;
   //     writeFile(fop_log, coach_char,14);
   //     writeFile(fop_log, NULL,11);
   //     unsigned char pos[2]={0x00,0x01};
   //     send_position(coach_obu[coach_temp_num],pos);
   //   }

   // }
  }










  /*构造打印数组*/      /*打印rssi信息*/
  if(ptr[0] == 0x04)
  {   
    memset(data_from_uart,0,100);
    memset(uart_convet,0,200);
    data_from_uart[0] = 0x7e;
    data_from_uart[1] = p;
    memcpy(data_from_uart+2,ptr,len);/*包含校验位*/
    convet(data_from_uart, uart_convet, p);/*uart_convet里面数据是ascii*/
    //转换成ascii码后，一位变成两位

    unsigned char obu_temp[8]={0};
    memcpy(obu_temp,ptr+1,8);
    char obu_temp_1[17]={0};
    //char *obu_temp_2="00";
    convet(obu_temp,obu_temp_1,8);
    
    weight_pos(ptr[16],ptr[17],ptr[25],ptr[26],ptr[34],ptr[35],ptr[43],ptr[44],obu_temp_1);


    unsigned char obu_log[17]={0};
    memcpy(obu_log,uart_convet+6,16);
    
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
    //writeFile(fop_log, rssi, 8);  



    // printf("**************************************rssi=%s\n",rssi);
    
    // int last_obu=0;// obu最后一位
    // last_obu=ptr[8]; 
    
    // int i=0;
    // int coach_temp_num=99;
    // int couch_in_flag=0;
    // for(i=0;i<10;i++)
    // {
    //  if(last_obu==coach_obu[i][7])
    //  {
    //    coach_temp_num=i;
    //    break;
    //  }
    // }
    // if(coach_temp_num==99)
    // {
    //  if(couch_num==9)
    //    couch_num=0;
    //  else couch_num++;
    //  coach_temp_num=couch_num;
    //  memcpy(coach_obu[coach_temp_num],ptr+1,8);
    //  //coach_flag[coach_temp_num]=0x401;
    //  couch_in_flag=1;//第一次进数组的标志
    // }
    // coach_time[coach_temp_num]=time(NULL);
    
    // if(couch_in_flag==1&&floor_value==1)//第一次进数组，发进站
    // {
    //  coach_pos[coach_temp_num][1]=0x01;
    //  writeFile(fop_log,obu_log,14);
    //  writeFile(fop_log,NULL,11);
    //  unsigned char pos[2]={0x00,0x01};
    //  send_position(coach_obu[coach_temp_num],pos);
    //  return len-1;
    // }else if(couch_in_flag==1&&floor_value==4)//第一次进数组，发进站
    // {
    //  coach_pos[coach_temp_num][1]=0x01;
    //  writeFile(fop_log,obu_log,14);
    //  writeFile(fop_log,NULL,18);
    //  unsigned char pos[2]={0x04,0x01};
    //  send_position(coach_obu[coach_temp_num],pos);
    //  return len-1;
    // }
    // int nums=0;//路由数
    // nums=getRssi(coach_temp_num,data_from_uart);
    // if(getPosvalue(coach_temp_num,nums))//达到权值，判断位置
    // {
    //  unsigned char pos_new=getPos(coach_temp_num,coach_pos[coach_temp_num][1]);
    //  //if(pos_new!=coach_pos[coach_temp_num][1])
    //  {
    //    coach_pos[coach_temp_num][1]=pos_new;
    //    unsigned char pos[2];
    //    pos[0]=0x01;
    //    pos[1]=pos_new;
    //    unsigned char pos_log[10]={0};
    //    convet(pos, pos_log, 2);
    //    writeFile(fop_log,obu_log,14);
    //    writeFile(fop_log,pos_log,13);
    //    send_position(coach_obu[coach_temp_num],pos);
    //    memset(coach_posvalue[coach_temp_num],0,20);
    //  }
    // }
    
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
  case 17:
    ret = fprintf(fp, "  {车辆四层出战!}\n");
    break;
  case 18:
    ret = fprintf(fp, "  {车辆四层进站!}\n");
    break;
	case 19:
		ret = fprintf(fp, "[%s]:发送的数据是 %s\n",curTime,str);
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
//  printf("len = %d \n",len);
  char *dest1 = dest+len-1;
  int lenobu=strlen(obu)-1;
//  printf("lenobu = %d \n",lenobu);
  for(i=0;i<t;i++)
  {
    if(obu[lenobu]!=*dest1)
      {
//      printf("obu改变\n");
//      printf("obu[lenobu] = %c \n",obu[lenobu]);
//      printf("*dest1 = %c \n",*dest1);
      return false;
      }
    lenobu--;
    dest1--;
  }
//  printf("obu 一样没有改变\n");
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
/*****************************************************************
将rssi传入数组,返回路由个数
******************************************************************/
int getRssi(int num_in_buff,unsigned char src[])
{
  coach_rssi[num_in_buff][0]=src[18];
  coach_rssi[num_in_buff][1]=src[19];
  coach_rssi[num_in_buff][2]=src[27];
  coach_rssi[num_in_buff][3]=src[28];
  coach_rssi[num_in_buff][4]=src[36];
  coach_rssi[num_in_buff][5]=src[37];
  coach_rssi[num_in_buff][6]=src[45];
  coach_rssi[num_in_buff][7]=src[46];
  int i=0;
  int num=0;
  for(i=0;i<8;i+=2)
  {
    if(coach_rssi[num_in_buff][i]>0)
      num++;
  }
  return num;
  
}
/*****************************************************************
获取pos_value，传入该车在数组中的位置；达到定位要求，返回true，否则false
******************************************************************/
bool getPosvalue(int num_in_buff,int num)
{
  switch(num){
    case 1:
      printf("**********1个路由\n"); 
      if(coach_rssi[num_in_buff][R1]==0x12)
        coach_posvalue[num_in_buff][P108]+=2;         
      else if(coach_rssi[num_in_buff][R1]==0x13||coach_rssi[num_in_buff][R1]==0x14)
        coach_posvalue[num_in_buff][P105]+=2;                 
      else if(coach_rssi[num_in_buff][R1]==0x19)
        coach_posvalue[num_in_buff][P999]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x11)
        coach_posvalue[num_in_buff][P138]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x16)
      { 
        coach_posvalue[num_in_buff][P138]+=1;
        coach_posvalue[num_in_buff][P108]+=1;
      }
      else printf("*********无效数据，不做加权\n");
      break;
    case 2:
      printf("**********2个路由\n");
      if(coach_rssi[num_in_buff][R1]==0x12||coach_rssi[num_in_buff][R1]==0x13)
        coach_posvalue[num_in_buff][P108]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x14||coach_rssi[num_in_buff][R1]==0x18)
        coach_posvalue[num_in_buff][P105]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x15)
        coach_posvalue[num_in_buff][P110]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x11&&coach_rssi[num_in_buff][R2]==0x12)
        coach_posvalue[num_in_buff][P138]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x11)
        coach_posvalue[num_in_buff][P138]+=1;
      else if(coach_rssi[num_in_buff][R1]==0x19)
        coach_posvalue[num_in_buff][P999]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x16)
      { 
        coach_posvalue[num_in_buff][P138]+=1;
        coach_posvalue[num_in_buff][P108]+=1;
      }
      else printf("*********无效数据，不做加权\n");
      break;
    case 0x03://3个路由情况
      printf("**********3个路由\n"); 
      if(coach_rssi[num_in_buff][R1]==0x12)
        coach_posvalue[num_in_buff][P108]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x14)
        coach_posvalue[num_in_buff][P105]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x11)
        coach_posvalue[num_in_buff][P138]+=1;
      else if(coach_rssi[num_in_buff][R1]==0x16&&coach_rssi[num_in_buff][R2]==0x11&&coach_rssi[num_in_buff][R3]==0x12)            
      { 
        coach_posvalue[num_in_buff][P138]+=1;
        coach_posvalue[num_in_buff][P108]+=1;
      }
      else printf("*********无效数据，不做加权\n");
      break;
    case 0x04://4个路由情况
      printf("**********4个路由\n"); 
      if(coach_rssi[num_in_buff][R1]==0x11)
        coach_posvalue[num_in_buff][P138]+=1;
      else if(coach_rssi[num_in_buff][R1]==0x12||coach_rssi[num_in_buff][R1]==0x13)
        coach_posvalue[num_in_buff][P108]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x14||coach_rssi[num_in_buff][R1]==0x18)
        coach_posvalue[num_in_buff][P105]+=2;
      else if(coach_rssi[num_in_buff][R1]==0x16)
      {
        coach_posvalue[num_in_buff][P108]+=1;
        coach_posvalue[num_in_buff][P138]+=1;
      }
      else printf("*********无效数据，不做加权\n");
      break;            
    default: printf("*********路由个数不对，出现bug!!!!\n");
      break;
  } 
  if(coach_posvalue[num_in_buff][P138]>=APOS_VALUE138||coach_posvalue[num_in_buff][P108]>=APOS_VALUE108||coach_posvalue[num_in_buff][P105]>=APOS_VALUE105||coach_posvalue[num_in_buff][P110]>=APOS_VALUE110)
    return true;
  else return false;
}

unsigned char getPos(int num_in_buff,unsigned char pos_old)
{
  if(coach_posvalue[num_in_buff][P138]>=APOS_VALUE138)
  {
    return 0x38;
  }
  else if(coach_posvalue[num_in_buff][P108]>=APOS_VALUE108)
  {
    return 0x08;
  }
  else if(coach_posvalue[num_in_buff][P105]>=APOS_VALUE105)
  {
    return 0x05;
  }
  else if(coach_posvalue[num_in_buff][P110]>=APOS_VALUE110)
  {
    return 0x10;
  }
  else return false;

}
void send_position(unsigned char obu_buf[8],unsigned char pos[2])
{
  /*队列*/
  f_head=0xf2;/*帧头*/
  data_attr=0x8888;/*属性*/
  dev_id = real_device_id;/*设备id*/
  data_num++;/*流水号*/
  data_rec=0xc0a80002;  /*接收方的ip地址192.168.0.2*/       
  f_end=0xf1;
  if(data_num==65535)
  {
    data_num=0x01;
  }
  time_t timep;
  time(&timep);
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
  /*封装头*/ 
  memcpy(pos_buff+14,obu_buf,8);
  pos_buff[22]=0x00;
  pos_buff[23]=0x00;
  memcpy(pos_buff+24,pos,2);    
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
  int i;
  printf("position data is : ");
  for(i=0;i<33;i++)
    printf("%x",pos_buff[i]);
  printf("\n");
}
int zhuanyi(char *src,char *dest,int length)
{
	int i=0;
	int j=0;
	src[j++]='F';
	src[j++]='2';
	for(i=2;i<length-2;)
	{
		if(src[i]=='F'&&src[i+1]==0)
		{	
			dest[j++]='F';
			dest[j++]='0';
			dest[j++]='0';
			dest[j++]='2';
			i+=2;
		}	
		else if(src[i]=='F'&&src[i+1]==1)
		{	
			dest[j++]='F';
			dest[j++]='0';
			dest[j++]='0';
			dest[j++]='1';
			i+=2;
		}	
		else if(src[i]=='F'&&src[i+1]==2)
		{	
			dest[j++]='F';
			dest[j++]='0';
			dest[j++]='0';
			dest[j++]='0';
			i+=2;
		}
		else dest[j++]=src[i++];
	}
	dest[j++]=src[i++];
	dest[j++]=src[i++];
	return j/2;
}



