#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char* get_pipe_name(int side, int n) {
	static char name_buf[16];

	if (side != MSG_PRODUCER && side != MSG_CONSUMER) {
		return MSG_BAD_SIDE;
	}

	const char prefix = side == PRODUCER ? 'p' : 'c';

	sprintf(name_buf, "%c_%d.fifo", prefix, n);

	return name_buf;
}

// Creates all FIFO files. Should only be called once per group of processes. returns MSG_ERROR_CODE. 
int init_messaging(int num_producers, int num_consumers) {
	for (int i = 0; i < num_producers; i++) {
		char* fifo_name = get_pipe_name(MSG_PRODUCER, i);
		mkfifo(fifo_name, 0660);
	}

	for (int i = 0; i < num_consumers; i++) {
		char* fifo_name = get_pipe_name(MSG_CONSUMER, i);
		mkfifo(fifo_name, 0660);
	}

	return MSG_OKAY;
}

// Call once per process with own side and own number. returns MSG_ERROR_CODE
int open_msg(int num_producers, int num_consumers, int side, int n, msg_t* msg) {
	msg->num_consumers = num_consumers;
	msg->num_producers = num_producers;
	msg->side = side;
	msg->n = n;
	msg->fifo_fd = open(get_pipe_name(side, n), O_RDONLY);
	msg->num_producers_alive = num_producers;
	msg->alive_producers = NULL;

	if (side == MSG_CONSUMER) {
		// keep list of alive producer fifo fds
		msg->alive_producers = malloc(sizeof(int) * num_producers);
		for (int i = 0; i < num_producers; i++) {
			msg->alive_producers[i] = open(get_pipe_name(MSG_PRODUCER, i);, O_RDONLY);
		}
	}

	return MSG_OKAY;
}


int close_msg(msg_t* msg) {
	close(msg->fifo_fd);

	if (msg->side == MSG_CONSUMER) {
		for (int i = 0; i < msg->num_producers_alive; i++) {
			close(msg->alive_producers[i]);
		}
		free(msg->alive_producers);
	}

	return MSG_OKAY;
}

int send_packet(msg_t* msg, int dst_fd, int type, const void* buf, int buflen) {
	char msg_buf[PIPE_BUF];
	int header_buf[4];

	if (buflen > MSG_MAX_SIZE) {
		return MSG_TOO_LONG;
	}
	
	header[0] = msg->side;
	header[1] = msg->n;
	header[2] = type;
	header[3] = buflen;

	memcpy(msg_buf, header_buf, MSG_HEADER_SIZE);
	memcpy(msg_buf[MSG_HEADER_SIZE], buf, buflen);

	const total_msg_len = buflen + MSG_HEADER_SIZE;
	if (write(dst_fd, msg_buf, total_msg_len) < 0) {
		return MSG_WRITE_ERR;
	} else {
		return MSG_OKAY;
	}
}

int recv_packet(msg_t* msg, int* oside, int* on, int* type, int* buflen, void* buf) {
	char msg_buf[PIPE_BUF];

	if (read(msg->fifo_fd, msg_buf, sizeof(msg_buf)) < 0) {
		return MSG_READ_ERR;
	}

	memcpy(oside, msg_buf, sizeof(int));
	memcpy(on, msg_buf + sizeof(int), sizeof(int));
	memcpy(type, msg_buf + 2 * sizeof(int), sizeof(int));
	memcpy(buflen, msg_buf + 3* sizeof(int), sizeof(int));
	memcpy(buf, msg_buf + 4 * sizeof(int), *buflen);

	return MSG_OKAY;
}

// used by producers, waits for job request then serves job
int serve_job(msg_t* msg, const void* job_buf, int job_len) {
	static char msg_buf[PIPE_BUF];

	if (buflen > MSG_MAX_SIZE) {
		return MSG_TOO_LONG;
	}

	// wait for consumer request
	int origin_side, origin_n, msg_type, msg_len;
	recv_packet(msg, *origin_side, *origin_n, *msg_type, *msg_len, msg_buf);

	printf("Got request: s%d #%d t%d l%d", origin_side, origin_n, msg_type, msg_len);

	// respond to corresponding origin with job reponse
	send_packet(msg, )
}

// used by consumers, polls random producer for job
int request_job(msg_t* msg, void* buf, int* buflen);