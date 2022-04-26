#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define DEF_PROTO_REQ_NAMES
#include "proto.h"


int spnav_send_str(int fd, int req, const char *str)
{
	int len;
	struct reqresp rr = {0};

	if(fd == -1) {
		return -1;
	}

	len = str ? strlen(str) : 0;
	rr.type = req;
	rr.data[6] = len;

	do {
		if(str) {
			memcpy(rr.data, str, len > REQSTR_CHUNK_SIZE ? REQSTR_CHUNK_SIZE : len);
		}
		write(fd, &rr, sizeof rr);
		str += REQSTR_CHUNK_SIZE;
		len -= REQSTR_CHUNK_SIZE;
		rr.data[6] = len | REQSTR_CONT_BIT;
	} while(len > 0);

	return 0;
}

int spnav_recv_str(struct reqresp_strbuf *sbuf, struct reqresp *rr)
{
	int len;

	if(rr->data[6] < 0) return -1;
	len = REQSTR_REMLEN(rr);

	if(REQSTR_FIRST(rr)) {
		/* first packet, allocate buffer */
		free(sbuf->buf);
		sbuf->expect = len;
		sbuf->size = sbuf->expect + 1;
		if(!(sbuf->buf = malloc(sbuf->size))) {
			return -1;
		}
		sbuf->endp = sbuf->buf;
	}

	if(!sbuf->size || !sbuf->buf || !sbuf->endp) {
		return -1;
	}
	if(sbuf->endp < sbuf->buf || sbuf->endp >= sbuf->buf + sbuf->size) {
		return -1;
	}
	if(sbuf->expect > sbuf->size) return -1;
	if(len != sbuf->expect) return -1;

	if(len > REQSTR_CHUNK_SIZE) {
		len = REQSTR_CHUNK_SIZE;
	}
	memcpy(sbuf->endp, rr->data, len);
	sbuf->endp += len;
	sbuf->expect -= len;

	if(sbuf->expect < 0) return -1;

	if(!sbuf->expect) {
		*sbuf->endp = 0;
		return 1;
	}
	return 0;
}
