#ifndef __PATH_H__
#define __PATH_H__

typedef enum
{
	DEFAULT_CMD,
	SET_SPD,
	SET_ROBO_MODE,
	SET_ROBO_STATE,
	SET_PID_GP,
	SET_PID_GD,
	SET_PID_GI,

	MOVE,
	ROTATE,
	DSBL_SONAR,
	SET_MAINTAINENCE_MODE,
	CONNECT,	//added by KP
	IDLE,		//added by KP
	SET_HEADING,
	UPLOAD,
	GET_NODE_ID,		//Send RF node ID from ARM to OBC
	SET_NODE_ID,
	MAX_CMD,
	//till this point all the commands handled by robot

	//command handled by pi will start from 0x20
	PLAY_AV = 0x20,
	//PLAY_VIDEO,

	END = 0xFF
}OBC_cmd_t;

typedef enum
{
	STOP,
	FWD,
	BACKWD,
	ROT_CLK,
	ROT_ANT_CLK,

	FWD_PLUS_ROT_CLK,
	FWD_PLUS_ROT_ANT_CLK,

	KILL_MOT,
	MAINTAINENCE,

	WAIT,	//added by KP
	
	//sub commands handled by pi will start from 0x10
	AV = 0x10,	
	//VIDEO,

}robot_state_t;

typedef enum
{
	PATH_DATA,
	//SONAR_DATA;


}obc_upload_t;

typedef enum
{
	//USER,
	//AUTONOMOUS
	OBC,
	REMOTE,
	LEARNING,
}robo_mode_t;

typedef enum{CMD_IP=0x10,CMD_IDLE,CMD_CMPLT,CMD_NC,CMD_CONNECTED,CMD_OBSTACLE_DETECTED,CMD_MAX}hb_state_t;

enum{ACK=0x03,NACK=0x04,NO_RSP};

typedef enum
{
AV_NAMASKAR,
AV_USER_IN,
AV_OBSTACLE,
AV_RUNTIME,
AV_ONE,
AV_TWO,
AV_THREE,
AV_FOUR,
AV_FIVE,
AV_SIX,
AV_SEVEN,
AV_EIGHT,
AV_NINE,
AV_TEN
}av_file_t;

typedef struct
{
	unsigned char cmd;
	char dir;
	short data;
	char datalen;	//kp
}path_t;

typedef struct 
{
	av_file_t indx;
	char filepath[70];
}av_t;

unsigned char verify_packet(unsigned char*);
void decode_command(unsigned char);
int prepare_packet (unsigned char, unsigned char*);
int prepare_pl_packet(unsigned char *);
int prepare_stop_packet(unsigned char *);

#endif
