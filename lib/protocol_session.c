/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"

#define NYI() assert(0)

extern void
proto_session_dump(Proto_Session *s)
{
  fprintf(stderr, "Session s=%p:\n", s);
  fprintf(stderr, " fd=%d, extra=%p slen=%d, rlen=%d\n shdr:\n  ", 
	  s->fd, s->extra,
	  s->slen, s->rlen);
  proto_dump_msghdr(&(s->shdr));
  fprintf(stderr, " rhdr:\n  ");
  proto_dump_msghdr(&(s->rhdr));
}

extern void
proto_session_init(Proto_Session *s)
{
  if (s) bzero(s, sizeof(Proto_Session));
}

extern void
proto_session_reset_send(Proto_Session *s)
{
  bzero(&s->shdr, sizeof(s->shdr));
  s->slen = 0;
}

extern void
proto_session_reset_receive(Proto_Session *s)
{
  bzero(&s->rhdr, sizeof(s->rhdr));
  s->rlen = 0;
}

static void
proto_session_hdr_marshall_sver(Proto_Session *s, Proto_StateVersion v)
{
  s->shdr.sver.raw = htonll(v.raw);
}

static void
proto_session_hdr_unmarshall_sver(Proto_Session *s, Proto_StateVersion *v)
{
  v->raw = ntohll(s->rhdr.sver.raw);
}

static void // Changed 
proto_session_hdr_marshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{
    s->shdr.pstate.v0.raw = htonl(ps->v0.raw);

    // changed
    s->shdr.pstate.v1.raw = htonl(ps->v1.raw);
    s->shdr.pstate.v2.raw = htonl(ps->v2.raw);
    s->shdr.pstate.v3.raw = htonl(ps->v3.raw);

}

static void // changed
proto_session_hdr_unmarshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{
    ps->v0.raw = ntohl(s->rhdr.pstate.v0.raw);
    ps->v1.raw = ntohl(s->rhdr.pstate.v1.raw);
    ps->v2.raw = ntohl(s->rhdr.pstate.v2.raw);
    ps->v3.raw = ntohl(s->rhdr.pstate.v3.raw);

}

static void // changed
proto_session_hdr_marshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
  s->shdr.gstate.v0.raw = htonl(gs->v0.raw);
  s->shdr.gstate.v1.raw = htonl(gs->v1.raw);
  s->shdr.gstate.v2.raw = htonl(gs->v2.raw);
}

static void // changed
proto_session_hdr_unmarshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
  gs->v0.raw = ntohl(s->rhdr.gstate.v0.raw);
  gs->v1.raw = ntohl(s->rhdr.gstate.v1.raw);
  gs->v2.raw = ntohl(s->rhdr.gstate.v2.raw);
}

static int // changed
proto_session_hdr_unmarshall_blen(Proto_Session *s)
{
  return ntohl(s->rhdr.blen);
}

static void // changed
proto_session_hdr_marshall_type(Proto_Session *s, Proto_Msg_Types t)
{
  s->shdr.type = htonl(t);
}

extern Proto_Msg_Types // changed
proto_session_hdr_unmarshall_type(Proto_Session *s)
{
  return ntohl(s->rhdr.type);
}

static int // changed
proto_session_hdr_unmarshall_version(Proto_Session *s)
{
  return ntohll(s->rhdr.version);
}

extern void
proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  
  h->version = proto_session_hdr_unmarshall_version(s);
  h->type = proto_session_hdr_unmarshall_type(s);
  proto_session_hdr_unmarshall_sver(s, &h->sver);
  proto_session_hdr_unmarshall_pstate(s, &h->pstate);
  proto_session_hdr_unmarshall_gstate(s, &h->gstate);
  h->blen = proto_session_hdr_unmarshall_blen(s);
}
   
extern void
proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  // ignore the version number and hard code to the version we support
  s->shdr.version = PROTOCOL_BASE_VERSION;
  proto_session_hdr_marshall_type(s, h->type);
  proto_session_hdr_marshall_sver(s, h->sver);
  proto_session_hdr_marshall_pstate(s, &h->pstate);
  proto_session_hdr_marshall_gstate(s, &h->gstate);
  // we ignore the body length as we will explicity set it
  // on the send path to the amount of body data that was
  // marshalled.
}

extern int 
proto_session_body_marshall_ll(Proto_Session *s, long long v)
{
  if (s && ((s->slen + sizeof(long long)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonll(v);
    s->slen+=sizeof(long long);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_ll(Proto_Session *s, int offset, long long *v)
{
  if (s && ((s->rlen - (offset + sizeof(long long))) >=0 )) {
    *v = *((long long *)(s->rbuf + offset));
    *v = htonl(*v);
    return offset + sizeof(long long);
  }
  return -1;
}

extern int 
proto_session_body_marshall_int(Proto_Session *s, int v)
{
  if (s && ((s->slen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonl(v);
    s->slen+=sizeof(int);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_int(Proto_Session *s, int offset, int *v)
{
  //fprintf(stderr, "Got inside body_unmarshall_int\n");
  if (s && ((s->rlen  - (offset + sizeof(int))) >=0 )) {
    //fprintf(stderr, "Got inside if statement.\n");
    *v = *((int *)(s->rbuf + offset));
    //fprintf(stderr, "Interpreted integer at (s->rbuf + offset)\n");
    *v = htonl(*v);
    //fprintf(stderr, "Unmarshalled integer\n");
    return offset + sizeof(int);
  }
  //fprintf(stderr, "Returned -1 NOOOOOOO\n");
  return -1;
}

extern int 
proto_session_body_marshall_char(Proto_Session *s, char v)
{
  if (s && ((s->slen + sizeof(char)) < PROTO_SESSION_BUF_SIZE)) {
    s->sbuf[s->slen] = v;
    s->slen+=sizeof(char);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_char(Proto_Session *s, int offset, char *v)
{
  if (s && ((s->rlen - (offset + sizeof(char))) >= 0)) {
    *v = s->rbuf[offset];
    return offset + sizeof(char);
  }
  return -1;
}

extern int
proto_session_body_reserve_space(Proto_Session *s, int num, char **space)
{
  if (s && ((s->slen + num) < PROTO_SESSION_BUF_SIZE)) {
    *space = &(s->sbuf[s->slen]);
    s->slen += num;
    return 1;
  }
  *space = NULL;
  return -1;
}

extern int
proto_session_body_ptr(Proto_Session *s, int offset, char **ptr)
{
  if (s && ((s->rlen - offset) > 0)) {
    *ptr = &(s->rbuf[offset]);
    return 1;
  }
  return -1;
}
	    
extern int
proto_session_body_marshall_bytes(Proto_Session *s, int len, char *data)
{
  if (s && ((s->slen + len) < PROTO_SESSION_BUF_SIZE)) {
    memcpy(s->sbuf + s->slen, data, len);
    s->slen += len;
    return 1;
  }
  return -1;
}

extern int
proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, int len, 
				     char *data)
{
  if (s && ((s->rlen - (offset + len)) >= 0)) {
    memcpy(data, s->rbuf + offset, len);
    return offset + len;
  }
  return -1;
}

// rc < 0 on comm failures
// rc == 1 indicates comm success
extern  int
proto_session_send_msg(Proto_Session *s, int reset)
{
  //NYI();
  //fprintf(stderr, "Before marshaling, slen = %d\n", s->slen);
  s->shdr.blen = htonl(s->slen);
  //fprintf(stderr, "After marshaling, slen (or s->shdr.blen) = %d\n", s->shdr.blen);

  // write request
  // changed
  // int k;
  // for (k = 0; k < PROTO_SESSION_BUF_SIZE; k++)
  // {
  //   fprintf(stderr,"%c", s->sbuf[k]);
  // }
  // fprintf(stderr, "\n");
  // fprintf(stderr, "Writing bytes to fd %d\n", s->fd);
  // fprintf(stderr, "Address of shdr = %x\n", &(s->shdr));
  // fprintf(stderr, "sizeof(Proto_Msg_Hdr) = %d\n", sizeof(Proto_Msg_Hdr));
  if (net_writen(s->fd, &(s->shdr), sizeof(Proto_Msg_Hdr)) == -1)
  {
    return -1;
  }
  // fprintf(stderr, "Wrote message type: %d", s->shdr.type);
  // fprintf(stderr, "Wrote version: %d", s->shdr.version);
  // fprintf(stderr, "Wrote blen: %d", s->shdr.blen);

  //fprintf(stderr, "Before sending, sbuf = ");
  int i;
  // for (i = 0; i < 9; i++)
  // {
  //   fprintf(stderr, "%c ", s->sbuf[i]);
  // }

  if (net_writen(s->fd, &(s->sbuf), s->slen) == -1)
  {
    return -1;
  }
  
  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_send_msg: SENT:\n", pthread_self());
    proto_session_dump(s);
  }

  // communication was successfull 
  if (reset) proto_session_reset_send(s);

  return 1;
}

extern int
proto_session_rcv_msg(Proto_Session *s)
{
  //NYI();

  
  proto_session_reset_receive(s);

  // read reply
  // changed

 
  int ret = net_readn(s->fd, &(s->rhdr), sizeof(Proto_Msg_Hdr));
  if (ret == -1)
  {
    return -1;
  }
  else if (ret != 0)
  {
    //proto_dump_msghdr(&(s->rhdr));
    //fprintf(stderr, "Received message type: %d\n", s->rhdr.type);
    //fprintf(stderr, "Received version: %d\n", s->rhdr.version);
    //fprintf(stderr, "Received blen: %d\n", s->rhdr.blen);
  }

  int newblen = ntohl(s->rhdr.blen);

  if (net_readn(s->fd, &(s->rbuf), newblen) == -1)
    {
      return -1;
    }

  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_rcv_msg: RCVED:\n", pthread_self());
    proto_session_dump(s);
  }
  return 1;
}

extern int
proto_session_rpc(Proto_Session *s) 
{
   int rc;
   // fprintf(stderr, "Before unmarshalling, mt = %d\n", s->shdr.type);
   // int unmarshalled = ntohl(s->shdr.type);
   // fprintf(stderr, "After unmarshalling, mt = %d\n", unmarshalled);

   if (proto_session_send_msg(s, 1) != -1)
   {
      //fprintf(stderr, "Sent message correctly in proto_session_rpc.\n");
      rc = proto_session_rcv_msg(s);
   }
   else
   {
      rc = -1;
   }

   return rc;
  //NYI();
}

