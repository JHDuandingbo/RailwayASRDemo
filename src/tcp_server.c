#include "defs.h"
static struct ev_loop *loop = NULL;
static ev_io accept_io;
static ev_timer timer;
static int PORT_NO = 60000;
static int total_clients  =0;
static void accept_cb(EV_P_  ev_io *watcher, int revents);
static void read_audio_cb(EV_P_ ev_io *watcher, int revents);
static void timer_cb (EV_P_ ev_timer *w, int revents);
//FILE * fp = NULL;
//////////////////////////////// ////////////////
//////////////////////////////// ////////////////
//methods
//////////////////////////////// ////////////////
//////////////////////////////// ////////////////
static void (*on_audio_cb)(char *, int );
//#on_audio_cb = NULL;
static void accept_cb(EV_P_  ev_io *watcher, int revents){
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_sd;
	struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));
	if(EV_ERROR & revents){
		perror("got invalid event");
		return;
	}
	client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_sd < 0){
		perror("accept error");
		return;
	}
	total_clients ++; // Increment total_clients count
	// Initialize and start watcher to read client requests
	printf("Got new client, total_clients:%d\n", total_clients);
	ev_io_init(w_client, read_audio_cb, client_sd, EV_READ);
	ev_io_start(EV_A_  w_client);
}







static char buffer[BUFSIZ* 1024];
static ssize_t bytes;
static void read_audio_cb(EV_P_ ev_io *watcher, int revents){
	if(EV_ERROR & revents){
		perror("got invalid event");
		return;
	}
	// Rece//ive message from client socket
	//bytes = recv(watcher->fd, &buffer[buffer_len], sizeof(buffer) - buffer_len, 0);
	//printf("recv bytes\n");
	bytes = recv(watcher->fd,buffer, sizeof(buffer), 0);
	//printf("recv bytes:%u\n",bytes);
	//int audio_start =     ntohl();
	//bytes = bytes(watcher->fd, buffer, BUFFER_SIZE, 0);
	if(bytes < 0){
		perror("read error");
		ev_io_stop(EV_A_ watcher);
		free(watcher);
		perror("peer might closing");
		close(watcher->fd);
		return;
	}
	if(bytes == 0){
		ev_io_stop(EV_A_ watcher);
		free(watcher);
		perror("peer might closing");
		//on_audio_cb(NULL, 0);
		close(watcher->fd);
		return;
	}
	else{
		//              fwrite(buffer, bytes, 1, fp);
		on_audio_cb(buffer, bytes);
	}
}
void * setup_tcp_server(void *arg){
	////static void (*on_audio_cb)(char *, int );


	on_audio_cb = (void (*)(char *, int))arg;
	loop    = ev_loop_new(EVFLAG_AUTO);
	int sd;
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);
	// Create server socket
	if( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("socket error");
		return  NULL;
	}
	int enable = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		perror("setsockopt(SO_REUSEADDR) failed");
	}
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NO);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0){
		perror("bind error");
		exit(1);
	}
	if (listen(sd, 2) < 0){
		perror("listen error");
		return  NULL;
	}
	ev_io_init(&accept_io, accept_cb, sd, EV_READ);
	ev_io_start(EV_A_  &accept_io);
	printf("%s ready!\n\n", __FUNCTION__);
	ev_loop(EV_A_  0);
	return  NULL;
}
