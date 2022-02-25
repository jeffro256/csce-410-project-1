#pragma once

#include <limits.h>

enum MSG_ERROR_CODE {
	MSG_OKAY,
	MSG_PRODUCERS_DONE,
	MSG_TOO_LONG,
	MSG_BAD_SIDE,
	MSG_BAD_NUM,
	MSG_READ_ERR,
	MSG_WRITE_ERR
};

enum MSG_SIDE {
	MSG_PRODUCER,
	MSG_CONSUMER
};

enum MSG_TYPES {
	MSG_REQUEST,
	MSG_RESPONSE,
	MSG_END,
	MSG_UNKNOWN
}

typedef struct {
	int num_consumers;
	int num_producers;
	int side;
	int n;
	int fifo_fd;
	int num_producers_alive; // used by consumer only
	int* alive_producers;    // used by consumer only
} msg_t;

// FIFO writes of size <= PIPE_BUF guaranteed atomic
#define MSG_HEADER_SIZE 32
#define MSG_MAX_SIZE (PIPE_BUF - MSG_HEADER_SIZE)

// no need to free, will change on next call to get_pipe_name. Not thread-safe
char* get_pipe_name(int side, int n);

// Creates all FIFO files. Should only be called once per group of processes. returns MSG_ERROR_CODE. 
int init_messaging(int num_producers, int num_consumers);

// Call once per process with own side and own number. retusns MSG_ERROR_CODE
int open_msg(int num_producers, int num_consumers, int side, int n, msg_t* msg);
int close_msg(msg_t* msg);

// used by producers, waits for job request then serves job
int serve_job(msg_t* msg, const void* buf, int buflen);

// used by consumers, polls random producer for job
int request_job(msg_t* msg, void* buf, int* buflen);