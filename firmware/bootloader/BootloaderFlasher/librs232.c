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
* File Name: librs232.c							
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include "librs232.h"

static BOOL setBaudRate  (struct termios *rs232_attr, int baudrate);
static BOOL setDataBits  (struct termios *rs232_attr, int databits);
static BOOL setParity    (struct termios *rs232_attr, char parity);
static BOOL setStopBits  (struct termios *rs232_attr, int stopbits);
static BOOL setFlowCtl   (struct termios *rs232_attr, int xonxoff, int rtscts);

typedef struct tagINTER_PARAM
{
	int nDev;
	struct termios rs232_attr;
}INTER_PARAM;

/*****************************************************************************
 * OPEN SERIAL DEVICE
 ****************************************************************************/
HRS232 RS232_Open (char const *pDeviceName)
{
	INTER_PARAM *pDev = NULL;

	pDev = (INTER_PARAM*)malloc(sizeof(INTER_PARAM));
	if(pDev==NULL)
		return NULL;

	pDev->nDev = open (pDeviceName, O_RDWR | O_NOCTTY);
	if (pDev->nDev == -1) 
	{
	  fprintf (stderr, "error opening %s\n", pDeviceName); 
	  free(pDev);
	  return NULL;		  
	}
	return (HRS232)pDev;
}

/*****************************************************************************
 * INITIALIZE SERIAL DEVICE (SET COMMUNICATION PARAMETERS)
 ****************************************************************************/
BOOL RS232_SetParam (HRS232 hRS232, RS232_PARAM *pRS232_PARAM)
{
	INTER_PARAM *pDev = (INTER_PARAM *)hRS232;
	if(pDev==NULL)
		return FALSE;

	pDev->rs232_attr.c_oflag = 0;
	pDev->rs232_attr.c_lflag = 0;
	pDev->rs232_attr.c_iflag = ICRNL | IGNPAR;
	pDev->rs232_attr.c_cflag = CREAD | CLOCAL;	

	if (setBaudRate (&pDev->rs232_attr, pRS232_PARAM->BaudRate) == FALSE)
		return FALSE;
	if (setDataBits (&pDev->rs232_attr, pRS232_PARAM->DataBits) == FALSE) 
		return FALSE;
	if (setParity   (&pDev->rs232_attr, pRS232_PARAM->Parity  ) == FALSE) 
		return FALSE;
	if (setStopBits (&pDev->rs232_attr, pRS232_PARAM->StopBits) == FALSE) 
		return FALSE;
	if (setFlowCtl  (&pDev->rs232_attr, pRS232_PARAM->xonxoff, pRS232_PARAM->rtscts) == FALSE)
		return FALSE; 

	if ( tcsetattr (pDev->nDev, TCSANOW, &pDev->rs232_attr) == -1) 
	{ 
		fprintf (stderr, "error setting tcsetattr on \n");  
		return FALSE;
	}

	return TRUE;
}

/*****************************************************************************
 * SEND STRING TO SERIAL DEVICE
 ****************************************************************************/
BOOL RS232_Send (HRS232 hRS232, RS_UINT8 *sendstring, RS_UINT32 nLength)
{
	INTER_PARAM *pDev = (INTER_PARAM *)hRS232;
	if(pDev==NULL)
		return FALSE;
	
	if(nLength > MAXSENDCHARS)
	{
		fprintf (stderr, "too much data\n");
		return FALSE;
	}

	if (write(pDev->nDev, sendstring, nLength) != nLength) 
	{
		fprintf (stderr, "error: write\n");
		return FALSE;
	}
	return TRUE;
};

/*****************************************************************************
 * READ # of BYTES FROM SERIAL DEVICE OR UNTIL STOP CHARACTER IS FOUND
 ****************************************************************************/
RS_UINT32 RS232_Read (HRS232 hRS232, RS_UINT8 *readbuf, RS_UINT32 readlength)
{
	RS_UINT32 nRead = 0;
	INTER_PARAM *pDev = (INTER_PARAM *)hRS232;
	if(pDev==NULL)
		return FALSE;
	
   if ((nRead = read (pDev->nDev, readbuf, readlength)) == -1)
	   return 0;
   return nRead;
}

/*****************************************************************************
 * CLOSE SERIAL DEVICE
 ****************************************************************************/
void RS232_Close (HRS232 hRS232)
{
	INTER_PARAM *pDev = (INTER_PARAM *)hRS232;
	if(pDev==NULL)
		return;

	if (close(pDev->nDev) == -1) 
	{
		return;
	}
	free(pDev);
	return;
}

/*****************************************************************************
 * SET BAUD RATE
 ****************************************************************************/
static BOOL setBaudRate (struct termios *rs232_attr, int baudrate)
{
   switch (baudrate) 
   { 
    case 50:
      rs232_attr->c_cflag |= B50;
      break;
    case 75:
      rs232_attr->c_cflag |= B75;
      break;
    case 110:
      rs232_attr->c_cflag |= B110;
      break;
    case 134:
      rs232_attr->c_cflag |= B134;
      break;
    case 150:
      rs232_attr->c_cflag |= B150;
      break;
    case 200:
      rs232_attr->c_cflag |= B200;
      break;
    case 300:
      rs232_attr->c_cflag |= B300;
      break;
    case 600:
      rs232_attr->c_cflag |= B600;
      break;
    case 1200:
      rs232_attr->c_cflag |= B1200;
      break;
    case 2400:
      rs232_attr->c_cflag |= B2400;
      break;
    case 4800:
      rs232_attr->c_cflag |= B4800;
      break;
    case 9600:
      rs232_attr->c_cflag |= B9600;
      break;
    case 19200:
      rs232_attr->c_cflag |= B19200;
      break;
    case 38400:
      rs232_attr->c_cflag |= B38400;
      break;
    case 57600:
      rs232_attr->c_cflag |= B57600;
      break;
    case 115200:
      rs232_attr->c_cflag |= B115200;
      break;
    case 230400:
      rs232_attr->c_cflag |= B230400;
      break;
    default:
      fprintf (stderr, "error: unsupported baud rate\n");
      return FALSE;
   }

   return TRUE;
}

/*****************************************************************************
 * SET CHARACTER SIZE
 ****************************************************************************/
static BOOL setDataBits (struct termios *rs232_attr, int databits)
{
   switch (databits)
   {   
    case 5:
      rs232_attr->c_cflag |= CS5;
      break;
    case 6:  
      rs232_attr->c_cflag |= CS6;
      break;
    case 7:
      rs232_attr->c_cflag |= CS7;
      break;
    case 8:
      rs232_attr->c_cflag |= CS8;
      break;
    default:
      fprintf (stderr, "error: unsupported character size\n");
      return FALSE;
   }
   return TRUE;
}

/*****************************************************************************
 * SET PARITY
 ****************************************************************************/
static BOOL setParity (struct termios *rs232_attr, char parity)
{
   switch (toupper(parity)) 
   {   
     case 'N':
       break;
     case 'E':  
       rs232_attr->c_cflag |= PARENB;
       break;
     case 'O': 
       rs232_attr->c_cflag |= PARODD;
       break;
     default:
       fprintf (stderr, "error: unsupported parity mode\n");
       return FALSE;
   }
   return TRUE;
}

/*****************************************************************************
 * SET STOP BITS
 ****************************************************************************/
static BOOL setStopBits (struct termios *rs232_attr, int stopbits)
{
   switch (stopbits)
   {   
    case 1:
		  break;
    case 2:
		{
		  rs232_attr->c_cflag |= CSTOPB;
		  break;
		}
    default :
		{
		  fprintf (stderr, "error: unsupported number of stop bits\n");
		  return FALSE;
		}
   }
   return TRUE;
}

/*****************************************************************************
 * SET HARDWARE and SOFTWARE FLOW CONTROL
 ****************************************************************************/
static BOOL setFlowCtl (struct termios *rs232_attr, int xonxoff, int rtscts)
{
   if (xonxoff) 
   {
      rs232_attr->c_iflag |= IXON;
      rs232_attr->c_iflag |= IXOFF;
   }
   
   if (rtscts)
     rs232_attr->c_cflag |= CRTSCTS;
   
   return TRUE;
}
   

