/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: sicslowmac.c,v 1.8 2010/06/14 19:19:17 adamdunkels Exp $
 */


/**
 * \file
 *         MAC interface for packaging radio packets into 802.15.4 frames
 *
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Eric Gnoske <egnoske@gmail.com>
 *         Blake Leverett <bleverett@gmail.com>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <string.h>
#include <math.h>
#include "net/mac/sicslowmac.h"
#include "net/mac/frame802154.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/netstack.h"
#include "lib/random.h"
#include "radio/rf212bb/rf212bb.h"
#include "net/uip.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#else
#define PRINTF(...)
#define PRINTADDR(addr)
#endif

/**  \brief The sequence number (0x00 - 0xff) added to the transmitted
 *   data or MAC command frame. The default is a random value within
 *   the range.
 */
static uint8_t mac_dsn;

/**  \brief The frame counter (0x00000000 - 0xffffffff) added to the auxiliary security
 * header and is part of the nonce.
 */
uint32_t mac_framecnt;

/**  \brief The key counter (0x00 - 0xff) added to the auxiliary security
 * header and is part of the nonce.
 */
uint8_t mac_keycnt;

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   sending to.  If this value is 0xffff, the device is not
 *   associated.
 */
uint16_t mac_dst_pan_id = IEEE802154_PANID;

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   operating.  If this value is 0xffff, the device is not
 *   associated.
 */
uint16_t mac_src_pan_id = IEEE802154_PANID;

extern uint8_t encryption_enabled; //global variable for AES encryption


/*---------------------------------------------------------------------------*/
static int
is_broadcast_addr(uint8_t mode, uint8_t *addr)
{
  int i = mode == FRAME802154_SHORTADDRMODE ? 2 : 8;
  while(i-- > 0) {
    if(addr[i] != 0xff) {
      return 0;
    }
  }
  return 1;
}

static uint8_t authtag_size(uint8_t sl)
{
	switch (sl & 3) {
	case 1: return 4;
	case 2: return 8;
	case 3: return 16;
	default: return 0;
	}
}

static void
update_mac(void *mac, const void *data, uint16_t len)
{
	uint8_t *umac = mac;
	const uint8_t *udata = data;
	uint8_t i;

	while (len >= 16) {
		for (i = 0; i < 16; i++)
			umac[i] ^= udata[i];

		rf212_cipher(umac);

		udata += 16;
		len -= 16;
	}

	if (len) {
		for (i = 0; i < len; i++)
			umac[i] ^= udata[i];

		rf212_cipher(umac);
	}
}

static void
calc_mac_assoc(void *mac, const void *assoc, uint16_t len)
{
	uint8_t *umac = mac;
	const uint8_t *uassoc = assoc;
	uint8_t plen;
	uint8_t buf[16];

	memset(buf, 0, sizeof(buf));

	if (len == 0) {
		return;
	} else if (len < 65280) {
		buf[0] = len >> 8;
		buf[1] = len & 0xFF;
		plen = 2;
	} else {
		buf[0] = 0xff;
		buf[1] = 0xfe;
		buf[4] = len >> 8;
		buf[5] = len & 0xFF;
		plen = 6;
	}

	if (len >= sizeof(buf) - plen) {
		memcpy(buf + plen, uassoc, sizeof(buf) - plen);
		len -= sizeof(buf) - plen;
		uassoc += sizeof(buf) - plen;
	} else {
		memcpy(buf + plen, uassoc, len);
		len = 0;
	}

	update_mac(mac, buf, sizeof(buf));
	if (len)
		update_mac(mac, uassoc, len);
}

static void
compute_mac(frame802154_t *pf, void *mac, const void *adata, uint16_t alen,
		const void *data, uint16_t len)
{
	nonce_802154_t nonce;

	rimeaddr_copy((rimeaddr_t *)&nonce.mac_addr, (rimeaddr_t *)&pf->src_addr);
	nonce.frame_cnt = uip_htonl(pf->aux_hdr.frame_counter);
	nonce.sec_level = pf->aux_hdr.security_control.security_level;
	nonce.block_cnt = uip_htons(len);

	nonce.flags = 1;
	nonce.flags |= ((authtag_size(nonce.sec_level) - 2) / 2) << 3;
	if (alen) {
		nonce.flags |= 1 << 6;
	}

	memcpy(mac, &nonce, 16);
	rf212_cipher(mac);

	calc_mac_assoc(mac, adata, alen);
	update_mac(mac, data, len);
}

static void
transform_block(nonce_802154_t *nonce, uint16_t block, uint8_t *data, uint16_t len)
{
	uint8_t k[16], i;

	nonce->block_cnt = uip_htons(block);
	memcpy(k, nonce, 16);
	rf212_cipher(k);

	for (i = 0; i < len; i++)
		data[i] ^= k[i];
}

/*---------------------------------------------------------------------------*/
int
encrypt_payload(frame802154_t *pf)
{
	nonce_802154_t nonce;
	uint8_t i, alen;
	uint8_t mac[16];

	if (pf->aux_hdr.security_control.security_level == 0)
		return 0;

	nonce.flags = 1;
	rimeaddr_copy((rimeaddr_t *)&nonce.mac_addr, (rimeaddr_t *)&pf->src_addr);
	nonce.frame_cnt = uip_htonl(pf->aux_hdr.frame_counter);
	nonce.sec_level = pf->aux_hdr.security_control.security_level;
	nonce.block_cnt = 0;

	PRINTF("encrypt nonce:");
	for (i = 0; i < 16; i++)
		PRINTF(" %02x", ((uint8_t*) &nonce)[i]);
	PRINTF("\n");

	if (nonce.sec_level < 4) {
		compute_mac(pf, mac, packetbuf_hdrptr(), packetbuf_totlen(), 0, 0);
	} else if (nonce.sec_level > 4) {
		compute_mac(pf, mac, packetbuf_hdrptr(), packetbuf_hdrlen(),
				packetbuf_dataptr(), packetbuf_datalen());
	}
	transform_block(&nonce, 0, mac, 16);

	if (nonce.sec_level > 4) {
		uint8_t len_left = packetbuf_datalen();
		uint8_t block = 0;
		while (len_left > 0) {
			i = len_left > 16 ? 16 : len_left;
			transform_block(&nonce, block + 1, (uint8_t*) packetbuf_dataptr() + 16 * block, i);
			len_left -= i;
			block++;
		}
	}

	alen = authtag_size(nonce.sec_level);
	memcpy((char*) packetbuf_dataptr() + packetbuf_datalen(), mac, alen);
	packetbuf_set_datalen(packetbuf_datalen() + alen);

	return 0;
}

/*---------------------------------------------------------------------------*/
int
decrypt_payload(frame802154_t *pf, uint8_t hlen)
{
	nonce_802154_t nonce;
	uint8_t i, alen;
	uint8_t mac[16];

	if (pf->aux_hdr.security_control.security_level == 0)
		return 0;

	nonce.flags = 1;
	rimeaddr_copy((rimeaddr_t *)&nonce.mac_addr, (rimeaddr_t *)&pf->src_addr);
	nonce.frame_cnt = uip_htonl(pf->aux_hdr.frame_counter);
	nonce.sec_level = pf->aux_hdr.security_control.security_level;
	nonce.block_cnt = 0;

	PRINTF("decrypt nonce:");
	for (i = 0; i < 16; i++)
		PRINTF(" %02x", ((uint8_t*) &nonce)[i]);
	PRINTF("\n");

	alen = authtag_size(nonce.sec_level);

	if (nonce.sec_level > 4) {
		uint8_t len_left = packetbuf_datalen() - alen;
		uint8_t block = 0;
		while (len_left > 0) {
			i = len_left > 16 ? 16 : len_left;
			transform_block(&nonce, block + 1, (uint8_t*) packetbuf_dataptr() + 16 * block, i);
			len_left -= i;
			block++;
		}
	}

	if (nonce.sec_level < 4) {
		compute_mac(pf, mac, (const char*) packetbuf_dataptr() - hlen,
				packetbuf_datalen() + hlen - alen, 0, 0);
	} else if (nonce.sec_level > 4) {
		compute_mac(pf, mac, (const char*) packetbuf_dataptr() - hlen,
				hlen, packetbuf_dataptr(), packetbuf_datalen() - alen);
	}
	packetbuf_set_datalen(packetbuf_datalen() - alen);
	transform_block(&nonce, 0, (uint8_t*) packetbuf_dataptr() + packetbuf_datalen(), alen);

	if (memcmp(mac, (const char*) packetbuf_dataptr() + packetbuf_datalen(), alen)) {
		PRINTF("mac mismatch");
		for (i = 0; i < alen; i++) PRINTF(" %02x", mac[i]);
		PRINTF("\n");
		const uint8_t *macc = (const uint8_t*) packetbuf_dataptr() + packetbuf_datalen();
		PRINTF("mac input is");
		for (i = 0; i < alen; i++) PRINTF(" %02x", macc[i]);
		PRINTF("\n");
		return 1;
	}

	if (pf->aux_hdr.frame_counter > mac_framecnt)
		mac_framecnt = pf->aux_hdr.frame_counter;
	return 0;
}

/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  frame802154_t params;
  uint8_t len;

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_DATAFRAME;
  //TODO
  if (encryption_enabled) {
	  params.fcf.security_enabled = 1;
	  // auxiliary security header length = 6
  	  params.aux_hdr.security_control.key_id_mode = 1;
  	  // Encryption with mic32
  	  params.aux_hdr.security_control.security_level = 5;
  	  //set frame counter
  	  params.aux_hdr.frame_counter = mac_framecnt++;
	  // TODO: key rollover
	  params.aux_hdr.key[0] = mac_keycnt;
  } else
	  params.fcf.security_enabled = 0;
  params.fcf.frame_pending = 0;
  params.fcf.ack_required = packetbuf_attr(PACKETBUF_ATTR_RELIABLE);
  params.fcf.panid_compression = 0;

  /* Insert IEEE 802.15.4 (2003) version bit. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;

  /* Increment and set the data sequence number. */
  params.seq = mac_dsn++;

  /* Complete the addressing fields. */
  /**
     \todo For phase 1 the addresses are all long. We'll need a mechanism
     in the rime attributes to tell the mac to use long or short for phase 2.
  */
  params.fcf.src_addr_mode = FRAME802154_LONGADDRMODE;
  params.dest_pid = mac_dst_pan_id;

  /*
   *  If the output address is NULL in the Rime buf, then it is broadcast
   *  on the 802.15.4 network.
   */
  if(rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null)) {
    /* Broadcast requires short address mode. */
    params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    params.dest_addr[0] = 0xFF;
    params.dest_addr[1] = 0xFF;
    /*Disable 802.15.4 ACK for broadcast messages, else broadcasts will be duplicated by the retry count, since no one will ACK them!*/
    params.fcf.ack_required = 0;

  } else {
    rimeaddr_copy((rimeaddr_t *)&params.dest_addr,
                  packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    params.fcf.dest_addr_mode = FRAME802154_LONGADDRMODE;
  }

  /* Set the source PAN ID to the global variable. */
  params.src_pid = mac_src_pan_id;

  /*
   * Set up the source address using only the long address mode for
   * phase 1.
   */
  rimeaddr_copy((rimeaddr_t *)&params.src_addr, &rimeaddr_node_addr);

  params.payload = packetbuf_dataptr();
  params.payload_len = packetbuf_datalen();
  len = frame802154_hdrlen(&params);

  if(packetbuf_hdralloc(len)) {
    int ret;
    frame802154_create(&params, packetbuf_hdrptr(), len);

    if (encryption_enabled)
      encrypt_payload(&params);

//    PRINTF("6MAC-UT: %2X", params.fcf.frame_type);
//    PRINTADDR(params.dest_addr.u8);
//    PRINTF("%u %u (%u)\n", len, packetbuf_datalen(), packetbuf_totlen());

    ret = NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());
    if(sent) {
      switch(ret) {
      case RADIO_TX_OK:
        sent(ptr, MAC_TX_OK, 1);
        break;
      case RADIO_TX_ERR:
        sent(ptr, MAC_TX_ERR, 1);
        break;
      }
    }
  } else {
    PRINTF("6MAC-UT: too large header: %u\n", len);
  }
}

/*---------------------------------------------------------------------------*/
void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  if(buf_list != NULL) {
    queuebuf_to_packetbuf(buf_list->buf);
    send_packet(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
  frame802154_t frame;
  int len;

  len = packetbuf_datalen();

  if(frame802154_parse(packetbuf_dataptr(), len, &frame) &&
     packetbuf_hdrreduce(len - frame.payload_len)) {
    if(frame.fcf.dest_addr_mode) {
      if(frame.dest_pid != mac_src_pan_id &&
         frame.dest_pid != FRAME802154_BROADCASTPANDID) {
        /* Not broadcast or for our PAN */
        PRINTF("6MAC: for another pan %u\n", frame.dest_pid);
        return;
      }
      if(!is_broadcast_addr(frame.fcf.dest_addr_mode, frame.dest_addr)) {
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, (rimeaddr_t *)&frame.dest_addr);
        if(!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_node_addr)) {
          /* Not for this node */
          PRINTF("6MAC: not for us\n");
          return;
        }
      }
    }
    if (frame.fcf.security_enabled && decrypt_payload(&frame, len - frame.payload_len))
      return;

    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (rimeaddr_t *)&frame.src_addr);

    PRINTF("6MAC-IN: %2X", frame.fcf.frame_type);
    PRINTADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    PRINTADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    PRINTF("%u\n", packetbuf_datalen());
    NETSTACK_MAC.input();
  } else {
    PRINTF("6MAC: failed to parse hdr\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  if(keep_radio_on) {
    return NETSTACK_RADIO.on();
  } else {
    return NETSTACK_RADIO.off();
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  uint8_t aes_key[16], x;
  eeprom_read_block(aes_key, eep_addr(encryption_key), 16);
  PRINTF("key: ");
  for (int x = 0; x < 16; x++)
	PRINTF(" %02x", aes_key[x]);
  PRINTF("\n");
  rf212_key_setup(aes_key);
  mac_dsn = random_rand() % 256;

  NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver sicslowmac_driver = {
  "sicslowmac",
  init,
  send_packet,
  send_list,
  input_packet,
  on,
  off,
  channel_check_interval
};
/*---------------------------------------------------------------------------*/
