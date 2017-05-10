#include<avr/io.h>
/*#include<avr/interrupt.h>*/
#include"sbit.h"
#include<util/delay.h>
#include<string.h>
#include<stdlib.h>


#define CRY_FREQ 8000000L
#define BAUD_RATE 9600
#define BAUD_VALUE ((CRY_FREQ/16/BAUD_RATE)-1)

#define PACKET_INTERVAL (300)		//in ms
#define NODE_INTERVAL (300)		//in ms

#define N1_KEY (1<<2)
#define N2_KEY (1<<3)
#define N3_KEY (1<<4)
#define N4_KEY (1<<5)
#define N5_KEY (1<<6)

#define N1 (0)
#define N2 (1)
#define N3 (2)
#define N4 (3)
#define N5 (4)

#define PACKET_LEN (6)
#define NODE_LEN (6)
//#define DATA_INDX (2)
//#define CHKSUM_INDX (3)

#define FWD_PIN  (1<<0)
#define REV_PIN  (1<<1)
#define RCK_PIN  (1<<2)
#define RACK_PIN (1<<3)

#define RS SBIT(PORTC,1)
#define EN SBIT(PORTC,0)

#define X_MID_POINT (515)
#define Y_MID_POINT (508)

#define DEAD_BAND	(10)

unsigned char node[5][NODE_LEN] = { 		{0xAA, 0x02, 0x10, 0xF1, 0x01, 0x55},
								{0xAA, 0x02, 0x10, 0xF2, 0x02, 0x55},
								{0xAA, 0x02, 0x10, 0xF3, 0x03, 0x55},
								{0xAA, 0x02, 0x10, 0xF4, 0x04, 0x55},
								{0xAA, 0x02, 0x10, 0xF5, 0x05, 0x55}};

unsigned char mov_fwd[PACKET_LEN] = {0xAA, 0x02, 0x07, 0x01, 0x08, 0x55};
unsigned char mov_rev[PACKET_LEN] = {0xAA, 0x02, 0x07, 0x02, 0x09, 0x55};

unsigned char rot_clk[PACKET_LEN] = {0xAA, 0x02, 0x07, 0x03, 0x0A, 0x55};
unsigned char rot_aclk[PACKET_LEN] = {0xAA, 0x02, 0x07, 0x04, 0x0B, 0x55};

unsigned char stop[PACKET_LEN] = {0xAA, 0x02, 0x07, 0x00, 0x07, 0x55};

void serial_init()
{
		/*UCSRC=0X86;
		UCSRB=0XC8;*/
		UCSRB|=(1<<RXEN)|(1<<TXEN);
		UCSRC|=(1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);
		UBRRL=(unsigned char) BAUD_VALUE;
		UBRRH=(unsigned char) (BAUD_VALUE>>8);
}

void send_string(unsigned char * str, uint8_t len)
{
		/*unsigned char shifts, length;
		length = strlen(str);
     	for(shifts = 0; shifts < length; shifts++)
		{
				while(!(UCSRA & (1<<UDRE)));
				UDR = str[shifts];
		}*/

		uint8_t i = 0;

		while(i<len)
		{
			while(!(UCSRA & (1<<UDRE)));
			UDR = str[i++];
		}
}

void initADC()
{
	ADMUX=(1<<REFS0);
	ADCSRA=(1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

unsigned int ReadADC(char ch)
{

	//select ADC channel ch must be 0-7
	ADMUX &=0xf0;
	ch=ch & 0b00000111;
	ADMUX|=ch;

	//start single conversion
	ADCSRA|=(1<<ADSC);

	//wait for conversion to complete
	while(!(ADCSRA & (1<<ADIF)));

	//clear ADIF by writing one to it
	//note- this is standard way of clearing bits in io as per datasheets
	//The code writes '1' but it result in setting bit to '0'!

	ADCSRA|=(1<<ADIF);
//	val=ADCL;
//	val=(ADCH<<8) | val;
	return(ADC);
}

/*void LCD_data(char c)
{
	EN=0;
	RS=1;
	asm("nop");
	asm("nop");
	EN=1;
	PORTD=c;
	asm("nop");
	asm("nop");
	EN=0;
	asm("nop");
	asm("nop");
	asm("nop");
	_delay_ms(2);
}

void LCD_cmd(char d)
{
	EN=0;
	RS=0;
	asm("nop");
	asm("nop");
	EN=1;
	PORTD=d;
	asm("nop");
	asm("nop");
	EN=0;
	asm("nop");
	asm("nop");
	asm("nop");
	_delay_ms(3);
}

void LCD_init()
{
	_delay_ms(20);
	LCD_cmd(0x3f);
	_delay_ms(5);
	LCD_cmd(0x3f);
	_delay_us(120);
	LCD_cmd(0x3f);
	LCD_cmd(0x3f);
	LCD_cmd(0x0c);
	LCD_cmd(0x01);
	LCD_cmd(0x06);
}

void LCD_cursor(char r , char col)
{
	char add;
	if (r==1)
		add=0x00+col;
	else
		add=0x40+col;
	add=add | 0x80;
	LCD_cmd(add);
}

void lcd_printf(char *str)
{
	int length,i;
	length=strlen(str);
	for(i=0;i<length;i++)
	{
		LCD_data(str[i]);
	}
}

void lcd_clear()
{
	LCD_cmd(0x01);
}*/

int main()
{

		char cs_f, ls_f=1, cs_r, ls_r=1, cs_ccw, ls_ccw=1, cs_cw, ls_cw=1;
		char cs_n1, ls_n1=1, cs_n2, ls_n2=2, cs_n3, ls_n3=1, cs_n4, ls_n4=1, cs_n5, ls_n5=1;
		unsigned int y_read,x_read;
		char str[4];
		DDRC = 0x03;
		DDRD = 0xff;//(0x03);

//		LCD_init();
	//	initADC();
		serial_init();
		/*sei();*/
/*		unsigned int val;
		lcd_printf("Welcome");
		_delay_ms(3000);
		lcd_clear();*/
		while(1)
		{
		//	x_read = ReadADC(2);
		//	y_read = ReadADC(3);
		/*	val = x_read;
			itoa(val,str,10);
			lcd_printf(str);
			val = y_read;
			itoa(val,str,10);
			lcd_printf(str);
			_delay_ms(2000);*/
		/*	if(((x_read >= (X_MID_POINT + DEAD_BAND))&&(y_read >= (Y_MID_POINT - DEAD_BAND))&&(y_read <= (Y_MID_POINT + DEAD_BAND))))
			{
	//			lcd_printf("Left");
				send_string(rot_aclk, PACKET_LEN);
				_delay_ms(PACKET_INTERVAL);
				//lcd_clear();
			}
			else if((x_read <= (X_MID_POINT - DEAD_BAND))&&(y_read >= (Y_MID_POINT - DEAD_BAND))&&(y_read <= (Y_MID_POINT + DEAD_BAND)))
			{
				//lcd_printf("Right");
				send_string(rot_clk, PACKET_LEN);
				_delay_ms(PACKET_INTERVAL);
				//lcd_clear();
			}
			else if((y_read >= (Y_MID_POINT + DEAD_BAND))&&(x_read >= (X_MID_POINT - DEAD_BAND))&&(x_read <= (X_MID_POINT + DEAD_BAND)))
			{
				//lcd_printf("Forward");
				send_string(mov_fwd, PACKET_LEN);
				_delay_ms(PACKET_INTERVAL);
				//lcd_clear();
			}
			else if((y_read <= (Y_MID_POINT - DEAD_BAND))&&(x_read >= (X_MID_POINT - DEAD_BAND))&&(x_read <= (X_MID_POINT + DEAD_BAND)))
			{
				//lcd_printf("Backward");
				send_string(mov_rev, PACKET_LEN);
				_delay_ms(PACKET_INTERVAL);
				//lcd_clear();
			}
			else
			{*/
				//lcd_printf("Stop");
				send_string(stop, PACKET_LEN);
				_delay_ms(PACKET_INTERVAL);
				//lcd_clear();
		//	}
/*			itoa(val,str,10);
			lcd_printf(str);
			_delay_ms(100);
			lcd_clear();*/
			/*cs_f = (PINC & FWD_PIN);
			if((!cs_f) && ls_f)
			{
				_delay_ms(20);
				while(!(PINC & FWD_PIN))
				{
					send_string(mov_fwd, PACKET_LEN);
					_delay_ms(PACKET_INTERVAL);
				}

				send_string(stop, PACKET_LEN);
			}
			ls_f = cs_f;

			cs_r = (PINC & REV_PIN) ;
			if((!cs_r) && ls_r)
			{
				_delay_ms(20);
				while(!(PINC & REV_PIN))
				{
					send_string(mov_rev, PACKET_LEN);
					_delay_ms(PACKET_INTERVAL);
				}

				send_string(stop, PACKET_LEN);
			}
			ls_r = cs_r;

			cs_cw = (PINC & RCK_PIN);
			if((!cs_cw) && ls_cw)
			{
				while(!(PINC & RCK_PIN))
				{
					send_string(rot_clk, PACKET_LEN);
					_delay_ms(PACKET_INTERVAL);
				}

				send_string(stop, PACKET_LEN);
			}
			ls_cw = cs_cw;

			cs_ccw = (PINC & RACK_PIN);
			if((!cs_ccw) && ls_ccw)
			{
				while(!(PINC & RACK_PIN))
				{
					send_string(rot_aclk, PACKET_LEN);
					_delay_ms(PACKET_INTERVAL);
				}

				send_string(stop, PACKET_LEN);
			}
			ls_ccw = cs_ccw;

*/
			// Node Read

	//		cs_n1 = (PIND & N1_KEY) ;
		//	if((!cs_n1) && ls_n1)
			//{
				//_delay_ms(20);
				//while(!(PIND & N1_KEY))
			//	{
			//		send_string(node[N1], NODE_LEN);
			//		_delay_ms(NODE_INTERVAL);
			//	}
			//}
			//ls_n1 = cs_n1;


			//cs_n2 = (PIND & N2_KEY) ;
			//if((!cs_n2) && ls_n2)
			//{
				//_delay_ms(20);
		//		while(!(PIND & N2_KEY))
			//	{
				//	send_string(node[N2], NODE_LEN);
				//	_delay_ms(NODE_INTERVAL);
				//}
			//}
			//ls_n2 = cs_n2;

			//cs_n3 = (PIND & N3_KEY) ;
			//if((!cs_n3) && ls_n3)
			//{
				//_delay_ms(20);
				//while(!(PIND & N3_KEY))
				///{
				//	send_string(node[N3], NODE_LEN);
				//	_delay_ms(NODE_INTERVAL);
				//}*/
			//}
			//ls_n3 = cs_n3;

			//cs_n4 = (PIND & N4_KEY) ;
			//if((!cs_n4) && ls_n4)
			//{
				//_delay_ms(20);
				//while(!(PIND & N4_KEY))
				//{
				//	send_string(node[N4], NODE_LEN);
					//_delay_ms(NODE_INTERVAL);
				//}
			//}
			//ls_n4 = cs_n4;

		//	cs_n5 = (PIND & N5_KEY) ;
			//if((!cs_n5) && ls_n5)
			//{
			//	_delay_ms(20);
				//while(!(PIND & N5_KEY))
				//{
				//	send_string(node[N5], NODE_LEN);
				//	_delay_ms(NODE_INTERVAL);
				//}*/
			//}
		//	ls_n5 = cs_n5;
		}
}
/*ISR(USART_RXC_vect)
{
		cli();
		UCSRA=0x00;
		sei();
}*/
/*ISR(USART_TXC_vect)
{
		cli();
		//UCSRA|=0x40;
		sei();
}*/

