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
            #include "protocol_server.h"

            #define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 1024
            #define NYI() assert(0)

            struct {
              FDType   RPCListenFD;
              PortType RPCPort;


              FDType             EventListenFD;
              PortType           EventPort;
              pthread_t          EventListenTid;
              pthread_mutex_t    EventSubscribersLock;
              int                EventLastSubscriber;
              int                EventNumSubscribers;
              FDType             EventSubscribers[PROTO_SERVER_MAX_EVENT_SUBSCRIBERS];
              Proto_Session      EventSession; 
              pthread_t          RPCListenTid;
              Proto_MT_Handler   session_lost_handler;
              Proto_MT_Handler   base_req_handlers[PROTO_MT_REQ_BASE_RESERVED_LAST - 
            				       PROTO_MT_REQ_BASE_RESERVED_FIRST-1];
            } Proto_Server;

            extern PortType proto_server_rpcport(void) { return Proto_Server.RPCPort; }
            extern PortType proto_server_eventport(void) { return Proto_Server.EventPort; }
            extern Proto_Session *
            proto_server_event_session(void) 
            { 
              return &Proto_Server.EventSession; 
            }

            extern int
            proto_server_set_session_lost_handler(Proto_MT_Handler h)
            {
              Proto_Server.session_lost_handler = h;
            }

            extern int
            proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h)
            {
              //NYI();
              int i;

              if (mt>PROTO_MT_REQ_BASE_RESERVED_FIRST &&
                  mt<PROTO_MT_REQ_BASE_RESERVED_LAST) {
                //i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
                i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST; // NOTE: Removed -1 from line above to make index of handler correspond to message type.
                Proto_Server.base_req_handlers[i] = h;
                //fprintf(stderr, "Set request handler for index %d of request handler array", i);
                return 1;
              } else {
                return -1;
              }
            }

            extern int
            proto_server_remove_event_subscriber(int index)
            {
              int rc;
              if (index >= 0 && index < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS)
              {
                Proto_Server.EventSubscribers[index] = -1;
                rc = 1;
              }
              else
              {
                rc = -1;
              }
              return rc;
            }


            static int
            proto_server_record_event_subscriber(int fd, int *num)
            {
              //NYI();
              int rc=-1;

              pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

              if (Proto_Server.EventLastSubscriber < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS
                  && Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]
                  ==-1) {
                Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber] = fd; // changed
                *num = Proto_Server.EventLastSubscriber;// changed
                Proto_Server.EventLastSubscriber++;// changed
                //fprintf()
                Proto_Server.EventNumSubscribers++;// changed
                fprintf(stderr, "Subscriber number %d with file descriptor %d recorded at index %d\n", Proto_Server.EventNumSubscribers, fd, (Proto_Server.EventLastSubscriber - 1));
                rc = 1;
              } else {
                int i;
                for (i=0; i< PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
                  if (Proto_Server.EventSubscribers[i]==-1) {
            	   Proto_Server.EventSubscribers[i] = fd;  // changed
            	   *num=i; // changed
                   Proto_Server.EventNumSubscribers++; // changed
            	   rc=1;
                  }
                }
              }
              fprintf(stderr, "Number of subscribers: %d\n", Proto_Server.EventNumSubscribers);
              pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

              return rc;
            }

            static
            void *
            proto_server_event_listen(void *arg)
            {
              //NYI();
              int fd = Proto_Server.EventListenFD;
              int connfd;

              if (net_listen(fd)<0) {
                exit(-1);
              }

              for (;;) {
                connfd = net_accept(fd);
                //printf("First");
                if (connfd < 0) {
                  fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
                } else {
                  int i;
                  //printf("Second");
                  fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

                  if (proto_server_record_event_subscriber(connfd, &i) <0) {
            	fprintf(stderr, "oops no space for any more event subscribers\n");
            	close(connfd);
                  } else {
            	fprintf(stderr, "subscriber num %d\n", i);
                  }
                } 
              }
            } 

            void
            proto_server_post_event(void) 
            {
              //NYI();
              int i;
              int num;

              pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

              i = 0;
              num = Proto_Server.EventNumSubscribers;
              while (num) {
                Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
                //fprintf(stderr, "Got fd #%d from index %d\n", Proto_Server.EventSession.fd, i);
                if (Proto_Server.EventSession.fd != -1) {
                  num--;
                  //int unmarshalled = ntohl(Proto_Server.EventSession.shdr.type);
                  //fprintf(stderr, "Server Event Session shdr mt = %d\n", unmarshalled);
                  //fprintf(stderr, "Server Event Session shdr blen = %d\n", Proto_Server.EventSession.shdr.blen);
                  if (proto_session_send_msg(&Proto_Server.EventSession, 0)<0) {
            	       // must have lost an event connection
            	       close(Proto_Server.EventSession.fd);
            	       Proto_Server.EventSubscribers[i]=-1;
            	       Proto_Server.EventNumSubscribers--;
                     //fprintf(stderr, "Post event failed.\n");
            	       Proto_Server.session_lost_handler(&Proto_Server.EventSession);
                  } 
                  // else 
                  // {
                  //   fprintf(stderr, "Apparently post_event worked correctly.\n");
                  // }
                  // while (/// time less than timeout)
                  // {
                  //     if (proto_session_rcv_msg(&s) <0)
                  //     {
                  //       // get reply
                  //       break;
                  //     }
                  // }
                  // FIXME: add ack message here to ensure that game is updated 
                  // correctly everywhere... at the risk of making server dependent
                  // on client behaviour (use time out to limit impact... drop
                  // clients that misbehave but be carefull of introducing deadlocks
                }
                i++;
              }
              proto_session_reset_send(&Proto_Server.EventSession);
              pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
            }


            static void *
            proto_server_req_dispatcher(void * arg)
            {
              //printf("Got into method.");
              //NYI();
              Proto_Session s;
              Proto_Msg_Types mt;
              Proto_MT_Handler hdlr;
              int i;
              unsigned long arg_value = (unsigned long) arg;
              //fprintf(stderr, "Top of method.");
              pthread_detach(pthread_self());

              //fprintf(stdout, "Detached thread.");
              proto_session_init(&s);
              //printf("Initialized session.");
              s.fd = (FDType) arg_value;
              //fprintf(stderr, "got here!!!");
              fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n", 
            	  pthread_self(), s.fd);

              //fprintf(stderr, "got here.");

              for (;;) {
                //fprintf(stderr, "got here");
                if (proto_session_rcv_msg(&s)==1) {
                  //fprintf(stderr, "Got inside the if statement.\n");
                  mt = proto_session_hdr_unmarshall_type(&s);

                  if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST && 
                  mt < PROTO_MT_REQ_BASE_RESERVED_LAST)
                  {
                    //fprintf(stderr, "Received message type %d\n", mt);
                    //fprintf(stderr, "Good mt = %d", mt);
                    hdlr = Proto_Server.base_req_handlers[mt];
                    //fprintf(stderr, "Set hdlr to request handler.\n");
            	      if (hdlr(&s)<0) goto leave;
                  }
                } else {
                  goto leave;
                }
             }
             leave:
              Proto_Server.session_lost_handler(&s);
              close(s.fd);
              return NULL;
            }

            static
            void *
            proto_server_rpc_listen(void *arg)
            {
              //NYI();
              int fd = Proto_Server.RPCListenFD;
              unsigned long connfd;
              pthread_t tid;
              
              if (net_listen(fd) < 0) {
                fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
                exit(-1);
              }

              for (;;) {
                connfd = net_accept(fd);
                if (connfd < 0) {
                  fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
                } else {
                  pthread_create(&tid, NULL, &proto_server_req_dispatcher,
            		     (void *)connfd);
                }
              }
            }

            extern int
            proto_server_start_rpc_loop(void)
            {
              if (pthread_create(&(Proto_Server.RPCListenTid), NULL, 
            		     &proto_server_rpc_listen, NULL) !=0) {
                fprintf(stderr, 
            	    "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
                perror("pthread_create:");
                return -3;
              }
              return 1;
            }

            static int 
            proto_session_lost_default_handler(Proto_Session *s)
            {
              fprintf(stderr, "Session lost...:\n");
              proto_session_dump(s);
              return -1;
            }

            static int 
            proto_server_mt_null_handler(Proto_Session *s)
            {
              int rc=1;
              Proto_Msg_Hdr h;
              
              fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
              proto_session_dump(s);

              // setup dummy reply header : set correct reply message type and 
              // everything else empty
              bzero(&h, sizeof(s));
              h.type = proto_session_hdr_unmarshall_type(s);
              h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
              proto_session_hdr_marshall(s, &h);

              // setup a dummy body that just has a return code 
              proto_session_body_marshall_int(s, 0xdeadbeef);

              rc=proto_session_send_msg(s,1);

              return rc;
            }

            extern int
            proto_server_init(void)
            {
              //NYI();
              int i;
              int rc;

              proto_session_init(&Proto_Server.EventSession);

              proto_server_set_session_lost_handler(
            				     proto_session_lost_default_handler);
              for (i=PROTO_MT_REQ_BASE_RESERVED_FIRST+1; 
                   i<PROTO_MT_REQ_BASE_RESERVED_LAST; i++) {
                Proto_Server.base_req_handlers[i] = proto_server_mt_null_handler;
              }


              for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
                Proto_Server.EventSubscribers[i]=-1;
              }
              Proto_Server.EventNumSubscribers=0;
              Proto_Server.EventLastSubscriber=0;
              pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);


              rc=net_setup_listen_socket(&(Proto_Server.RPCListenFD),
            			     &(Proto_Server.RPCPort));

              if (rc==0) { 
                fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for RPCPort\n");
                return -1;
              }

              Proto_Server.EventPort = Proto_Server.RPCPort + 1;

              rc=net_setup_listen_socket(&(Proto_Server.EventListenFD),
            			     &(Proto_Server.EventPort));

              if (rc==0) { 
                fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for EventPort=%d\n", 
            	    Proto_Server.EventPort);
                return -2;
              }

              if (pthread_create(&(Proto_Server.EventListenTid), NULL, 
            		     &proto_server_event_listen, NULL) !=0) {
                fprintf(stderr, 
            	    "proto_server_init: pthread_create: create EventListen thread failed\n");
                perror("pthread_createt:");
                return -3;
              }

              return 0;
            }
