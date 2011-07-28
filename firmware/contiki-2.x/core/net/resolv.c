/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uipdns uIP hostname resolver functions
 * @{
 *
 * The uIP DNS resolver functions are used to lookup a hostname and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the resolv_lookup()
 * function. New hostnames can be resolved using the resolv_query()
 * function.
 *
 * The event resolv_event_found is posted when a hostname has been
 * resolved. It is up to the receiving process to determine if the
 * correct hostname has been found by calling the resolv_lookup()
 * function with the hostname.
 */

/**
 * \file
 * DNS host name to IP address resolver.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file implements a DNS host name to IP address resolver.
 */

/*
 * Copyright (c) 2002-2003, Adam Dunkels.
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
 * $Id: resolv.c,v 1.9 2010/05/31 15:22:08 nifi Exp $
 *
 */

#include "net/tcpip.h"
#include "net/resolv.h"
#include "net/uip-udp-packet.h"
#if UIP_UDP

#include <string.h>
#include <stdio.h>

//#define VERBOSE_DEBUG 1

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

// If RESOLV_CONF_SUPPORTS_MDNS is set, then queries
// for domain names in the local TLD will use mDNS as
// described by draft-cheshire-dnsext-multicastdns.
#ifndef RESOLV_CONF_SUPPORTS_MDNS
#define RESOLV_CONF_SUPPORTS_MDNS (1)
#endif

/*
#ifndef RESOLV_CONF_MDNS_RESPONDER
#define RESOLV_CONF_MDNS_RESPONDER RESOLV_CONF_SUPPORTS_MDNS
#endif
*/

#if RESOLV_CONF_MDNS_RESPONDER
#error MDNS responder is not yet implemented!
#endif

/** \internal The maximum number of retries when asking for a name. */
#ifndef RESOLV_CONF_MAX_RETRIES
#define RESOLV_CONF_MAX_RETRIES (8)
#endif

#ifndef RESOLV_CONF_MAX_MDNS_RETRIES
#define RESOLV_CONF_MAX_MDNS_RETRIES (3)
#endif

/** \internal The DNS message header. */
struct dns_hdr {
  u16_t id;
  u8_t flags1, flags2;
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
  u16_t numquestions;
  u16_t numanswers;
  u16_t numauthrr;
  u16_t numextrarr;
};

#if 1
#define RESOLV_ENCODE_INDEX(i)		(uip_htons(i+61616))
#define RESOLV_DECODE_INDEX(i)		(unsigned char)(uip_ntohs(i)-61616)
#else // The following versions are useful for debugging.
#define RESOLV_ENCODE_INDEX(i)		(htons(i))
#define RESOLV_DECODE_INDEX(i)		(unsigned char)(ntohs(i))
#endif

/** \internal The DNS answer message structure. */
struct dns_answer {
  /* DNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t class;
  u16_t ttl[2];
  u16_t len;
#if UIP_CONF_IPV6
  u8_t ipaddr[16];
#else
  u8_t ipaddr[4];
#endif
};

#if UIP_CONF_IPV6
const uip_ipaddr_t resolv_default_dns_server = {
	.u8 = { // HE's DNS (2001:470:20::2)
		// When Google or whoever starts offering recursive DNS
		// via IPv6, then we should use it as the default instead.
		0x20, 0x01, 0x04, 0x70,
		0x00, 0x20, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x02,
	}
};
#else
const uip_ipaddr_t resolv_default_dns_server = {
	.u8 = { 8, 8, 8, 8 }	// Google's DNS
};
#endif

#define DNS_TYPE_A		(1)
#define DNS_TYPE_CNAME	(5)
#define DNS_TYPE_PTR	(12)
#define DNS_TYPE_MX		(15)
#define DNS_TYPE_TXT	(16)
#define DNS_TYPE_AAAA	(28)
#define DNS_TYPE_SRV	(33)

#define DNS_CLASS_IN	(1)

#ifndef DNS_PORT
#define DNS_PORT	(53)
#endif

#ifndef MDNS_PORT
#define MDNS_PORT	(5353)
#endif

#ifndef MDNS_RESPONDER_PORT
#define MDNS_RESPONDER_PORT	(5354)
#endif

#if RESOLV_CONF_SUPPORTS_MDNS
#if UIP_CONF_IPV6
static const uip_ipaddr_t resolv_mdns_addr = {
	.u8 = { 
		0xff, 0x02, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xfb,
	}
};
#else
static const uip_ipaddr_t resolv_mdns_addr = {
	.u8 = { 224, 0, 0, 251 }
};
#endif
#endif

struct namemap {
#define STATE_UNUSED 0
#define STATE_NEW    1
#define STATE_ASKING 2
#define STATE_DONE   3
#define STATE_ERROR  4
  u8_t state;
  u8_t tmr;
  u8_t retries;
  u8_t seqno;
  u8_t err;
#if RESOLV_CONF_SUPPORTS_MDNS
  u8_t is_mdns;
#endif
  char name[32];
  uip_ipaddr_t ipaddr;
};

#ifndef UIP_CONF_RESOLV_ENTRIES
#define RESOLV_ENTRIES 4
#else /* UIP_CONF_RESOLV_ENTRIES */
#define RESOLV_ENTRIES UIP_CONF_RESOLV_ENTRIES
#endif /* UIP_CONF_RESOLV_ENTRIES */


static struct namemap names[RESOLV_ENTRIES];

static u8_t seqno;

static struct uip_udp_conn *resolv_conn = NULL;

#if RESOLV_CONF_SUPPORTS_MDNS
static struct uip_udp_conn *mdns_conn = NULL;
#endif

static struct etimer retry;

process_event_t resolv_event_found;

PROCESS(resolv_process, "DNS resolver");

static void resolv_found(char *name, uip_ipaddr_t *ipaddr);

enum {
  EVENT_NEW_SERVER=0
};

/*-----------------------------------------------------------------------------------*/
/** \internal
 * Walk through a compact encoded DNS name and return the end of it.
 *
 * \return The end of the name.
 */
/*-----------------------------------------------------------------------------------*/
static unsigned char *
parse_name(unsigned char *query)
{
  unsigned char n;

#if VERBOSE_DEBUG
	printf("resolver: Parsing name: ");
#endif

  do {
    n = *query++;
    
    while(n > 0) {
#if VERBOSE_DEBUG
		printf("%c", *query);
#endif
      ++query;
      --n;
    };
#if VERBOSE_DEBUG
	printf(".");
#endif
  } while(*query != 0);
#if VERBOSE_DEBUG
	printf("\n");
#endif
  return query + 1;
}
/*-----------------------------------------------------------------------------------*/
/** \internal
 * Runs through the list of names to see if there are any that have
 * not yet been queried and, if so, sends out a query.
 */
/*-----------------------------------------------------------------------------------*/
static void
check_entries(void)
{
  register struct dns_hdr *hdr;
  char *query, *nptr, *nameptr;
  uint8_t i;
  uint8_t n;
  register struct namemap *namemapptr;
  
  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    namemapptr = &names[i];
    if(namemapptr->state == STATE_NEW ||
       namemapptr->state == STATE_ASKING) {
      etimer_set(&retry, CLOCK_SECOND);
      if(namemapptr->state == STATE_ASKING) {
	if(--namemapptr->tmr == 0) {
	  if(++namemapptr->retries == (namemapptr->is_mdns?RESOLV_CONF_MAX_MDNS_RETRIES:RESOLV_CONF_MAX_RETRIES)) {
	    namemapptr->state = STATE_ERROR;
	    resolv_found(namemapptr->name, NULL);
	    continue;
	  }
	  namemapptr->tmr = namemapptr->retries;
	} else {
	  /*	  printf("Timer %d\n", namemapptr->tmr);*/
	  /* Its timer has not run out, so we move on to next
	     entry. */
	  continue;
	}
      } else {
	namemapptr->state = STATE_ASKING;
	namemapptr->tmr = 1;
	namemapptr->retries = 0;
      }
      hdr = (struct dns_hdr *)uip_appdata;
      memset(hdr, 0, sizeof(struct dns_hdr));
      hdr->id = RESOLV_ENCODE_INDEX(i);
      hdr->flags1 = namemapptr->is_mdns?0:DNS_FLAG1_RD;
      hdr->numquestions = HTONS(1);
      query = (char *)uip_appdata + 12;
      nameptr = namemapptr->name;
      --nameptr;
      /* Convert hostname into suitable query format. */
      do {
	++nameptr;
	nptr = query;
	++query;
	for(n = 0; *nameptr != '.' && *nameptr != 0; ++nameptr) {
	  *query = *nameptr;
	  ++query;
	  ++n;
	}
	*nptr = n;
      } while(*nameptr != 0);
      {
	  *query++=0; // End the the name.
#if UIP_CONF_IPV6
	  *query++=(int8_t)((DNS_TYPE_AAAA)>>8);
	  *query++=(int8_t)((DNS_TYPE_AAAA));
#else
	  *query++=(int8_t)((DNS_TYPE_A)>>8);
	  *query++=(int8_t)((DNS_TYPE_A));
#endif
	  *query++=(int8_t)((DNS_CLASS_IN)>>8);
	  *query++=(int8_t)((DNS_CLASS_IN));
      }
#if RESOLV_CONF_SUPPORTS_MDNS
	if(namemapptr->is_mdns) {
		uip_udp_packet_sendto(
			mdns_conn,
			uip_appdata,
			(query - (char *)uip_appdata),
			&resolv_mdns_addr,
			HTONS(MDNS_PORT)
		);
#if VERBOSE_DEBUG
	printf("resolver: Sent MDNS request for \"%s\".\n",namemapptr->name);
#endif
	} else {
		uip_udp_conn = resolv_conn;
#if VERBOSE_DEBUG
		printf("resolver: Sent DNS request for \"%s\".\n",namemapptr->name);
#endif
		uip_udp_send((unsigned char)(query - (char *)uip_appdata));
	}
#else
#if VERBOSE_DEBUG
	printf("resolver: Sent DNS request for \"%s\".\n",namemapptr->name);
#endif
		uip_udp_conn = resolv_conn;
      uip_udp_send((unsigned char)(query - (char *)uip_appdata));
#endif
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
/** \internal
 * Called when new UDP data arrives.
 */
/*-----------------------------------------------------------------------------------*/
static void
newdata(void)
{
  unsigned char *nameptr;
  struct dns_answer *ans;
  struct dns_hdr *hdr;
  static u8_t nquestions, nanswers;
  static u8_t i;
  register struct namemap *namemapptr;
  
  hdr = (struct dns_hdr *)uip_appdata;
  /*  printf("ID %d\n", htons(hdr->id));
  printf("Query %d\n", hdr->flags1 & DNS_FLAG1_RESPONSE);
  printf("Error %d\n", hdr->flags2 & DNS_FLAG2_ERR_MASK);
  printf("Num questions %d, answers %d, authrr %d, extrarr %d\n",
	 htons(hdr->numquestions),
	 htons(hdr->numanswers),
	 htons(hdr->numauthrr),
	 htons(hdr->numextrarr));
  */

  /* The ID in the DNS header should be our entry into the name
     table. */
  i = RESOLV_DECODE_INDEX(hdr->id);
  if(i>=RESOLV_ENTRIES) {
#if VERBOSE_DEBUG
    printf("resolver: Bad ID (%04X) on incoming DNS response\n",uip_htons(hdr->id));
#endif
	return;
  }
  namemapptr = &names[i];
  if(i < RESOLV_ENTRIES &&
     namemapptr->state == STATE_ASKING) {
    /* We only care about the question(s) and the answers. The authrr
       and the extrarr are simply discarded. */
    nquestions = (u8_t)uip_htons(hdr->numquestions);
    nanswers = (u8_t)uip_htons(hdr->numanswers);

	if(!nanswers) {
		// Skipit.
		return;
	}

    /* This entry is now finished. */
    namemapptr->state = STATE_ERROR; // We'll change this to DONE when we find the record.
    namemapptr->err = hdr->flags2 & DNS_FLAG2_ERR_MASK;

#if VERBOSE_DEBUG
    printf("resolver: Incoming response for \"%s\" query.\n",namemapptr->name);
#endif

    /* Check for error. If so, call callback to inform. */
    if(namemapptr->err != 0) {
      namemapptr->state = STATE_ERROR;
      resolv_found(namemapptr->name, NULL);
      return;
    }

#if VERBOSE_DEBUG
    printf("resolver: nquestions=%d, nanswers=%d\n",nquestions,nanswers);
#endif

    nameptr = (char*)hdr+sizeof(*hdr);

    while(nquestions > 0) {
      if(*nameptr & 0xc0) {
	/* Compressed name. */
	nameptr +=2;
#if VERBOSE_DEBUG
		printf("resolver: Compressed anwser\n");
#endif
      } else {
	/* Not compressed name. */
    /* Skip the name in the question. XXX: This should really be
       checked agains the name in the question, to be sure that they
       match. */
	nameptr = parse_name((uint8_t *)nameptr);
      }
	  nameptr += 4; // Skip past question data
	  nquestions--;
	}

    while(nanswers > 0) {
      /* The first byte in the answer resource record determines if it
	 is a compressed record or a normal one. */
      if(*nameptr & 0xc0) {
	/* Compressed name. */
	nameptr +=2;
#if VERBOSE_DEBUG
		printf("resolver: Compressed anwser\n");
#endif
      } else {
	/* Not compressed name. */
    /* Skip the name in the question. XXX: This should really be
       checked agains the name in the question, to be sure that they
       match. */
	nameptr = parse_name((uint8_t *)nameptr);
      }

      ans = (struct dns_answer *)nameptr;
#if VERBOSE_DEBUG
           printf("resolver: Answer: type %d, class %d, ttl %d, length %d\n",
        		   uip_ntohs(ans->type), uip_ntohs(ans->class)&0x7FFF, (int)((uint32_t)uip_ntohs(ans->ttl[0])
	     << 16) | (uint32_t)uip_ntohs(ans->ttl[1]), uip_ntohs(ans->len));
#endif


      /* Check for IP address type and Internet class. Others are
	 discarded. */

#if UIP_CONF_IPV6
      if((ans->type == HTONS(DNS_TYPE_AAAA)) && ((uip_ntohs(ans->class)&0x7FFF) == DNS_CLASS_IN) && (ans->len == HTONS(16)))
#else // UIP_CONF_IPV6
      if(ans->type == HTONS(DNS_TYPE_A) && (uip_ntohs(ans->class)&0x7FFF == DNS_CLASS_IN) && (ans->len == HTONS(4)))
#endif
      {
#if VERBOSE_DEBUG
		printf("resolver: Answer is usable.\n");
#endif
		namemapptr->state = STATE_DONE;
	    uip_ipaddr_copy(&namemapptr->ipaddr,(uip_ipaddr_t*)ans->ipaddr);
		resolv_found(namemapptr->name, &namemapptr->ipaddr);
		return;
      } else {
	  // Keep looking.
	nameptr = nameptr + 10 + htons(ans->len);
      }
      --nanswers;
    }
  } else {
#if VERBOSE_DEBUG
    printf("resolver: Bad ID (%04X) on incoming DNS response\n",htons(hdr->id));
#endif
  }
}
/*-----------------------------------------------------------------------------------*/
/** \internal
 * The main UDP function.
 */
/*-----------------------------------------------------------------------------------*/
PROCESS_THREAD(resolv_process, ev, data)
{
  int i;
  
  PROCESS_BEGIN();

  memset(names,0,sizeof(names));

  resolv_conn = NULL;
  resolv_event_found = process_alloc_event();

#if VERBOSE_DEBUG
	printf("resolver: Process started.\n");
#endif

#if RESOLV_CONF_SUPPORTS_MDNS
	printf("resolver: Supports MDNS name resolution.\n");
	if(!mdns_conn) {
		mdns_conn = udp_new(NULL,0,NULL);
#if RESOLV_CONF_MDNS_RESPONDER
		uip_udp_bind(mdns_conn,HTONS(MDNS_PORT));
#else
		uip_udp_bind(mdns_conn,HTONS(61619));
#endif
		mdns_conn->rport = 0;
	}
#endif
    
  while(1) {
    PROCESS_WAIT_EVENT();
    
    if(ev == PROCESS_EVENT_TIMER) {
	  check_entries();
      if(resolv_conn != NULL) {
        tcpip_poll_udp(resolv_conn);
      }

    } else if(ev == EVENT_NEW_SERVER) {
      if(resolv_conn != NULL) {
	uip_udp_remove(resolv_conn);
      }
      resolv_conn = udp_new((uip_ipaddr_t *)data, HTONS(DNS_PORT), NULL);
#if VERBOSE_DEBUG
	printf("resolver: EVENT_NEW_SERVER: resolv_conn=%p (data=%p) UIP_UDP_CONNS=%d\n",resolv_conn,data,UIP_UDP_CONNS);
#endif
      
    } else if(ev == tcpip_event) {
		if((uip_udp_conn == resolv_conn)
#if RESOLV_CONF_SUPPORTS_MDNS
			|| (uip_udp_conn == mdns_conn)
#endif
		) {
			if(uip_poll()) {
				check_entries();
			}
			if(uip_newdata()) {
				newdata();
			}
		}
	}
  }
  
  PROCESS_END();
}

static char dns_name_without_dots[32]; // For removing trailing dots.

/*-----------------------------------------------------------------------------------*/
/**
 * Queues a name so that a question for the name will be sent out.
 *
 * \param name The hostname that is to be queried.
 */
/*-----------------------------------------------------------------------------------*/
void
resolv_query(const char *name)
{
  static u8_t i;
  static u8_t lseq, lseqi;
  register struct namemap *nameptr;
      
  lseq = lseqi = 0;
  nameptr = 0;                //compiler warning if not initialized

  {	// Remove trailing dots, if present.
	size_t len = strlen(name);
	if(name[len-1]=='.') {
		strncpy(dns_name_without_dots,name,sizeof(dns_name_without_dots));
		while(len && (dns_name_without_dots[len-1]=='.')) {
			dns_name_without_dots[--len]=0;
		}
		name = dns_name_without_dots;
	}
  }
  
  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    nameptr = &names[i];
    if((nameptr->state == STATE_UNUSED) || (0==strcmp(nameptr->name,name))) {
      break;
    }
    if(seqno - nameptr->seqno > lseq) {
      lseq = seqno - nameptr->seqno;
      lseqi = i;
    }
  }

  if(i == RESOLV_ENTRIES) {
    i = lseqi;
    nameptr = &names[i];
  }

#if VERBOSE_DEBUG
	printf("resolver: Starting query for \"%s\".\n",name);
#endif
  

#if RESOLV_CONF_SUPPORTS_MDNS
	{
		size_t name_len = strlen(name);
		static const char local_suffix[] = "local";
		if((name_len>(sizeof(local_suffix)-1))
			&& (0==strcmp(name+name_len-(sizeof(local_suffix)-1),local_suffix))
		) {
#if VERBOSE_DEBUG
			printf("resolver: Using MDNS to look up \"%s\".\n",name);
#endif
			nameptr->is_mdns = 1;
		} else {
			nameptr->is_mdns = 0;
		}
	}
#endif

  strncpy(nameptr->name, name, sizeof(nameptr->name));
  nameptr->state = STATE_NEW;
  nameptr->seqno = seqno;
  ++seqno;

  // Force check_entires() to run on our process.
  process_post(&resolv_process, PROCESS_EVENT_TIMER, 0);
}
/*-----------------------------------------------------------------------------------*/
/**
 * Look up a hostname in the array of known hostnames.
 *
 * \note This function only looks in the internal array of known
 * hostnames, it does not send out a query for the hostname if none
 * was found. The function resolv_query() can be used to send a query
 * for a hostname.
 *
 * \return A pointer to a 4-byte representation of the hostname's IP
 * address, or NULL if the hostname was not found in the array of
 * hostnames.
 */
/*-----------------------------------------------------------------------------------*/
uip_ipaddr_t *
resolv_lookup(const char *name)
{
  static u8_t i;
  struct namemap *nameptr;

  {	// Remove trailing dots, if present.
	size_t len = strlen(name);
	if(name[len-1]=='.') {
		strncpy(dns_name_without_dots,name,sizeof(dns_name_without_dots)-1);
		name = dns_name_without_dots;
		while(len && (dns_name_without_dots[len-1]=='.')) {
			dns_name_without_dots[--len]=0;
		}
	}
  }

  /* Walk through the list to see if the name is in there. If it is
     not, we return NULL. */
  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    nameptr = &names[i];
    if(nameptr->state == STATE_DONE
		&& (strcmp(name, nameptr->name) == 0)
	) {
#if VERBOSE_DEBUG
	printf("resolver: Found \"%s\" in cache.\n",name);
	uip_ipaddr_t *addr=&nameptr->ipaddr;
	
	printf("resolver: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x \n", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15]);
#endif
      return &nameptr->ipaddr;
    }
  }
#if VERBOSE_DEBUG
	printf("resolver: \"%s\" is NOT cached.\n",name);
#endif
  return NULL;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Obtain the currently configured DNS server.
 *
 * \return A pointer to a 4-byte representation of the IP address of
 * the currently configured DNS server or NULL if no DNS server has
 * been configured.
 */
/*-----------------------------------------------------------------------------------*/
uip_ipaddr_t *
resolv_getserver(void)
{
  if(resolv_conn == NULL) {
    return NULL;
  }
  return &resolv_conn->ripaddr;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Configure a DNS server.
 *
 * \param dnsserver A pointer to a 4-byte representation of the IP
 * address of the DNS server to be configured.
 */
/*-----------------------------------------------------------------------------------*/
void
resolv_conf(const uip_ipaddr_t *dnsserver)
{
  static uip_ipaddr_t server;
  uip_ipaddr_copy(&server, dnsserver);
  process_post(&resolv_process, EVENT_NEW_SERVER, &server);
}
/*-----------------------------------------------------------------------------------*/
/** \internal
 * Callback function which is called when a hostname is found.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
resolv_found(char *name, uip_ipaddr_t *ipaddr)
{
#if VERBOSE_DEBUG
	if(addr) {
		printf("resolver: Found address for \"%s\".\n",name);
		printf("resolver: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x \n", ((u8_t *)ipaddr)[0], ((u8_t *)ipaddr)[1], ((u8_t *)ipaddr)[2], ((u8_t *)ipaddr)[3], ((u8_t *)ipaddr)[4], ((u8_t *)ipaddr)[5], ((u8_t *)ipaddr)[6], ((u8_t *)ipaddr)[7], ((u8_t *)ipaddr)[8], ((u8_t *)ipaddr)[9], ((u8_t *)ipaddr)[10], ((u8_t *)ipaddr)[11], ((u8_t *)ipaddr)[12], ((u8_t *)ipaddr)[13], ((u8_t *)ipaddr)[14], ((u8_t *)ipaddr)[15]);
	} else {
		printf("resolver: Unable to retrieve address for \"%s\".\n",name);
	}
#endif

  process_post(PROCESS_BROADCAST, resolv_event_found, name);
}
/*-----------------------------------------------------------------------------------*/
#endif /* UIP_UDP */

/** @} */
/** @} */
