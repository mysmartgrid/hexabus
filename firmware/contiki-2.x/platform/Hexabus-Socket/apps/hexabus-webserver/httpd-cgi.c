/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *			Adam Dunkels
 *
 */
/* Line endings in git repository are LF instead of CR-LF ? */
/*
 * This file includes functions that are called by the web server
 * scripts. The functions takes no argument, and the return value is
 * interpreted as follows. A zero means that the function did not
 * complete and should be invoked for the next packet as well. A
 * non-zero value indicates that the function has completed and that
 * the web server should move along to the next script line.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hexabus_config.h"

#include "contiki-net.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"
#include "httpd-fsdata.h"
#include "lib/petsciiconv.h"

//#include "sensors.h"

#include "metering.h"
#include "temperature.h"
#include "relay.h"
#include "eeprom_variables.h"
#include "datetime_service.h"
#include "state_machine.h"

#if RF230BB
#include "radio/rf230bb/rf230bb.h"
#elif RF212BB
#include "radio/rf212bb/rf212bb.h"
#endif


#define DEBUGLOGIC 0        //See httpd.c, if 1 must also set it there!
#if DEBUGLOGIC
#define uip_mss(...) 512
#define uip_appdata TCPBUF
extern char TCPBUF[512];
#endif

/* RADIOSTATS must also be set in clock.c and the radio driver */
#if RF230BB || RF212BB
// TODO #define RADIOSTATS 1
#endif

/*
#if RADIOSTATS
uint8_t RF212_rsigsi, rf212_last_rssi;
uint16_t RF212_sendpackets,RF212_receivepackets,RF212_sendfail,RF212_receivefail;
#endif
*/


static struct httpd_cgi_call *calls = NULL;

/*cgi function names*/
#if HTTPD_FS_STATISTICS
static const char   file_name[] HTTPD_STRING_ATTR = "file-stats";
#endif
static const char    tcp_name[] HTTPD_STRING_ATTR = "tcp-connections";
static const char   proc_name[] HTTPD_STRING_ATTR = "processes";
static const char socket_status_name[] HTTPD_STRING_ATTR = "socket_stat";
static const char   adrs_name[] HTTPD_STRING_ATTR = "addresses";
static const char   nbrs_name[] HTTPD_STRING_ATTR = "neighbors";
static const char   rtes_name[] HTTPD_STRING_ATTR = "routes";
static const char config_name[] HTTPD_STRING_ATTR = "config";

/*Process states for processes cgi*/
static const char      closed[] HTTPD_STRING_ATTR = "CLOSED";
static const char    syn_rcvd[] HTTPD_STRING_ATTR = "SYN-RCVD";
static const char    syn_sent[] HTTPD_STRING_ATTR = "SYN-SENT";
static const char established[] HTTPD_STRING_ATTR = "ESTABLISHED";
static const char  fin_wait_1[] HTTPD_STRING_ATTR = "FIN-WAIT-1";
static const char  fin_wait_2[] HTTPD_STRING_ATTR = "FIN-WAIT-2";
static const char     closing[] HTTPD_STRING_ATTR = "CLOSING";
static const char   time_wait[] HTTPD_STRING_ATTR = "TIME-WAIT";
static const char    last_ack[] HTTPD_STRING_ATTR = "LAST-ACK";
static const char        none[] HTTPD_STRING_ATTR = "NONE";
static const char     running[] HTTPD_STRING_ATTR = "RUNNING";
static const char      called[] HTTPD_STRING_ATTR = "CALLED";
static const char *states[] = {
  closed,
  syn_rcvd,
  syn_sent,
  established,
  fin_wait_1,
  fin_wait_2,
  closing,
  time_wait,
  last_ack,
  none,
  running,
  called};

  extern unsigned long seconds, sleepseconds;
#if RADIOSTATS
  extern unsigned long radioontime;
  static unsigned long savedradioontime;
#if RF230BB
  extern uint8_t RF230_radio_on, rf230_last_rssi;
  extern uint16_t RF230_sendpackets,RF230_receivepackets,RF230_sendfail,RF230_receivefail;
#elif RF212BB
  extern uint8_t RF212_radio_on, rf212_last_rssi;
  extern uint16_t RF212_sendpackets, RF212_receivepackets, RF212_sendfail, RF212_receivefail;
#endif
#endif


/*---------------------------------------------------------------------------*/
static
PT_THREAD(nullfunction(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_cgifunction
httpd_cgi(char *name)
{
  struct httpd_cgi_call *f;

  /* Find the matching name in the table, return the function. */
  for(f = calls; f != NULL; f = f->next) {
    if(httpd_strncmp(name, f->name, httpd_strlen(f->name)) == 0) {
      return f->function;
    }
  }
  return nullfunction;
}

#if HTTPD_FS_STATISTICS
static char *thisfilename;
/*---------------------------------------------------------------------------*/
static unsigned short
generate_file_stats(void *arg)
{
  static const char httpd_cgi_filestat1[] HTTPD_STRING_ATTR = "<p class=right><br><br><i>This page has been sent %u times</i></div></body></html>";
  static const char httpd_cgi_filestat2[] HTTPD_STRING_ATTR = "<tr><td><a href=\"%s\">%s</a></td><td>%d</td>";
  static const char httpd_cgi_filestat3[] HTTPD_STRING_ATTR = "%5u";
  char tmp[20];
  struct httpd_fsdata_file_noconst *f,fram;
  u16_t i;
  unsigned short numprinted;

  /* Transfer arg from whichever flash that contains the html file to RAM */
  httpd_fs_cpy(&tmp, (char *)arg, 20);

  /* Count for this page, with common page footer */
  if (tmp[0]=='.') {
    numprinted=httpd_snprintf((char *)uip_appdata, uip_mss(), httpd_cgi_filestat1, httpd_fs_open(thisfilename, 0));

  /* Count for all files */
  /* Note buffer will overflow if there are too many files! */
  } else if (tmp[0]=='*') {
    i=0;numprinted=0;
    for(f = (struct httpd_fsdata_file_noconst *)httpd_fs_get_root();
        f != NULL;
        f = (struct httpd_fsdata_file_noconst *)fram.next) {

      /* Get the linked list file entry into RAM from from wherever it is*/
      httpd_memcpy(&fram,f,sizeof(fram));

      /* Get the file name from whatever memory it is in */
      httpd_fs_cpy(&tmp, fram.name, sizeof(tmp));
#if HTTPD_FS_STATISTICS==1
      numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_filestat2, tmp, tmp, f->count);
#elif HTTPD_FS_STATISTICS==2
      numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_filestat2, tmp, tmp, httpd_filecount[i]);
#endif
      i++;
    }

  /* Count for specified file */
  } else {
    numprinted=httpd_snprintf((char *)uip_appdata, uip_mss(), httpd_cgi_filestat3, httpd_fs_open(tmp, 0));
  }
#if DEBUGLOGIC
  return 0;
#endif
  return numprinted;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(file_stats(struct httpd_state *s, char *ptr))
{

  PSOCK_BEGIN(&s->sout);

  thisfilename=&s->filename[0]; //temporary way to pass filename to generate_file_stats

  PSOCK_GENERATOR_SEND(&s->sout, generate_file_stats, (void *) ptr);

  PSOCK_END(&s->sout);
}
#endif /*HTTPD_FS_STATISTICS*/
/*---------------------------------------------------------------------------*/
static unsigned short
make_tcp_stats(void *arg)
{
  static const char httpd_cgi_tcpstat1[] HTTPD_STRING_ATTR = "<tr align=\"center\"><td>%d</td><td>";
  static const char httpd_cgi_tcpstat2[] HTTPD_STRING_ATTR = "-%u</td><td>%s</td><td>%u</td><td>%u</td><td>%c %c</td></tr>\r\n";
  struct uip_conn *conn;
  struct httpd_state *s = (struct httpd_state *)arg;
  char tstate[20];
  uint16_t numprinted;

  conn = &uip_conns[s->u.count];

  numprinted = httpd_snprintf((char *)uip_appdata, uip_mss(), httpd_cgi_tcpstat1, uip_htons(conn->lport));
  numprinted += httpd_cgi_sprint_ip6(conn->ripaddr, uip_appdata + numprinted);
  httpd_strcpy(tstate,states[conn->tcpstateflags & UIP_TS_MASK]);
  numprinted +=  httpd_snprintf((char *)uip_appdata + numprinted, uip_mss() - numprinted,
                 httpd_cgi_tcpstat2,
                 uip_htons(conn->rport),
                 tstate,
                 conn->nrtx,
                 conn->timer,
                 (uip_outstanding(conn))? '*':' ',
                 (uip_stopped(conn))? '!':' ');

  return numprinted;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(tcp_stats(struct httpd_state *s, char *ptr))
{

  PSOCK_BEGIN(&s->sout);

  for(s->u.count = 0; s->u.count < UIP_CONNS; ++s->u.count) {
    if((uip_conns[s->u.count].tcpstateflags & UIP_TS_MASK) != UIP_CLOSED) {
      PSOCK_GENERATOR_SEND(&s->sout, make_tcp_stats, s);
    }
  }

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static unsigned short
make_processes(void *p)
{
  static const char httpd_cgi_proc[] HTTPD_STRING_ATTR = "<tr align=\"center\"><td>%p</td><td>%s</td><td>%p</td><td>%s</td></tr>\r\n";
  char name[40],tstate[20];

  strncpy(name, ((struct process *)p)->name, 40);
  petsciiconv_toascii(name, 40);
  httpd_strcpy(tstate,states[9 + ((struct process *)p)->state]);
  return httpd_snprintf((char *)uip_appdata, uip_mss(), httpd_cgi_proc, p, name,
    *(char *)(&(((struct process *)p)->thread)),

    tstate);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(processes(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  for(s->u.ptr = PROCESS_LIST(); s->u.ptr != NULL; s->u.ptr = ((struct process *)s->u.ptr)->next) {
    PSOCK_GENERATOR_SEND(&s->sout, make_processes, s->u.ptr);
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static const char httpd_cgi_addrh[] HTTPD_STRING_ATTR = "<code>";
static const char httpd_cgi_addrf[] HTTPD_STRING_ATTR = "</code>[Room for %u more]";
static const char httpd_cgi_addrb[] HTTPD_STRING_ATTR = "<br>";
static const char httpd_cgi_addrn[] HTTPD_STRING_ATTR = "(none)<br>";
extern uip_ds6_nbr_t uip_ds6_nbr_cache[];
extern uip_ds6_route_t uip_ds6_routing_table[];
extern uip_ds6_netif_t uip_ds6_if;

static unsigned short
make_addresses(void *p)
{
uint8_t i,j=0;
uint16_t numprinted;
  numprinted = httpd_snprintf((char *)uip_appdata, uip_mss(),httpd_cgi_addrh);
  for (i=0; i<UIP_DS6_ADDR_NB;i++) {
    if (uip_ds6_if.addr_list[i].isused) {
      j++;
      numprinted += httpd_cgi_sprint_ip6(uip_ds6_if.addr_list[i].ipaddr, uip_appdata + numprinted);
      numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrb); 
    }
  }
//if (j==0) numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrn);
  numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrf, UIP_DS6_ADDR_NB-j); 
  return numprinted;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(addresses(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, make_addresses, s->u.ptr);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/	
static unsigned short
make_neighbors(void *p)
{
uint8_t i,j=0;
uint16_t numprinted;
  numprinted = httpd_snprintf((char *)uip_appdata, uip_mss(),httpd_cgi_addrh);
  for (i=0; i<UIP_DS6_NBR_NB;i++) {
    if (uip_ds6_nbr_cache[i].isused) {
      j++;
      numprinted += httpd_cgi_sprint_ip6(uip_ds6_nbr_cache[i].ipaddr, uip_appdata + numprinted);
      numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrb); 
    }
  }
//if (j==0) numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrn);
  numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrf,UIP_DS6_NBR_NB-j);
  return numprinted;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(neighbors(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, make_neighbors, s->u.ptr);  

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static unsigned short
make_routes(void *p)
{
static const char httpd_cgi_rtes1[] HTTPD_STRING_ATTR = "(%u (via ";
static const char httpd_cgi_rtes2[] HTTPD_STRING_ATTR = ") %lus<br>";
static const char httpd_cgi_rtes3[] HTTPD_STRING_ATTR = ")<br>";
uint8_t i,j=0;
uint16_t numprinted;
  numprinted = httpd_snprintf((char *)uip_appdata, uip_mss(),httpd_cgi_addrh);
  for (i=0; i<UIP_DS6_ROUTE_NB;i++) {
    if (uip_ds6_routing_table[i].isused) {
      j++;
      numprinted += httpd_cgi_sprint_ip6(uip_ds6_routing_table[i].ipaddr, uip_appdata + numprinted);
      numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_rtes1, uip_ds6_routing_table[i].length);
      numprinted += httpd_cgi_sprint_ip6(uip_ds6_routing_table[i].nexthop, uip_appdata + numprinted);
      if(uip_ds6_routing_table[i].state.lifetime < 3600) {
         numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_rtes2, uip_ds6_routing_table[i].state.lifetime);
      } else {
         numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_rtes3);
      }
    }
  }
  if (j==0) numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrn);
  numprinted += httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_addrf,UIP_DS6_ROUTE_NB-j);
  return numprinted;
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_IPV6_RPL
static
PT_THREAD(routes(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, make_routes, s->u.ptr); 

  PSOCK_END(&s->sout);
}
#endif
/*---------------------------------------------------------------------------*/
static unsigned short
generate_socket_readings(void *arg)
{
  uint16_t numprinted;
  static const char httpd_cgi_sensor1[] HTTPD_STRING_ATTR = "<em>Electrical power consumption:</em> %d Watt<br>";
  static const char httpd_cgi_sensor2[] HTTPD_STRING_ATTR = "<em>Current Relay state:</em> %s <br>";
  static const char httpd_cgi_sensor3[] HTTPD_STRING_ATTR = "<form action=\"socket_stat.shtml\" method=\"post\"><input type=\"submit\" value=\"%s\" ></form>";
  static const char httpd_cgi_sensor4[] HTTPD_STRING_ATTR = "<em>Current temperature:</em> %s deg. C<br>";
  static const char httpd_cgi_datetime[] HTTPD_STRING_ATTR = "<em>Current Date and Time:</em> %s <br>";
	numprinted=0;

  //N:
  if(relay_get_state()==0)
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor1, metering_get_power());
  else
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor1, 0);

  //N:
  if(relay_get_state()==0)
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor2, "ON");
  else
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor2, "OFF");

  //numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor2, !relay_get_state());

  if (!relay_get_state())
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor3, "Toggle Off");
  else
	  numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor3, "Toggle On");

  // Add Temperature
 // char buffer[10];
 // float temperature_value=getTemperatureFloat();
 // dtostrf(temperature_value, 9, 4, &buffer);
	numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_sensor4, temperature_as_string());
  // Add Date and Time
  struct hxb_datetime dt;
	if(getDatetime(&dt) == 0) {
		char time[30];
		sprintf(time, "%u:%u:%u, %u.%u.%u", dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
		numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_datetime, time);
	} else {
		numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_datetime, "Date and Time not valid.");
	}
  return numprinted;
}

/*---------------------------------------------------------------------------*/
//dynamically generate the content of "Configuration" webpage
static unsigned short
generate_config(void *arg)
{
	uint16_t numprinted;
//	extern bool relay_state;

	static const char httpd_cgi_config_line1[] HTTPD_STRING_ATTR = "<td><input name=\"domain_name\" type=\"text\" size=\"50\" maxlength=\"30\" value=\"%s\"></td></tr>";
	static const char httpd_cgi_config_line2[] HTTPD_STRING_ATTR = "<tr><td align=\"right\">Default Relay State</td><td><input type=\"radio\" name=\"relay\" value=\"1\" %s>On<input type=\"radio\" name=\"relay\" value=\"0\" %s>Off</td></tr>";
//	static const char httpd_cgi_config_line3[] HTTPD_STRING_ATTR = "<tr><td align=\"right\">S0 meter (Imp./kWh)</td><td><input type=\"text\" size=\"4\" maxlength=\"4\" name=\"s0\" value=\"%s\"></td></tr>";
    static const char httpd_cgi_config_line4[] HTTPD_STRING_ATTR = "<tr><input type=\"hidden\" name=\"terminator\" value=\"\"><td align=\"right\">Submit:</td><td><input type=\"submit\" value=\" Submit \" ></td></tr></table></form>"; //additional ampersand from the hidden value simplifies parsing

    char* checked = "checked";
	numprinted=0;
	char tmp[EE_DOMAIN_NAME_SIZE];
	eeprom_read_block(tmp,(const void*) EE_DOMAIN_NAME, EE_DOMAIN_NAME_SIZE);
	numprinted =httpd_snprintf((char *)uip_appdata, uip_mss(), httpd_cgi_config_line1, tmp);

	if (!eeprom_read_byte((void*) EE_RELAY_DEFAULT))
		numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_config_line2, checked, "");
	else
		numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_config_line2, "", checked);

#if S0_ENABLE
    char s0val[5];
    uint32_t refval= (eeprom_read_word((void*) EE_METERING_REF));
    refval = ((3600000*CLOCK_SECOND)/(refval*10));            // 1h*1000(for kilowatts) / refval*10(to fit into 16bits)

    ltoa(refval, s0val, 10);

   numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_config_line3, s0val);
#endif

	numprinted+=httpd_snprintf((char *)uip_appdata+numprinted, uip_mss()-numprinted, httpd_cgi_config_line4);

	return numprinted;

}



#if RADIOSTATS
/*---------------------------------------------------------------------------*/
static unsigned short
generate_radio_stats(void *arg)
{
  uint16_t numprinted;
  uint16_t h,m,s;
  uint8_t p1,p2;
  static const char httpd_cgi_sensor10[] HTTPD_STRING_ATTR = "<em>Radio on time  :</em> %02d:%02d:%02d (%d.%02d%%)<br>";
  static const char httpd_cgi_sensor11[] HTTPD_STRING_ATTR = "<em>Packets:</em> Tx=%5d Rx=%5d TxL=%5d RxL=%5d RSSI=%2ddBm\n";

  s=(10000UL*savedradioontime)/seconds;
  p1=s/100;
  p2=s-p1*100;
  h=savedradioontime/3600;
  s=savedradioontime-h*3600;
  m=s/60;
  s=s-m*60;

  numprinted =httpd_snprintf((char *)uip_appdata             , uip_mss()             , httpd_cgi_sensor10,\
    h,m,s,p1,p2);

#if RF230BB
  numprinted+=httpd_snprintf((char *)uip_appdata + numprinted, uip_mss() - numprinted, httpd_cgi_sensor11,\
    RF230_sendpackets,RF230_receivepackets,RF230_sendfail,RF230_receivefail,-92+rf230_last_rssi);

#elif RF212BB
  numprinted+=httpd_snprintf((char *)uip_appdata + numprinted, uip_mss() - numprinted, httpd_cgi_sensor11,\
      RF212_sendpackets,RF212_receivepackets,RF212_sendfail,RF212_receivefail,-92+rf212_last_rssi);
#else
  p1=0;
  radio_get_rssi_value(&p1);
  p1 = -91*3(p1-1);
  numprinted+=httpd_snprintf((char *)uip_appdata + numprinted, uip_mss() - numprinted, httpd_cgi_sensor11,\
    RF230_sendpackets,RF230_receivepackets,RF230_sendfail,RF230_receivefail,p1);
#endif

  return numprinted;
}
#endif
/*---------------------------------------------------------------------------*/
void hxbtos(char *dest, char *data, uint8_t datatype)
{
	struct hxb_datetime *dt;
	switch ((enum hxb_datatype) datatype) {
		case HXB_DTYPE_UNDEFINED:	// "Do nothing" Datatype
			sprintf(dest, "0");		// 0 so the format does not break
			break;
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			sprintf(dest, "%u", *(uint8_t*)data);
			break;
		case HXB_DTYPE_UINT32:
		case HXB_DTYPE_TIMESTAMP:
			sprintf(dest, "%lu", *(uint32_t*)data);
			break;
		case HXB_DTYPE_DATETIME:
			dt = (struct hxb_datetime*)data;
			sprintf(dest, "%u*%u*%u*%u*%u*%u*%u*", dt->hour, dt->minute, dt->second, dt->day, dt->month, (uint16_t)dt->year, dt->weekday); 
			break;
		case HXB_DTYPE_FLOAT:
			dtostrf(*(float*)data, 1, 6, dest);
			uint8_t i;
			for(i = 0;dest[i] != '.';) {
				i++;
			}
			dest[i] = ',';
			break;
		case HXB_DTYPE_128STRING:
		case HXB_DTYPE_65BYTES:
		case HXB_DTYPE_16BYTES:
		default:
			break;
	}
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(socket_readings(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, generate_socket_readings, s);
#if RADIOSTATS
  PSOCK_GENERATOR_SEND(&s->sout, generate_radio_stats, s);
#endif
  
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(set_config(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, generate_config, s);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
void
httpd_cgi_add(struct httpd_cgi_call *c)
{
  struct httpd_cgi_call *l;

  c->next = NULL;
  if(calls == NULL) {
    calls = c;
  } else {
    for(l = calls; l->next != NULL; l = l->next);
    l->next = c;
  }
}
/*---------------------------------------------------------------------------*/

#if HTTPD_FS_STATISTICS
HTTPD_CGI_CALL(   file,   file_name,      file_stats);
#endif
HTTPD_CGI_CALL(    tcp,    tcp_name, tcp_stats      );
HTTPD_CGI_CALL(   proc,   proc_name, processes      );
HTTPD_CGI_CALL(   adrs,   adrs_name, addresses      );
HTTPD_CGI_CALL(   nbrs,   nbrs_name, neighbors      );
#if UIP_CONF_IPV6_RPL
HTTPD_CGI_CALL(   rtes,   rtes_name, routes         );
#endif
HTTPD_CGI_CALL(socket_stat, socket_status_name, socket_readings);
HTTPD_CGI_CALL(config, config_name, set_config);

void
httpd_cgi_init(void)
{
#if HTTPD_FS_STATISTICS
  httpd_cgi_add(   &file);
#endif
  httpd_cgi_add(    &tcp);
  httpd_cgi_add(   &proc);
  httpd_cgi_add(   &adrs);
  httpd_cgi_add(   &nbrs);
#if UIP_CONF_IPV6_RPL
  httpd_cgi_add(   &rtes);
#endif
  httpd_cgi_add(&socket_stat);
  httpd_cgi_add(&config);
}
/*---------------------------------------------------------------------------*/

uint8_t httpd_cgi_sprint_ip6(uip_ip6addr_t addr, char * result)
{
  unsigned char zerocnt = 0;
  unsigned char numprinted = 0;
  char * starting = result;

  unsigned char i = 0;

  while (numprinted < 8)
  {
    //Address is zero, have we used our ability to
    //replace a bunch with : yet?
    if ((addr.u16[i] == 0) && (zerocnt == 0))
    {
      //How many zeros?
      zerocnt = 0;
      while(addr.u16[zerocnt + i] == 0)
        zerocnt++;

      //just one, don't waste our zeros...
      if (zerocnt == 1)
      {
        *result++ = '0';
        numprinted++;
        break;
      }

      //Cool - can replace a bunch of zeros
      i += zerocnt;
      numprinted += zerocnt;
    }
    //Normal address, just print it
    else
    {
      result += sprintf(result, "%x", (unsigned int)(uip_ntohs(addr.u16[i])));
      i++;
      numprinted++;
    }

    //Don't print : on last one
    if (numprinted != 8)
      *result++ = ':';
  }

  return (result - starting);
}

