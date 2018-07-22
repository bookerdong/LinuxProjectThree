/*receive.c  没有线程*/
#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix标准函数定义*/
#include     <sys/types.h>  /**/
#include     <sys/stat.h>   /**/
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX终端控制定义*/
#include     <errno.h>      /*错误号定义*/
#include     <string.h>
#include     <pthread.h>
#define  TRUE 0
#define  FALSE -1
/***@brief  设置串口通信速率
*@param  fd     类型int  打开串口的文件句柄
*@param  speed  类型int  串口速度
*@return  void*/

// int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	    // B38400, B19200, B9600, B4800, B2400, B1200, B300, };
// int name_arr[] = {115200,38400,  19200,  9600,  4800,  2400,  1200,  300,
	    // 38400,  19200,  9600, 4800, 2400, 1200,  300, };
	  
int speed_arr[] = {B921600,B460800,B230400,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300 };
int name_arr[] = {921600,460800,230400,115200,38400,  19200,  9600,  4800,  2400,  1200,  300};

	int	fd;  //打开串口的文件句柄
	int nread,nreads;
	//设置接收缓冲区的大小
	char rbuff[2048];
	//可以一个一个字节的接收 
	char uart_0=0;
	
	
//设置波特率
void set_speed(int fd, int speed)
{
  int   i;
  int   status;
  struct termios   Opt;
  tcgetattr(fd, &Opt);
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
   {
   	if  (speed == name_arr[i])
   	{
   	    tcflush(fd, TCIOFLUSH);  //清空串口的缓冲区 
    	cfsetispeed(&Opt, speed_arr[i]);
    	cfsetospeed(&Opt, speed_arr[i]);
    	status = tcsetattr(fd, TCSANOW, &Opt);
    	if  (status != 0)
            perror("tcsetattr fd1");
     	return;
     	}
   		tcflush(fd,TCIOFLUSH);//清空串口的缓冲区 
   }
}
/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型 int  打开的串口文件句柄*
*@param  databits 类型 int 数据位  取值为7 或者*
*@param  stopbits 类型 int 停止位  取值为1 或者*
*@param  parity  类型 int  效验类型取值为N,E,O,,S
*/
//此类是封好的比较完整的类 可以直接使用 
static int set_Parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	
	if(tcgetattr(fd,&options)!=0) 
	{ 
		perror("SetupSerial 1");     
		return(FALSE);  
	}
	options.c_cflag &= ~CSIZE;
	
	switch (databits) /*设置数据位数*/
	{   
		case 7:		
			options.c_cflag |= CS7; 
			break;
		case 8:     
			options.c_cflag |= CS8;
			break;   
		default:    
			fprintf(stderr,"Unsupported data size\n"); return (FALSE);  
	}
	//设置校验类型
	switch (parity) 
	{   
		case 'n':
		case 'N':    
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
			break;  
		case 'o':   
		case 'O':     
			options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
			options.c_iflag |= INPCK;             /* Disnable parity checking */ 
			break;  
		case 'e':  
		case 'E':   
			options.c_cflag |= PARENB;     /* Enable parity */    
			options.c_cflag &= ~PARODD;   /* 转换为偶效验*/     
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S': 
		case 's':  /*as no parity*/   
	    		options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;break;  
		default:   
			fprintf(stderr,"Unsupported parity\n");    
			return (FALSE);  
	} 
	 
	//设置停止位  
	switch (stopbits)
	{   
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
	   		break;
		default:    
		 	fprintf(stderr,"Unsupported stop bits\n");  
		 	return (FALSE); 
	} 
	
	/* Set input parity option */ 
	if (parity != 'n')options.c_iflag |= INPCK ; 
	//清bit位  关闭字符映射 0x0a 0x0d
	options.c_iflag &= ~(INLCR|ICRNL);
	//清bit位  关闭流控字符 0x11 0x13 
	options.c_iflag &= ~(IXON);
	
	//需要注意的是:
	//如果不是开发终端之类的，只是串口传输数据，而不需要串口来处理，那么使用原始模式(Raw Mode)方式来通讯，设置方式如下： 
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   /*Output*/

	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME]=100;//设置超时10秒
	options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
	{ 
		perror("SetupSerial 3");   
		return (FALSE);  
	} 
	return (TRUE);  
}
/**
*@breif 打开串口
*/
int OpenDev(char *Dev)
{
	int	fd = open( Dev, O_RDWR| O_NOCTTY | O_NDELAY );         //
	if (-1 == fd)
		{ /*设置数据位数*/
			perror("Can't Open Serial Port");
			return -1;
		}
	return fd;
}

// Function: Thread signals when timer has expired
void *uartrcv(void *arg){

	// Run while loop until timer has expired 
	while(1)
	{
  		//如果串口缓冲区中有数据 
		if((nread = read(fd,rbuff,2048))>0)
   		{
			printf("---------------------------------------------------nread = %d\n",nread);
   			//如果是字符串用此方法显示 
     	  	// printf("\n%s\n",rbuff);
			for(int i = 0;i < nread;i++)
			{
				printf("%d ",rbuff[i]);
			}
    
    		//清空串口的缓冲区 
			tcflush(fd,TCIFLUSH);
			//清空数组 
      		bzero(rbuff,2048);
   	 	}   
   	 	else
   	 	{
	    	usleep(1); 
   	 	}
		
	} // End while statement
	pthread_exit(NULL);
}

/**
*@breif 	main()
*/
//主函数 
int main(int argc, char **argv)
{
	//发送一组16进制的数 
	char wbuff[1024];

	//线程相关
	pthread_t thread1;
	
	//COM1串口文件	
	char *dev ="/dev/ttyUSB0";
	//打开串口文件 
	fd = OpenDev(dev);
	
	//如果文件打开成功 
	if (fd>0)
	{
    	set_speed(fd,921600);//设置波特率	
	}
	else
	{
		printf("Can't Open Serial Port!\n");
		exit(0);
	}
	
	//设置串口的属性 
	if (set_Parity(fd,8,1,'N')== FALSE)
    {
    	printf("Set Parity Error\n");
    	exit(1);
  	}
	
	//创建线程
	pthread_create(&thread1, NULL, uartrcv, NULL);
  	
	//控制器打印字符串
	// wbuff[0]=0x7e;
	// wbuff[1]=0x05;
	// wbuff[2]=0x01;
	// wbuff[3]=0x08;
	// wbuff[4]=0x0e;
	// wbuff[5]=0xff;
	// wbuff[6]=0xff;
	// wbuff[7]=0x7f;

	//获取全部设备信息
	wbuff[0]=0x7e;
	wbuff[1]=0x07;
	wbuff[2]=0x00;
	wbuff[3]=0x01;
	wbuff[4]=0x00;
	wbuff[5]=0x00;
	wbuff[6]=0xff;
	wbuff[7]=0xff;
	wbuff[8]=0x7f;
  	//循环接收串口的数据 
    while(1)
  	{
		nreads=write(fd,wbuff,sizeof(wbuff));
		sleep(2);
   	}
	pthread_join(thread1, NULL);
   	//关闭串口文件 
    close(fd);
    exit(0);
}

