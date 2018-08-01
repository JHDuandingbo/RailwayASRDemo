/*
#include <nopoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <nopoll.h>
*/
#include "defs.h"
//browser websocket client;
int ws_send_msg(const char * msg, unsigned int  len);
static noPollConn * g_conn = NULL;
static noPollCtx      * ctx = NULL;
static const char *  PORT_NO = "60001";
static void __nopoll_regression_on_close (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data){
	g_conn=NULL;
	printf ("browser conn disconnected  !!\n");
	return;
}
static nopoll_bool on_connection_ready (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data){
	g_conn = conn;
	char test[BUFSIZ] = {0};
	sprintf(test, "Hello word\n\n");
	//        nopoll_conn_send_binary(g_conn,  test, 20);
}
static nopoll_bool on_connection_opened (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data){
	/* set connection close */
	printf("got connection from browser\n");
	//g_conn = conn;
	nopoll_conn_set_on_close (conn, __nopoll_regression_on_close, NULL);
	if (! nopoll_conn_set_sock_block (nopoll_conn_socket (conn), nopoll_false)) {
		printf ("ERROR: failed to configure non-blocking state to connection..\n");
		return nopoll_false;
	} /* end if */
	//printf("origin:%s, proto:%s, host:%s, \n", nopoll_conn_get_origin(conn), nopoll_conn_get_requested_protocol(conn), nopoll_conn_get_host_header(conn));
	return nopoll_true;
}
static void listener_on_message (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data){
	int bytes = nopoll_msg_get_payload_size (msg);
	//printf ("FIN = %d,Got %d bytes from browser\n", nopoll_msg_is_final (msg),bytes);
	char * content = (char *) nopoll_msg_get_payload (msg);
	printf("got %d bytes from browser, neglect!\n", bytes);
	return;
}


static void __terminate_listener (int value){
	printf ("__terminate_listener: Signal received...terminating listener..\n");
	nopoll_loop_stop (ctx);
	return;
}
int ws_send_msg(const char * msg, unsigned int  len){
	int ret = 0;
	if(g_conn == NULL){
		printf("browser not online  !\n");
		ret = -1;
	}else{
		ret=nopoll_conn_send_binary(g_conn, msg, len);
	}
	return ret;
}
void * setup_ws_server (void*){
	noPollConn     * listener;
	signal (SIGTERM,  __terminate_listener);
	ctx = nopoll_ctx_new ();
	listener = nopoll_listener_new (ctx, "0.0.0.0", PORT_NO);
	if (! nopoll_conn_is_ok (listener)) {
		printf ("ERROR: Expected to find proper listener connection status, but found..\n");
		return NULL;
	}
	printf ("noPoll listener started at: %s:%s (refs: %d)..\n", nopoll_conn_host (listener), nopoll_conn_port (listener), nopoll_conn_ref_count (listener));
	nopoll_ctx_set_on_msg (ctx, listener_on_message, NULL);
	nopoll_ctx_set_on_open (ctx, on_connection_opened, NULL);
	nopoll_ctx_set_on_ready (ctx, on_connection_ready, NULL);
	nopoll_loop_wait (ctx, 0);
	nopoll_conn_close (listener);
	//printf ("Listener: finishing references: %d\n", nopoll_ctx_ref_count (ctx));
	nopoll_ctx_unref (ctx);
	nopoll_cleanup_library ();
	return NULL;
}


