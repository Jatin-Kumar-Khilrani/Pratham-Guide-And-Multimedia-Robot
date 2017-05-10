#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //Used for UART
#include <fcntl.h> //Used for UART
#include <termios.h> //Used for UART
#include <signal.h>
#include "path.h"
#include "GPIO.h"

int get_serial_pkt_frm_robo();
void clr_serial_data(void);
void stop_audio(void);

enum
{
	AUTHENTICATION,
	WELCOME,
	SERVO_REQ,
	SERVO_CMP_ACK,
	USER_IN,
	AV_PLAY,
	WAIT_AV,
	PATH_CMDS,
	RX_ACK,
	RX_DONE,
	OBC_IDLE,
	PATH_READING,
};

#define SCH_TICK (50)
#define ACK_TIMEOUT (15*100/SCH_TICK) //(100/SCH_TICK)		 //100ms timeout
#define COM_LOSS_TIMEOUT (5*60*(1000/SCH_TICK))  //5 seconds timeout 

unsigned char path_packet[10];
extern char curr_file;
extern int pid;
extern int xpid;
extern unsigned char check_status;

//Setting Up The UART

//-------------------------
//----- SETUP USART 0 -----
//-------------------------
//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
int uart0_filestream = -1;

unsigned char rx_buffer[256];
static int rx_length=0;
int last_rsp_f=0; 
unsigned char last_rsp;

int read_len = 6;

struct termios options;
int main()
{
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	
	int pi_state = AUTHENTICATION;
	int pi_nxt_state = USER_IN;
	int button_pressed = 1 , complete = 0;
	
	//int cs = 1,ls = 1,val = 0;
	/* Enable GPIO PINS */

	if((-1 == GPIOExport(RFID_IN))|| (-1 == GPIOExport(VISIT_SWITCH)) || (-1 == GPIOExport(DBG_LED)) || (-1 == GPIOExport(PWR_OFF_SWITCH)) ||(-1 == GPIOExport(SERVO_REQ_PIN))||(-1 == GPIOExport(LEARNING_MODE_SW))||(-1 == GPIOExport(SERVO_CMP_PIN)))
		return (1);

	/* Set GPIO Direction */
	if((-1 == GPIODirection(RFID_IN, IN)) || (-1 == GPIODirection(VISIT_SWITCH, IN)) || (-1 == GPIODirection(DBG_LED, OUT)) ||(-1 == GPIODirection(PWR_OFF_SWITCH, IN)) || (-1 == GPIODirection(SERVO_REQ_PIN, OUT)) ||(-1 == GPIODirection(LEARNING_MODE_SW, IN))||(-1 == GPIODirection(SERVO_CMP_PIN, IN)))
		return (2);

	//OPEN THE UART
	//The flags (defined in fcntl.h):
	// Access modes (use 1 of these):
	// O_RDONLY - Open for reading only.
	// O_RDWR - Open for reading and writing.
	// O_WRONLY - Open for writing only.
	//
	// O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	// if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	// immediately with a failure status if the output can't be written immediately.
	//
	// O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);// | O_NDELAY); //Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART. Ensure it is not in use by another application\n");
	}
	//CONFIGURE THE UART
	//The flags (defined in termios.h - see http://pubs.opengroup.org/onlinepubs/00 ... ios.h.html):
	// Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	// CSIZE:- CS5, CS6, CS7, CS8
	// CLOCAL - Ignore modem status lines
	// CREAD - Enable receiver
	// IGNPAR = Ignore characters with parity errors
	// ICRNL - Map CR to NL on input
	// PARENB - Parity enable
	// PARODD - Odd parity (else even)
	//struct termios options;
	tcgetattr(uart0_filestream, &options);
	cfsetispeed(&options, B9600); //<Set baud rate
	cfsetospeed(&options, B9600); //<Set baud rate
	cfmakeraw(&options);
	options.c_cflag |= B9600 | CS8 | CLOCAL | CREAD; //<Set baud rate
	options.c_iflag |= IGNPAR | ICRNL  ;
	options.c_oflag = 0;
	options.c_cflag &= ~CRTSCTS;
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
	
	//Transmitting Bytes

	//----- TX BYTES -----
	unsigned char tx_buffer[20];
	unsigned char *p_tx_buffer;

	p_tx_buffer = &tx_buffer[0];
	*p_tx_buffer++ = 'H';
	*p_tx_buffer++ = 'e';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'l';
	*p_tx_buffer++ = 'o';
	if(uart0_filestream!=-1)
	{
		int count = write(uart0_filestream,&tx_buffer[0],(p_tx_buffer-&tx_buffer[0]));
		if(count<0)
			{
				printf("Uart tx error");
			}
	}
		
		int timeout = 0;
		int length,rfid_scan_done;
		int resp = 0;
		unsigned char path_index=0;
		unsigned char bptr[20];
		int status;//,result;
		pid_t result;
		int val=0;
		char pl_flag = 0;
		GPIOWrite(SERVO_REQ_PIN,1);
		char check_status = 0;
		unsigned char node_id[5] = { 0xF1, 0xF2, 0xF3, 0xF4, 0xF5};
		int node_indx=0;
		static int retry_cnt = 0, wait4node_id = 0;
		int count = 0,count_pl = 0,count_rfid= 0;
		//usleep(10000);
		while(1)
		{
			//val^=1;
			if(count++ > 60)   //3sec
			{
			     val^=1;
                 count = 0;
    			 GPIOWrite(DBG_LED,val);
    			 if(!(GPIORead(PWR_OFF_SWITCH)))
    			 {
    				break;
    			//	pi_state = USER_IN;
    				printf("pwr off ");
    			 }
            }
			//wait for 20 msec
			usleep(50000);
			
			resp = get_serial_pkt_frm_robo();
			if (resp>0)
			{
				if(((resp == ACK)||(resp == NACK)||(resp == 0xF1)||(resp == 0xF2)||(resp == 0xF3)||(resp == 0xF4)||(resp == 0xF5)||(resp == 0xFF)||((resp >= 0x10) && (resp < CMD_MAX))))
				{
					last_rsp = resp;
					last_rsp_f = 1;	
				}	
			}
			else			//we did not receive anything for last 3 sec, time to reset
			{
			#if 0
				usleep(100);
				if(timeout++> 30000))
				{
					timeout = 0;
					printf("timeout ");
					timeout = 0;	
					clr_serial_data();
				}
			#endif
								
			}
			if(pi_state == RX_DONE)
       		{	
                result = waitpid(pid,&status,WNOHANG);
             	if((result!=0)&&(result!=1))
                {
                   curr_file = AV_RUNTIME;
                   play_av();           
                }
            }
			switch(pi_state)
			{
				case AUTHENTICATION:
					//Wait for RFID authentication
					rfid_scan_done = 0;
					if(count_rfid++ >= 40)        //2sec
					{
                         count_rfid = 0;
                         rfid_scan_done = (!(GPIORead(RFID_IN)));
                    }
				//	rfid_scan_done = 1;
					path_index = 0;
					clr_serial_data();
					if(rfid_scan_done)
					{
						pi_state = WELCOME;
						printf("AUTHENTICATION\n");
					}
					if(count_pl++>40)              //2sec
					{
                         count_pl = 0;
    					if(!(GPIORead(LEARNING_MODE_SW)))
    					{
    						length = prepare_pl_packet(bptr);
    						if(length>0)
    						{
    							pi_state = RX_ACK;//PATH_READING;
    							pl_flag = 1;
    							printf("sent");
    							clr_serial_data();
    							usleep(20000);
    							write(uart0_filestream, bptr, length);
    						}
    						else 
    						{
    							pi_state = AUTHENTICATION;
    						}			
    					}
                  }
						
				break;

				case WELCOME:
					//Play welcome msg
					curr_file = AV_ONE;//AD_WELCOME;
					pi_state = AV_PLAY;
					pi_nxt_state = SERVO_REQ;//USER_IN;
					printf("WELCOME\n");
					//Press GUIDE button when you r ready
				break;
				case SERVO_REQ:
					GPIOWrite(SERVO_REQ_PIN,0);
					pi_state = SERVO_CMP_ACK;
					printf("servo_req");
					usleep(200000);
					curr_file = AV_NAMASKAR;
					usleep(3000000);
                    play_av();
					GPIOWrite(SERVO_REQ_PIN,1);
				break;
				case SERVO_CMP_ACK:
					if(!(GPIORead(SERVO_CMP_PIN)))
					{
						printf("servo done");
						pi_state = USER_IN;
					}
				break;
				case USER_IN:
					button_pressed = (!(GPIORead(VISIT_SWITCH)));
					if(button_pressed)
					{
						printf("USER_IN\n");
						path_index = 0;
						pi_state = PATH_CMDS;
					}
					else
					{
                        result = waitpid(pid,&status,WNOHANG);
    					if((result!=0)&&(result!=1))
    					{
                            curr_file = AV_USER_IN;
    						printf("\nAV COMPLETE\n");
    						play_av();
    					//	pi_state = pi_nxt_state;
    					}	
                    }

				break;
				case AV_PLAY:
                 result = waitpid(pid,&status,WNOHANG);
                 	if((result!=0)&&(result!=1))
		            {
                    }
                    else
                    {
                        stop_audio();
                    }
					play_av();
					//usleep(2000000);
					pi_state = WAIT_AV;
				break;
				case WAIT_AV:
					result = waitpid(pid,&status,WNOHANG);
				//	usleep(2000000);
				//	system("echo -n q >/usr/bin/omxplayer");
					if((result!=0)&&(result!=1))
					{
						printf("\nAV COMPLETE\n");
				/*		if(pi_nxt_state == RX_DONE)
						{
                                 curr_file = AV_RUNTIME;
                                 play_av();           
                        }*/
						pi_state = pi_nxt_state;
					}	
				break;				

				case PATH_CMDS:
					//read path from table and send to robot till eof
					printf("** %d  **\n",path_index);
				
				
				
					length = prepare_packet(path_index, bptr);
					if (length > 0)
					{
						if (length == 0x80)
						{
                            path_index++;
                            pi_state = AV_PLAY;
							pi_nxt_state = PATH_CMDS;
						}
						else if(length == 0x90)
						{
                            wait4node_id = 1;
                   			clr_serial_data();
							usleep(20000);		//added by kp
							write(uart0_filestream, bptr, length);
							usleep(20000);
							check_status = 1;         //added  by KP
							path_index++;
							pi_state = RX_ACK;
							printf("node id ");
						}
						else if(length)
						{
							clr_serial_data();
							usleep(20000);		//added by kp
							write(uart0_filestream, bptr, length);
							if(bptr[3] == FWD)
							{
                               result = waitpid(pid,&status,WNOHANG);
                             	if((result!=0)&&(result!=1))
					            {

                                   curr_file = AV_RUNTIME;
                                   play_av();           
                                }
                            }
							usleep(20000);
							path_index++;
							pi_state = RX_ACK;
						}
					}
					else
					{
                        stop_audio();
						path_index = 0;
						pi_state = OBC_IDLE;
						printf("\nall path completed\n");
					}
				break;

				case RX_ACK:
					if (last_rsp_f == 1)
					{
						last_rsp_f == 0;
                        
                            if(last_rsp == ACK)
    						{
    							pi_state = RX_DONE;
    							printf("ACK\n");
    						//	clr_serial_data();
    						}
    						else if(last_rsp == NACK)
    						{
    							//resend
    							path_index--;
    							pi_state = PATH_CMDS;
    							printf("NACK\n");
    							clr_serial_data();
    						}
    						//if something unexpected??
    						else if((last_rsp >= 0x10) && (last_rsp < CMD_MAX))
    						{
    							
    							if(last_rsp == CMD_IP)
    							{
                                               
    								pi_state = RX_DONE;
    								printf("\nIP");
    							}
    				      		else if(timeout++ >= ACK_TIMEOUT)
    							{
    								timeout = 0;
    								path_index--;
    								pi_state = PATH_CMDS;
    								printf("\ntimeout\n");
    							}
    							else
    							{
    								printf("Unexpected");
    								//clr_serial_data();
    							}
    
    
    						}
    						else
    						{
    								clr_serial_data();
    						//	printf("Cound not decode packet");
    						}
					}
					
				break;

				case RX_DONE:
					//printf(" %x last rsp",last_rsp);
					if (last_rsp_f == 1)
					{
					//	printf("last rsp %d",last_rsp);
						last_rsp_f == 0;
						
                        if(check_status == 1)
                        {
                             if(last_rsp >= 0xF1)
						     {
                                 check_status = 0;
    				             printf(" %x in chk ",last_rsp);
                                 if(last_rsp == node_id[node_indx])
                                 {
                                   result = waitpid(pid,&status,WNOHANG);
                                 	if((result!=0)&&(result!=1))
    					            {
    
                                    }
                                    else
                                    {
                                        stop_audio();
                                    }
                                                                           
                                   wait4node_id = 0;
                                   retry_cnt = 0;
                                   pi_state = PATH_CMDS;
                                   node_indx++;
                                 }
                                 else 
                                 {
                                       pi_state = PATH_CMDS;
                                       path_index++;
                                       node_indx++;
                                 }
                             }
                        }
                        else
                        {                           
    						if((last_rsp >= 0x10) && (last_rsp < CMD_MAX))
    						{
    						
    							if(pl_flag == 1)
    							{
    								if(last_rsp == CMD_IDLE)
    								{
    									pi_state = AUTHENTICATION;
    									pl_flag = 0;
    								}
    
    
    							}
    							else	
    							{	
    								if(last_rsp == CMD_CMPLT)
    								{
										check_status = 0;      //added by KP
    									pi_state = PATH_CMDS;
    									printf("\ncmd cmplt");
    									clr_serial_data();
    								}
    								else if(last_rsp == CMD_IDLE)	//added by KP
    								{
                                         stop_audio(); 
    									printf("\n all path completed");
    									pi_state = AUTHENTICATION;
    									node_indx = 0;
    								}
    								else if(last_rsp == CMD_OBSTACLE_DETECTED)
    								{
                                        stop_audio();
           	                            //usleep(100000);
                                       	curr_file = AV_OBSTACLE;//AD_WELCOME;
                                        pi_state = AV_PLAY;
                                        pi_nxt_state = RX_DONE;//USER_IN;
                                        //obs_detected = 1;
                                    }   								 
    								else
    								{
    									//printf(" waitng ");
    								}
    							}
                  			}
                       }
					}
					else
					{
                        if(wait4node_id == 1)
                        {
			//	printf(" waiting ");
                                        if(timeout++>=30)
                                        {
                                                         printf(" %d retry",retry_cnt);
                                                         timeout = 0;
                                                         retry_cnt++;
                                                         if(retry_cnt>4)
                                                         {
                                                             retry_cnt = 0;           
                                                             pi_state = PATH_CMDS;
                                                             wait4node_id = 0;
                                                          //lets go to next command, nothing to do here              
                                                         }
                                                         else
                                                         {
                                                             wait4node_id = 0;
                                                             path_index--;
                                                             pi_state = PATH_CMDS;
                                                         }
                                        }
                        }
                    }
				break;
				case PATH_READING:
					if (last_rsp_f == 1)
					{
						last_rsp_f == 0;
						resp = last_rsp;

					}	
					else
					{
						resp = NO_RSP;
					}

		//			 if(manage_path_learning(resp) == PL_COMPLETE)
		//			{
		//				pi_state = AUTHENTICATION;
		//			}
				break;
				case OBC_IDLE:
										
					usleep(10000);
					if(timeout++>10)
					{
						timeout = 0;
						printf("pi Idl ");
						clr_serial_data();
						usleep(20000);
					}					
				break;
			}
		}
		length = prepare_stop_packet(bptr);
   		usleep(20000);		//added by kp
        write(uart0_filestream, bptr, length);
  		usleep(20000);		//added by kp
		write(uart0_filestream, bptr, length);
    	usleep(20000);
    	system("sudo reboot");
	
}

void clr_serial_data(void)
{				
	//tcsetattr(uart0_filestream, TCSAFLUSH, &options);
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);// | O_NDELAY); //Open in non blocking read/write mode
	tcsetattr(uart0_filestream, TCSAFLUSH, &options);
	usleep(10000);
	tcflush(uart0_filestream, TCIOFLUSH);
	usleep(10000);
	rx_length = 0;
	memset(rx_buffer,0, sizeof(rx_buffer));
	last_rsp_f = 0; //FALSE;
	
}
int get_serial_pkt_frm_robo()
{
	unsigned char temp[10];
	int len;
	

	//----- CHECK FOR ANY RX BYTES -----
	if (uart0_filestream != -1)
	{
		// Read up to 255 characters from the port if they are there
		len = read(uart0_filestream, (void*)temp, read_len); //Filestream, buffer to store in, number of bytes to read (max)
		if (len < 0)
		{
			//An error occured
			printf("%s\n", strerror(errno));
		//	printf("UART RX error\n");
			clr_serial_data();			//tt
		}
		else if (len == 0)
		{
			//printf("No data waiting\n");
			//No data waiting
		}
		else if (len > 0)
		{
			//Bytes received
			memcpy(&rx_buffer[rx_length],temp,len);
			rx_length += len;
			if(rx_length>=read_len)	//6
			{
				temp[len] = '\0';
				if(verify_packet(rx_buffer))
				{
					unsigned char cmnd=rx_buffer[3];
					
					//store in 
					//memcpy(data_pkt,rx_buffer,rx_length);
					rx_length = 0;
					memset(rx_buffer,0,sizeof(rx_buffer));
					return cmnd;
				}	
				else
				{
			//		printf("not verify");
					rx_length = 0;
					memset(rx_buffer,0,sizeof(rx_buffer));
				}
			}
		}
		
	}

	return -1;
}
void stop_audio()
{
     usleep(1000000);
     killpg(pid,SIGKILL);
     usleep(1000000);
   /* signal(SIGQUIT, SIG_IGN);
    usleep(1000000);
    kill(0, SIGQUIT); 
    usleep(1000000); */
}

/*char manage_path_learning(int resp)
{

switch(pl_state)
{
	case PL_RSP_WAIT:
	break;

}
		
}*/
/*
char timeout()
{
	static int timeout_s = 0, timeout=0;
	if(timeout_s++>10000)
	{
		timeout_s = 0;

		if(timeout++>3)
		{
			timeout = 0;
		}
	}
}
*/
