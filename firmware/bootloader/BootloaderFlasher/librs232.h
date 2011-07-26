/*
* This source code is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*       
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* File Name: librs232.h							
*
* Reference:
*
* Author: Li Feng                                                   
*
* Description:
*
* 	
* 
* History:
* 11/18/2005  Li Feng    Created
*  
*
*CodeReview Log:
* 
*/
#ifndef __RS232_H__
#define __RS232_H__

/* MAXIMUM STRING LENGTHS */
#define MAXREADCHARS 1024
#define MAXSENDCHARS 512
#define MAXFILECHARS 16

/* BOOLEAN DEFINITIONS */
#define BOOL int
#define TRUE  (1==1)
#define FALSE (0==1)

//baud rate
#define BR_9600		9600
#define BR_19200	19200
#define BR_38400	38400
#define BR_57600	57600
#define BR_115200	115200
#define BR_230400	230400

//data bits
#define DB_5		5 
#define DB_6		6 
#define DB_7		7 
#define DB_8		8 

//parity
#define PARITY_NONE 'N'
#define PARITY_EVEN	'E'
#define PARITY_ODD	'O'

//stop bits
#define SB_1	1
#define SB_2	2		

typedef struct tagRS232_PARAM
{
   int	BaudRate;//baut rate
   int	DataBits;//data bits
   char Parity;
   int	StopBits;
   int	xonxoff;
   int	rtscts;
}RS232_PARAM;

#define RS_UINT32	unsigned int	
#define RS_UINT8	unsigned char	
#define RS_INT32	int		 
#define RS_INT8		char	

#define HRS232 void* /* serial device handle */

HRS232		RS232_Open  (char const *pDeviceName);
BOOL		RS232_SetParam(HRS232 hRS232, RS232_PARAM *pParam);
BOOL		RS232_Send (HRS232 hRS232, RS_UINT8 *sendstring, RS_UINT32 nLength);
RS_UINT32	RS232_Read (HRS232 hRS232, RS_UINT8 *readbuf, RS_UINT32 readlength);
void		RS232_Close (HRS232 hRS232);

#endif

