#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

char wbuf[50]={0};
char rbuf[1000]={0};
static int fd = -1; 
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
/* 五个参量 fd打开文件 speed设置波特率 bit数据位设置   neent奇偶校验位 stop停止位 */
    struct termios newtio,oldtio;
    if ( tcgetattr( fd,&oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;
    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
    break;
    case 8:
        newtio.c_cflag |= CS8;
    break;
    }
    switch( nEvent )
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }
switch( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
         cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if( nStop == 1 )
        newtio.c_cflag &= ~CSTOPB;
    else if ( nStop == 2 )
    newtio.c_cflag |= CSTOPB;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        perror("com set error");
        return -1;
    }
    printf("set done!\n");
    return 0;
}

//串口接收函数
void *ser_rece(void *arg)
{
	int n = 0;
	while(1)
	{
		n = read(fd,rbuf,100);
		if(n > 0)
		{
			printf("n > 0\n");
		}
		else
		{
			printf("n < 0\n");
		}
		sleep(5);
	} 
	pthread_exit(NULL);
	
}

int main(void)
{
	
	
	pthread_t thread1;
	
	fd = open("/dev/ttyUSB0",O_RDWR|O_NOCTTY|O_NDELAY);
	if(fd < 0)
	{
		perror("open");
	}
	
	set_opt(fd,921600,8,'N',1);
	
	pthread_create(&thread1, NULL,ser_rece, NULL);
	wbuf[0] = 0x7e;
	wbuf[1] = 0x05;
	wbuf[2] = 0x01;
	wbuf[3] = 0x08;
	wbuf[4] = 0x0e;
	wbuf[5] = 0xff;
	wbuf[6] = 0xff;
	wbuf[7] = 0x7f;
	while(1)
	{
		int n = write(fd,wbuf,8);
		if(n < 0)
		{
			perror("send");
		
		}
		printf("send\n");
		sleep(5);
	}
	
	
	pthread_join(thread1, NULL);
	close(fd);
	return 0;
}