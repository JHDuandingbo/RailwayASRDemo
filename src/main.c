///////////////////////////////////
#include <defs.h>
extern void* setup_tcp_server(void *);
extern void* setup_ws_server(void*);
extern void *setup_asr_client(void * arg );
extern int ws_send_msg(const char * msg, unsigned int len);
//extern int _send_pack(int cmd, int idx, void * audio_data, unsigned int data_len, pack_t  * req);
extern int send_data(int cmd, int flag,  void * audio_data, unsigned int data_len);
extern int get_idx();
extern int get_asr_client();
//////////////////////////////////
//input buffer for audio stream
static char audio_buffer[VOICE_MAX_IN_SAMPLE * 2];
static int audio_buffer_len=0;
//static pthread_mutex_t audio_lock = PTHREAD_MUTEX_INITIALIZER;
//pack_t req, res;
static int  sn=0;
//static int  idx=0;
static int num_need_forward=0;
static Fvad *fvad = NULL;
static int prev_res = 0;
static int started = 0;
static int current_res = 0;
static int silence_num=0;
static int current_frame=0;
//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//extern int setup_tcp_server(void (*func)(char *, int));
void  on_audio_cb(char *data, int bytes){
	if( data == NULL ||  bytes <=  0 ){
		return;
	}
	int ret = 0;
	int16_t frame[FRAME_SIZE_IN_SAMPLE];
	if(bytes + audio_buffer_len <= sizeof(audio_buffer)){
		memcpy(&audio_buffer[audio_buffer_len], data, bytes);
		audio_buffer_len += bytes;
	}else{
		num_need_forward = current_frame;
		if(current_res == 0){
			//leave silence for next voice
			num_need_forward -= (silence_num < SILENCE_THR ?  silence_num: SILENCE_THR);
		}
		if(num_need_forward >  0 ){
			printf("num_need_forward:%d, move mem,, audio_buffer_len:%d\n\n", num_need_forward, audio_buffer_len);
			memmove(audio_buffer, &audio_buffer[sizeof(frame)*num_need_forward], audio_buffer_len - sizeof(frame)*num_need_forward);
			audio_buffer_len -= num_need_forward * sizeof(frame);
			current_frame-= num_need_forward;
			num_need_forward = 0;
			//exit(1);
			if(bytes + audio_buffer_len <= sizeof(audio_buffer)){
				memcpy(&audio_buffer[audio_buffer_len], data, bytes);
				audio_buffer_len += bytes;
			}else{
				printf("audio buffer overflow, dump excess data!!!\n");
				return;
			}
		}
	}
	while(1){
		if(audio_buffer_len < (1+current_frame) *sizeof(frame)){
			break;
		}
		memmove(frame, &audio_buffer[current_frame * sizeof(frame)], sizeof(frame));
		int current_res = fvad_process(fvad, frame, FRAME_SIZE_IN_SAMPLE);
		if(prev_res == 0 && current_res == 0){
			silence_num++;
			prev_res = 0;
			//if(get_idx() > 0 ){
			if(started){
				//send silence after voice
				if(silence_num >=  SILENCE_THR){
					//send -1 * idx , send FINISH
					ret = send_data(CMD_DATA, -1,  frame, sizeof(frame));
					ret = send_data(CMD_FINISH,1,  NULL, 0);
					printf("\n\n send CMD_FINISH, silence_num:%d\n\n", silence_num);
					started = 0;
					//idx = 0; sn++;
				}else{
					//send  silence after voice
					ret = send_data(CMD_DATA, 1, frame, sizeof(frame));
				}
			}

		}else if(prev_res ==0 && current_res == 1){
			//if(get_idx() == 0){
			if(!started){
				//send START
				started=1;
				printf("\n\n send CMD_START, silence_num:%d\n\n", silence_num);
				ret = send_data(CMD_START, 1 , NULL, 0);
			
				int cnt = (silence_num < SILENCE_THR ? silence_num : SILENCE_THR);
				int from = current_frame - cnt;
				assert(from >= 0);
				printf("\ncurrent_frame:%d, send silence from :%d,\n",current_frame, from);
				while(from <= current_frame){
					memmove(frame, &audio_buffer[from * sizeof(frame)], sizeof(frame));
					ret = send_data(CMD_DATA, 1, frame, sizeof(frame));
					//printf("send_data \n",current_frame, from);
					from++;
				}
			}else{
				if(silence_num < SILENCE_THR){
					//send data
					ret = send_data(CMD_DATA, 1, frame, sizeof(frame));
				}
			}
			silence_num = 0;
			prev_res = 1;
		}else if( prev_res == 1 && current_res == 0){
			//send_data
			ret = send_data(CMD_DATA, 1, frame, sizeof(frame));
			silence_num=1;
			prev_res=0;
		}else if(prev_res == 1 && current_res == 1){
			silence_num=0;
			prev_res=1;
			ret = send_data(CMD_DATA, 1, frame, sizeof(frame));
		}
		//printf("\ncurrent:%d, buffer_len:%d, frame totoal:%d, prev_res:%d, current_res:%d,silence_num:%d ,  \n", current_frame,audio_buffer_len,  sizeof(frame) * current_frame, prev_res, current_res, silence_num);
		current_frame++;
	}//

	return;
}


void  on_asr_result_cb(const char *msg, unsigned int bytes){
	int ret = 0;
	if( msg != NULL && bytes > 0 ){
		ret = ws_send_msg(msg, bytes);
	}
	return;
}
int test(){
	setup_ws_server(NULL);
}
int main(){
	fvad = fvad_new();
	if(!fvad){
		printf("create vad failed!\n");
		return -1;
	}
	fvad_set_mode(fvad, 3);
	fvad_set_sample_rate(fvad, SAMPLE_RATE);
	//fp = fopen("./demo.pcm", "w");
	//setup_tcp_server(on_audio_cb);
	//signal(SIGINT, __quit__);
	//signal(SIGTERM, __quit__);
	pthread_t  tcp_pid, ws_pid, asr_pid;
	pthread_create(&tcp_pid, NULL, setup_tcp_server, (void*)on_audio_cb);
	pthread_create(&ws_pid,  NULL, setup_ws_server, NULL);
	asr_params_t  asr_params;
	asr_params.cb = on_asr_result_cb;
	//asr_params.audio_buffer = audio_buffer;
	//asr_params.audio_buffer_len_ptr = &audio_buffer_len;
	//asr_params.audio_lock_ptr = &audio_lock;
	//pthread_create(&asr_pid, NULL, setup_asr_client, (void*)on_asr_result_cb);
	pthread_create(&asr_pid, NULL, setup_asr_client, &asr_params);
	pthread_join(ws_pid, NULL);
	pthread_join(asr_pid, NULL);
	pthread_join(tcp_pid, NULL);
	return 0;
}

///////////////////////////////////
///////////////////////////////////
///////////////////////////////////
///////////////////////////////////
///////////////////////////////////

