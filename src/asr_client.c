#include "defs.h"
#define  PORT_NO 8001
//10 ms
/////////////////////////////////////////
//Audio buffer  from main.c
/////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

static void (*on_asr_result)(const char *, unsigned int );

static pack_t req, res;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


static int asr_client = -1, wait_for_rsp=0, idx=0;
static struct ev_loop * loop = NULL;
static ev_io  asr_client_r, asr_client_w;
static ev_timer asr_server_timeout;
static void asr_server_timeout_cb (EV_P_ ev_timer *w, int revents);
static void asr_client_write_cb(EV_P_ ev_io *watcher, int revents);
static void asr_client_read_cb(EV_P_ ev_io *watcher, int revents);

static char  batch[BATCH_SIZE_IN_SAMPLE * 2];
static int batch_len = 0;

int _send_pack(int cmd, int idx, void * audio_data, unsigned int data_len);
int send_data(int cmd, int flag,  void * audio_data, unsigned int data_len);
#define REQ_BUFFER_SIZE 10
static pack_t req_buffer[REQ_BUFFER_SIZE];
static int   req_num=0;


extern int ws_send_msg(const char * msg, unsigned int len);
extern int get_idx(){
	return idx;
}
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
static int setnonblock(int fd){
	int flags;
	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags);
}
char *nshead_to_str(const nshead_t *head, char *buf, size_t len){
	if (buf == NULL || head == NULL || len < sizeof(head))
		return NULL;
	snprintf(buf, len, "id:%u version:%u log_id:%u provider:%s, magic_num:%u reserved:%u body_len:%u",
			head->id, head->version, head->log_id, head->provider, head->magic_num, head->reserved, head->body_len);
	buf[len - 1] = '\0';
	return buf;
}

char *mcpack_to_str(mc_pack_t *pack, char *buf, size_t len){
	if (pack == NULL || buf == NULL || len < 1)
		return NULL;
	int ret = mc_pack_pack2text(pack, buf, len, 0);
	if (ret != 0)
		buf[0] = '\0';
	return buf;
}
char *req_to_str(pack_t *req,  char *buf, size_t len){
	char str[BUFSIZ];
	mc_pack_t *pack = mc_pack_open_r(req->detail, req->head.body_len, str, sizeof(str));
	return mcpack_to_str(pack, buf, len);
}
FILE * fp = NULL;

int fp_idx=0;


int _send_pack(int asr_client,int cmd, int idx, void * audio_data, unsigned int data_len){
	char filename[BUFSIZ];
	/*
	   if(data_len > 0 ){
	   if(idx == 1){
	   snprintf(filename, sizeof(filename), "audio_%02d.pcm", fp_idx++);
	   fp = fopen(filename, "w");
	   fwrite(audio_data, data_len,  1, fp);
	   }
	   }
	   if(CMD_FINISH == cmd){
	   fclose(fp);
	   }
	 */
	if(cmd == CMD_DATA){
		if(data_len <= 0 || !audio_data || idx == -1){
				printf("CMD_START , illegal data\n");
			return -1;
		}
	}
	char tmp[1024 * 8], str[8*1024];
	bzero(&req, sizeof(pack_t));
	int ret=0;
	req.head.id= 1;
	req.head.version= 5192;
	//req->head.log_id = sn;
	strcpy(req.head.provider, "audio_ts");
	req.head.magic_num = NSHEAD_MAGICNUM;
	mc_pack_t *req_mc = mc_pack_open_w(1, req.detail,sizeof(req.detail), tmp , sizeof(tmp));
	if (MC_PACK_PTR_ERR(req_mc)){
		fprintf(stderr, "%d, mc_pack_create failed\n", __LINE__);
		return -1;
	}
	if(cmd != CMD_START){
		ret=mc_pack_put_int32(req_mc,"idx", idx);
	}
	ret=mc_pack_put_int32(req_mc,"cmd", cmd);
	ret=mc_pack_put_int32(req_mc,"content_len", data_len);
	//ret=mc_pack_put_int32(req_mc,"muti_return", 1);
	ret=mc_pack_put_int32(req_mc,"sample_rate", 16);
	ret=mc_pack_put_int32(req_mc,"product_id", 7006);
	//ret=mc_pack_put_str(req_mc,"sn", filename);
	mc_pack_close(req_mc);
	int mc_len = mc_pack_get_size(req_mc);
	//fprintf(stderr, "%d, req data ,mc_len:%d,cmd:%d, data_len:%u, idx=%d , %s\n", __LINE__, mc_len,cmd, data_len,idx, __FUNCTION__);
	//      fprintf(stderr, "mc size:%d, head :%lu \n", mc_len, sizeof(nshead_t));
	req.head.body_len = mc_len;
	char * ptr=(char *) &req;
	if(cmd == CMD_DATA){
		req.head.body_len = mc_len + data_len;
		ptr = ptr +  mc_len + sizeof(nshead_t);
		//sizeof(nshead_t) + mc_len + data_len  <=  sizeof(pack_t)
		if(sizeof(pack_t) <  sizeof(nshead_t)  + mc_len + data_len){
			fprintf(stderr, "%d, audio data too large,mc_len:%d, data_len:%d! \n", __LINE__, mc_len, data_len);
			return -1;
		}
		memcpy(ptr, audio_data, data_len);
	}
	pthread_mutex_lock(&lock);
	//req_buffer[req_num++] = req;
	memmove(&req_buffer[req_num], &req, req.head.body_len);
	//req_buffer[req_num++] = req;
	//printf("add req to buffer, buffer len:%d, mcpack:%s\n", req_num,mcpack_to_str(req_mc , tmp, sizeof(tmp)));
	//printf("add req to buffer, buffer len:%d, mcpack:%s\n", req_num,req_to_str(&req, tmp, sizeof(tmp)));

	printf("add req to buffer, buffer len:%d, mcpack:%s\n", req_num,req_to_str(&req_buffer[req_num], tmp, sizeof(tmp)));
	printf("nshead content:%s\n", nshead_to_str(&(req_buffer[req_num].head) , tmp, sizeof(tmp)));
	req_num++;
	int i=0;
	for(i=0; i < req_num; i++){

		printf("%d,  in send_pack, buffer len:%d, mcpack:%s\n", i,req_num,req_to_str(&req_buffer[i], str, sizeof(str)));
		printf("%d,  in send_pack,nshead content:%s\n",i, nshead_to_str(&(req_buffer[i].head) , str, sizeof(str)));
	}

	printf("req_num:%d\n", req_num);
	if(req_num == REQ_BUFFER_SIZE){
		printf("req_num:%d, wait for cond:\n", req_num);
		exit(1);
		pthread_cond_wait(&cond, &lock);
	}
	pthread_mutex_unlock(&lock);
	return 0;


}

int send_data(int cmd, int flag,  void * audio_data, unsigned int data_len){
	int ret=0;
	if(cmd == CMD_FINISH){
		ret = _send_pack(asr_client, CMD_FINISH, DEFAULT_FINISH_PACKIDX, NULL ,0);
		idx =0;
	}else if(cmd == CMD_START){
		idx =0;
		ret = _send_pack(asr_client, CMD_START, idx++, NULL ,0);
	}else if(cmd == CMD_DATA){
		//batch overflow ?  batch size ?  data_len ?
		memmove(&batch[batch_len], audio_data, data_len);
		batch_len+=data_len;
		//printf("CMD_DATA: batch_len:%d\n",batch_len);
		if(flag == -1){
			ret = _send_pack(asr_client, CMD_DATA,  -1 * idx, batch,  batch_len);
			batch_len = 0;
		}else if(batch_len == sizeof(batch)){
			//printf("cmd:%d, batch len:%d, data_len:%d, batch:%d\n",cmd, batch_len, data_len, sizeof(batch));
			ret = _send_pack(asr_client, CMD_DATA,  idx++, batch, sizeof(batch));
			//send_batches++;
			batch_len =0;
		}
	}
}
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
static int  connnet_to_asr(EV_P){
	//loop    = ev_loop_new(EVFLAG_AUTO);
	struct sockaddr_in server;
	char *message , server_reply[2000];
	char ip[32] = "127.0.0.1";
	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons( PORT_NO );
	//Create socket
	//pthread_mutex_lock(&lock);

	//Create socket
	//pthread_mutex_lock(&lock);
	if(asr_client != -1){
		close(asr_client);
		asr_client = -1;
	}
	asr_client = socket(AF_INET , SOCK_STREAM , 0);
	if (asr_client == -1){
		perror("Socket:");
		exit(EXIT_FAILURE);
	}
	if (connect(asr_client , (struct sockaddr *)&server , sizeof(server)) < 0){
		puts("connect to asr server failed");
		return -1;
	}
	wait_for_rsp = 0;
	if(-1 == setnonblock(asr_client)) {
		perror("echo client socket nonblock");
		exit(EXIT_FAILURE);
	}
	ev_io_init(&asr_client_w, asr_client_write_cb, asr_client, EV_WRITE);
	ev_io_start(loop,  &asr_client_w);
	ev_io_init(&asr_client_r, asr_client_read_cb, asr_client, EV_READ);
	ev_io_start(loop,  &asr_client_r);

	printf("Connected to asr server, %s:%d\n", ip, PORT_NO);
	return 0;
}

static void asr_client_write_cb(EV_P_ ev_io *watcher, int revents){
	if(wait_for_rsp){
		return;
	}
	//printf("wait_for_rsp :%d\n", wait_for_rsp);
	pack_t  tmp_req;
	char str[8*1024];
	pthread_mutex_lock(&lock);
	if(!req_num ){
		pthread_mutex_unlock(&lock);
		return;
	}
	printf("in %s, wait_for_rsp:%d, req_num:%d\n", __FUNCTION__, wait_for_rsp, req_num);

	int i=0, ret=0, net_fail=0;
	for(i=0; i < req_num; i++){

		printf(" %d, list buffer, buffer len:%d, mcpack:%s\n", i, req_num,req_to_str(&req_buffer[i], str, sizeof(str)));
		printf("%d,  list buffer,nshead content:%s\n",i, nshead_to_str(&(req_buffer[i].head) , str, sizeof(str)));
	}
	i=0;

	while(i < req_num){
		//tmp_req = req_buffer[i];
		printf("req content:%s\n", req_to_str(&req_buffer[i] , str, sizeof(str)));
		printf("nshead content:%s\n", nshead_to_str(&(req_buffer[i].head) , str, sizeof(str)));
		memmove(&tmp_req, &req_buffer[i],sizeof(pack_t));
		//	memmove(&req_buffer[req_num], &req, req.head.body_len);
		mc_pack_t *pack = mc_pack_open_r(tmp_req.detail, tmp_req.head.body_len, str, sizeof(str));
		printf("\n try to send mcpack:%s\n", mcpack_to_str(pack , str, sizeof(str)));
		int cmd,idx=0;
		ret =mc_pack_get_int32(pack,"cmd", &cmd);
		if(ret != 0){
			//printf("fail to get cmd in mcpack!\n");
			printf("fail to get cmd , skip pack:%s!\n", req_to_str(&req_buffer[i], str, sizeof(str)));
			i++;
			continue;
		}else{
			if(cmd == CMD_DATA){

				ret =mc_pack_get_int32(pack,"idx", &idx);
				if(ret != 0){
					printf("fail to get idx in mcpack, skip pack:%s!\n", req_to_str(&req_buffer[i], str, sizeof(str)));
					i++;continue;
				}else   if(idx == -1){
					printf("idx == -1 , skip pack:%s!\n", req_to_str(&req_buffer[i], str, sizeof(str)));
					i++;continue;
				}
			}
		}
		ret = nshead_write (asr_client, &(tmp_req.head), 1000);
		if (ret != 0){
			printf("%d, nshead_write failed, sock:%d, erro str:%s\n", __LINE__, asr_client, nshead_error_c(ret));
			req_num=0;
			ev_io_stop(loop,  &asr_client_w);
			ev_io_stop(loop,  &asr_client_r);
			ev_break(loop, EVBREAK_ALL);
			close(asr_client);
			asr_client = -1;
			//exit(1);
			//net_fail = 1;
			break;
		}else{
			printf("\nnshead_write OK\n");
			//net_fail = 0;
			i++;
			if(cmd  == CMD_FINISH){
				printf("CMD_FINISH sent, setup timeout for asr server resp\n\n");
				ev_timer_stop(loop,  &asr_server_timeout);
				ev_timer_init(  &asr_server_timeout, asr_server_timeout_cb,  3, 0);
				ev_timer_start(loop,  &asr_server_timeout);
				wait_for_rsp = 1;
				break;
			}else if(cmd== CMD_START){
				wait_for_rsp = 1;
				break;
			}
		}
	}
	if(i>0 ){
		printf("move %d packs forward\n", i);
		memmove(req_buffer, &req_buffer[i], i * sizeof(pack_t));
		req_num -= i;
	}
	//      dump_reqs();
	//printf("signal cnod\n");
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);
	if(asr_client == -1){
		//sleep(1);
		connnet_to_asr(loop);
		printf("asr_client, sock:%d\n", asr_client);
	}
}
//////////////////////////////////

static void asr_client_read_cb(EV_P_ ev_io *watcher, int revents){
	printf("asr readable\n");
	char tmp[BUFSIZ], str[BUFSIZ];
	int ret=0;
	if(EV_ERROR & revents){
		perror("got invalid event");
		return;
	}
	bzero(&res, sizeof(res));
	ret = nshead_read(watcher->fd, &(res.head), sizeof(res), 1000);
	if (ret != 0){
		printf("%d, nshead_read failed:%s\n", __LINE__, nshead_error(ret));
		ev_io_stop(loop,  &asr_client_w);
		ev_io_stop(loop,  &asr_client_r);
		ev_break(loop, EVBREAK_ALL);
		close(asr_client);
		asr_client = -1;
		//exit(1);
		return ;
	}
	mc_pack_t *pack = mc_pack_open_r(res.detail, res.head.body_len, tmp, sizeof(tmp));
	if (MC_PACK_PTR_ERR(pack) != 0){
		fprintf(stderr, "%d,invalid mcpack ", __LINE__);
	}else{
		const char * pack_str = mcpack_to_str(pack , str, sizeof(str));
		printf("got data from asr server:%s\n", pack_str);
		int err_no=9999, idx=-1;
		mc_pack_get_int32(pack, "idx", &idx);
		mc_pack_get_int32(pack, "err_no", &err_no);
		printf("idx=%d,  err_no=%d, \n",idx,err_no);
		if(idx == -9999){
			if(err_no == 0){

				wait_for_rsp = 0;
				mc_pack_t *arr =  mc_pack_get_array(pack, "result");
				if(!MC_PACK_PTR_ERR(arr)){
					mc_pack_t *obj = mc_pack_get_object_arr(arr, 0);
					if(!MC_PACK_PTR_ERR(obj)){
						const char *res_str = mc_pack_get_str(obj , "word_class");
						if(!MC_PACK_PTR_ERR(res_str)){
							printf("res:%s\n", res_str);
							ws_send_msg(res_str, strlen(res_str)+1);
						}
					}
				}
			}else if(err_no == -7){
			}
				ev_io_stop(loop,  &asr_client_w);
				ev_io_stop(loop,  &asr_client_r);
				ev_timer_stop(loop, &asr_server_timeout);
				connnet_to_asr(loop);
			//on_asr_result(clean_buffer, strlen(clean_buffer)+1);
			//wait_for_rsp =0;
		}else if(idx == 0){//response of START
			wait_for_rsp =0;
		}
	}
}

///////////////////////////////////////




/*
   static void asr_client_read_cb(EV_P_ ev_io *watcher, int revents){
   char tmp[BUFSIZ], str[BUFSIZ];
   int ret=0;
   if(EV_ERROR & revents){
   perror("got invalid event");
   return;
   }
   bzero(&res, sizeof(res));
   ret = nshead_read(watcher->fd, &(res.head), sizeof(res), 1000);

   if (ret != 0){
   printf("%d, nshead_read failed:%s\n", __LINE__, nshead_error(ret));
   ev_io_stop(loop,  &asr_client_w);
   ev_io_stop(loop,  &asr_client_r);
   ev_break(loop, EVBREAK_ALL);
   close(asr_client);
   asr_client = -1;
//exit(1);
return ;
}
mc_pack_t *pack = mc_pack_open_r(res.detail, res.head.body_len, tmp, sizeof(tmp));
if (MC_PACK_PTR_ERR(pack) != 0){
fprintf(stderr, "%d,invalid mcpack ", __LINE__);
}else{
const char * pack_str = mcpack_to_str(pack , str, sizeof(str));
printf("got data from asr server:%s\n", pack_str);
int err_no=9999, idx=-1;
mc_pack_get_int32(pack, "idx", &idx);
mc_pack_get_int32(pack, "err_no", &err_no);
printf("idx=%d,  err_no=%d, \n",idx,err_no);
if(idx == -9999){
mc_pack_t *arr =  mc_pack_get_array(pack, "result");
if(!MC_PACK_PTR_ERR(arr)){
mc_pack_t *obj = mc_pack_get_object_arr(arr, 0);
if(!MC_PACK_PTR_ERR(obj)){
const char *res_str = mc_pack_get_str(obj , "word_class");
if(!MC_PACK_PTR_ERR(res_str)){
printf("res:%s\n", res_str);
ws_send_msg(res_str, strlen(res_str)+1);
}
}
}
//on_asr_result(clean_buffer, strlen(clean_buffer)+1);
ev_timer_stop(loop, &asr_server_timeout);
connnet_to_asr(loop);
wait_for_rsp =0;
}else if(idx == 0){//response of START
wait_for_rsp =0;
}
}

}
 */

static void asr_server_timeout_cb (EV_P_ ev_timer *w, int revents){
	printf("no asr res from server, reconnect!!!!!\n\n\n");
	ev_timer_stop(loop, &asr_server_timeout);
	connnet_to_asr(loop);
}
void * setup_asr_client(void *arg ){
	asr_params_t *  ptr = (asr_params_t *) arg;
	on_asr_result = ( void (*)(const char *, unsigned int))ptr->cb;
	//lock = ptr->lock;
	loop    = ev_loop_new(EVFLAG_AUTO);
	int ret = connnet_to_asr(loop);
	if(ret < 0){
		return NULL;
	}
	ev_run(loop,0);
	printf("%s ready!\n\n", __FUNCTION__);
	return NULL;
}

