#ifndef _RAIL_DEFS_
#define _RAIL_DEFS_
#include <fvad.h>
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
#include <ev.h>
#include <signal.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ul_log.h"
#include "ub.h"
#include "mc_pack.h"
#include "nshead.h"

#define DEFAULT_FINISH_PACKIDX            -9999
#define CMD_NONE            0
#define CMD_START           1
#define CMD_STOP            2
#define CMD_FINISH          3
#define CMD_DATA            4

#define SAMPLE_RATE 16000
#define SAMPLE_SIZE 2
//10 ms
#define FRAME_SIZE_IN_SAMPLE 160
//300 ms, 30 frames
//#define SILENCE_BEFORE_ACTIVE  (FRAME_SIZE_IN_SAMPLE * 30)
//300ms, 30 frames
//#define SILENCE_BEFORE_ACTIVE (FRAME_SIZE_IN_SAMPLE * 30)
// in sample
//##define VOICE_MIN   (10 * 1000/ 10 * FRAME_SIZE_IN_SAMPLE)
// in sample
#define VOICE_MAX_IN_SAMPLE (5 * 1000/10 * FRAME_SIZE_IN_SAMPLE )
//1280 bytes per packet,  4 frames per packet
#define BATCH_SIZE_IN_SAMPLE ( FRAME_SIZE_IN_SAMPLE * 20)
#define SILENCE_THR  100

typedef struct _pack_t{
        nshead_t head;
        char detail[1024*1024];
}pack_t;

typedef struct _asr_params_t{
	void  (*cb)(const char *, unsigned int );
}asr_params_t;


#endif
