#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "path.h"

char curr_file = 0;

char audio_player_path[]="/usr/bin/omxplayer\", \" ";

unsigned char check_status; 
int pid;
int xpid;
av_t av[] = 
{
    AV_NAMASKAR, "/home/pi/MI_Working/try_Pratham_OBC_23012014/namaskar.mp4",
    AV_USER_IN,  "/home/pi/MI_Working/try_Pratham_OBC_23012014/request.mp3",
	AV_OBSTACLE, "/home/pi/MI_Working/try_Pratham_OBC_23012014/obstacle.mp3",
	AV_RUNTIME, "/home/pi/MI_Working/try_Pratham_OBC_23012014/runtime.mp4",
	AV_ONE, 	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/main.mp4",
	AV_TWO, 	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/node1.mp4",
	AV_THREE,	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/node2.mp4",
	AV_FOUR,	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/node3.mp4",
	AV_FIVE,	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/node4.mp4",
	AV_SIX,	     "/home/pi/MI_Working/try_Pratham_OBC_23012014/node5.mp4",
    AV_SEVEN,	 "/home/pi/MI_Working/try_Pratham_OBC_23012014/node6.mp4",
};



path_t path[] = 
{
//	{ CONNECT, 0, 0},
// first command should come after connect
	{ SET_HEADING, 0,236 },
	{ MOVE, FWD, 174 },
	{ GET_NODE_ID, 0, 0},
	{ PLAY_AV, AV, AV_TWO },

	{ SET_HEADING, 0, 111},	
	{ MOVE, FWD, 110},
//	{ GET_NODE_ID, 0, 0},
//	{ PLAY_AV, AV, AV_THREE },

	{ SET_HEADING, 0,2},
	{ MOVE, FWD, 76},
	{ GET_NODE_ID, 0, 0},
	{ PLAY_AV, AV, AV_THREE },
	{ SET_HEADING, 0, 233},
	{ MOVE, FWD, 75},
	{ SET_HEADING, 0, 298},
	{ MOVE, FWD, 137},
//	{ GET_NODE_ID, 0, 0},
//	{ PLAY_AV, AV, AV_FIVE},

	{ SET_HEADING, 0, 355},
	{ MOVE, FWD, 178},
	{ GET_NODE_ID, 0, 0},
	{ PLAY_AV, AV, AV_FOUR},

	{ SET_HEADING, 0, 236},
/*	//{ SET_HEADING, 0, 117},
	{ MOVE, FWD ,117},

	{ GET_NODE_ID, 0, 0},
	{ PLAY_AV, AV, AV_FIVE},

	{ SET_HEADING, 0, 358},
	{ MOVE, FWD, 157},

	{ GET_NODE_ID, 0, 0},
	{ PLAY_AV, AV, AV_SIX},

	{ SET_HEADING, 0, 237},
/*
//	{ MOVE, FWD, 26},
	{ SET_HEADING, 0, 55},
	{ MOVE, FWD, 57},
	{ SET_HEADING, 0, 149},
	{ MOVE, FWD, 90},
	{ SET_HEADING, 0, 320},*/
//	{ MOVE, FWD, 92},
//last commands
	{ IDLE, 0, 0},
	{ END, '0', 0}
};

void play_av(void)
{
	pid = fork();
	xpid=pid;
	if(pid == 0)
	{
           setsid();
		execlp("/usr/bin/omxplayer", " ", av[curr_file].filepath,NULL);				
	}
}

unsigned char verify_packet(unsigned char *buffer)
{
	char i = 0;
	char chksum = 0;
	char len;

	if (buffer[0] == 0xAA)
	{
		len = buffer[1] ;
		
		if(buffer[len + 3] == 0x55)		//kp
		{
			for(i=2; i<(len+2);i++)		//kp
			{
				chksum += buffer[i];
			}

			if(chksum == buffer[len+2])	//kp
			{
				return 1;	
			}	
		}
	}
	return 0;
}
int prepare_packet (unsigned char index, unsigned char *packet)
{
	int i = 0, j;
	unsigned char chksum = 0;
	unsigned char cmd;
	
	cmd = path[index].cmd;
//	printf(" %x ",cmd);
	if(cmd == END)
	{
		return -1;
	}
	if((cmd == PLAY_AV))//||(cmd == PLAY_VIDEO))
	{
		printf("\nAudioPlay");	
		curr_file = path[index].data;
			
		return 0x80;	
	}
	if((cmd == CONNECT)||(cmd == IDLE))
	{
		
		packet[i++] = 0xAA;
		packet[i++] = 1;
		//chksum = chksum + packet[i++];
		packet[i++] = path[index].cmd;
		chksum = path[index].cmd;//chksum + packet[i++];
		packet[i++] = chksum;
		packet[i++] = 0x55;
	}
	else if(cmd == SET_HEADING)
	{
		packet[i++] = 0xAA;
		packet[i++] = 3;
		//chksum = chksum + packet[i++];

#if LSB_FIRST	
		
		packet[i] = path[index].cmd;
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data);
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data>>8);
		chksum = chksum + packet[i++];
#else
		packet[i] = path[index].cmd;
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data >> 8);
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data);
		chksum = chksum + packet[i++];
#endif
		packet[i++] = chksum;
		packet[i++] = 0x55;
	}
	else if(cmd == GET_NODE_ID)
	{
      	int i = 0, j;
    	unsigned char chksum = 0;
    	packet[i++] = 0xAA;
    	packet[i++] = 1;
    	packet[i] = path[index].cmd;
    	chksum += packet[i++];
    	packet[i++] = chksum;
    	packet[i++] = 0x55;
    	return 0x90;	

    }
	else if(cmd != END)
	{
		packet[i++] = 0xAA;
		packet[i++] = 4;//sizeof(path_t) + 2;
		//chksum = chksum + packet[i++];
#if LSB_FIRST	
		/*for(j = 0; j < sizeof(path_t); j++)
		{
			packet[i] = *bptr++;
			chksum = chksum + packet[i++];
		}*/
		
		packet[i] = path[index].cmd;
		chksum = chksum + packet[i++];
		packet[i] = path[index].dir;
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data);
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data>>8);
		chksum = chksum + packet[i++];
#else
		packet[i] = path[index].cmd;
		chksum = chksum + packet[i++];
		packet[i] = path[index].dir;
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data >> 8);
		chksum = chksum + packet[i++];
		packet[i] = (unsigned char)(path[index].data);
		chksum = chksum + packet[i++];
#endif
		packet[i++] = chksum;
		packet[i++] = 0x55;
	}
	return i;
}
int prepare_pl_packet(unsigned char *packet)
{
	int i = 0, j;
	unsigned char chksum = 0;
	packet[i++] = 0xAA;
	packet[i++] = 2;
	packet[i] = SET_ROBO_MODE;
	chksum += packet[i++];
	packet[i] = LEARNING;
	chksum += packet[i++];
	packet[i++] = chksum;
	packet[i++] = 0x55;
	return i;	
}
int prepare_stop_packet(unsigned char *packet)
{

	int i = 0, j;
	unsigned char chksum = 0;
	packet[i++] = 0xAA;
	packet[i++] = 2;
	packet[i] = SET_ROBO_STATE;
	chksum += packet[i++];
	packet[i] = STOP;
	chksum += packet[i++];
	packet[i++] = chksum;
	packet[i++] = 0x55;
	return i;	
}
