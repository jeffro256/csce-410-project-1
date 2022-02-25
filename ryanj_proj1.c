// Needs to be linked with -lrt

#include <errno.h>
#include <mqueue.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "is_prime.h"

typedef struct {
	int producer;     // producer_id
	int type;         // 0 if end signal, 1 if job 
	uint64_t data;    // ignored if type == 0
} job_t;

// Flag to stop consumer loop. Global b/c modified by signal handler. 
volatile sig_atomic_t producers_are_done = 0;

/******************************************************
 *               PRODUCER SECTION
 ******************************************************/

uint64_t get_random_64() {
	int urand_fd = open("/dev/urandom", O_RDONLY);
	uint64_t res;
	read(urand_fd, &res, 8);
	close(urand_fd);
	return res;
}

int producer_loop(int id, int nj, mqd_t mqd) {
	job_t job;
	job.producer = id;
	job.type = 1;      // data job
	for (int i = 0; i < nj; i++) {
		job.data = get_random_64() | 1; // odd random number

		if (mq_send(mqd, (char*) &job, sizeof(job), 0) == -1) {
			perror("Could not send to queue");
			return EXIT_FAILURE;
		}
	}

	//printf("Producer %d is exiting...\n", id);
	return EXIT_SUCCESS;
}

/******************************************************
 *               CONSUMER SECTION
 ******************************************************/

int consumer_loop(int id, mqd_t mqd) {
	// set timeout value
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 100000000;

	// loop until we have both received the producers are done signal
	// and the queue is empty.
	int got_timeout = 0;
	while (!producers_are_done || !got_timeout) {
		job_t job;
		if (mq_timedreceive(mqd, (char*) &job, sizeof(job), NULL, &timeout) == -1) {
			if (errno == ETIMEDOUT) {
				got_timeout = 1;
				continue;
			} else {
				perror("Could not receive from queue");
				return EXIT_FAILURE;
			}
		}

		printf("---job---\n");

		if (is_prime(job.data, 0)) {
			printf("Found prime number: %lu\n", job.data);
		}
	}

	//printf("Consumer %d is exiting...\n", id);
	return EXIT_SUCCESS;
}

/******************************************************
 *                 MOTHER/FORKER SECTION
 ******************************************************/

/*
Manages child processes: waits for producers to finish, sends signal to consumers,
waits for consumers to finish, then cleans up
*/
int mother_main(pid_t* producer_pids, int np, pid_t* consumer_pids, int nc,
		mqd_t mqd, const char* qname) {
	// wait for all producers to exit
	for (int i = 0; i < np; i++) {
		waitpid(producer_pids[i], NULL, 0);
	}

	// send SIGUSR1 to all consumers (lets them know producers are done)
	for (int i = 0; i < nc; i++) {
		kill(consumer_pids[i], SIGUSR1);
	}

	// wait for all consumers to exit
	for (int i = 0; i < nc; i++) {
		waitpid(consumer_pids[i], NULL, 0);
	}

	// free producer and consumer pid lists
	free(producer_pids);
	free(consumer_pids);

	// close message queue descriptor
	printf("All children exited successfully! Closing queue...\n");
	if (mq_close(mqd) == -1) {
		perror("Failed to close queue");
		return EXIT_FAILURE;
	}

	// remove message queue completely
	if (mq_unlink(qname) == -1) {
		perror("Failed to remove queue");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/******************************************************
 *                   MAIN SECTION
 ******************************************************/

void signal_handler(int signo) {
	if (signo == SIGUSR1) {
		producers_are_done = 1;
		//printf("Got producers_are_done signal\n");
	}
}

int main(int argc, char* argv[]) {
	int np = 3;
	int nc = 3;
	int nj = 10;
	int rm_mq = 0;
	char qname[256] = "/ryanj_p1mq";

	// Parse command line args
	int opt;
	while ((opt = getopt(argc, argv, "p:c:j:n:rh")) != -1) {
		switch (opt) {
		case 'p':
			np = atoi(optarg);
			break;
		case 'c':
			nc = atoi(optarg);
			break;
		case 'j':
			nj = atoi(optarg);
			break;
		case 'n':
			if (optarg[0] != '/') {
				fprintf(stderr, "Message queue name must begin with a slash /\n");			
				return EXIT_FAILURE;
			} else if (strlen(optarg) > 255) {
				fprintf(stderr, "Message queue name cannot be longer than 255 bytes\n");			
				return EXIT_FAILURE;
			}
			strcpy(qname, optarg);
		case 'r':
			rm_mq = 1;
			break;
		case 'h':
			printf("Usage: ryanj_proj1.exe [-p #producers] [-c #consumers] [-j #jobs]"
			" [-n queue_name] [-r] [-h]\n"
			"    -p #producers : set number of producers (defaults to 3)\n"
			"    -c #consumers : set number of consumers (defaults to 3)\n"
			"    -j #jobs : set number of jobs per producer (defaults to 10)\n"
			"    -n queue_name : set name of message queue (defaults to /ryanj_p1mq\n"
			"    -r : remove queue from system, then exit\n"
			"    -h : show this message, then exit\n"
			);
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "Bad command-line argument\n");
			return EXIT_FAILURE;
		}
	}

	// if -r is specified, try to unlink message queue
	if (rm_mq) {
		printf("Removing queue...\n");
		if (mq_unlink(qname) == -1) {
			perror("Failed to remove queue");
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	// try opening queue
	const long maxmsg = 64; // bounded size of queue
	struct mq_attr qattr;
	qattr.mq_maxmsg = 10;
	qattr.mq_msgsize = sizeof(job_t);
	const mqd_t mqd = mq_open(qname, O_RDWR | O_CREAT, 0660, &qattr);
	if (mqd == -1) {
		perror("Failed to open message queue");
		return EXIT_FAILURE;
	}

	// Register signal handler
	struct sigaction sigact;
	if (sigaction(SIGUSR1, NULL, &sigact) == -1) {
		perror("Failed to retreive signal handler info");
		return EXIT_FAILURE;
	}
	sigact.sa_handler = signal_handler;
	//sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART; // mq syscalls won't fail with EINTR
	//sigact.sa_restorer = NULL;
	if (sigaction(SIGUSR1, &sigact, NULL) == -1) {
		perror("Failed to register signal handler");
		return EXIT_FAILURE;
	}

	pid_t pid = -1;
	int is_producer;
	int id = -1; // producer/consumer id
	pid_t* producer_pids = malloc(np * sizeof(pid_t));
	pid_t* consumer_pids = malloc(nc * sizeof(pid_t));

	if (producer_pids == NULL || consumer_pids == NULL) {
		fprintf(stderr, "Failed to allocate PID lists\n");
		return EXIT_FAILURE;
	}

	// Fork producers + consumers
	for (int i = 0; i < np && pid; i++) {
		is_producer = 1;
		id = i;
		pid = consumer_pids[i] = fork();
	}
	for (int i = 0; i < nc && pid; i++) {
		is_producer = 0;
		id = i;
		pid = consumer_pids[i] = fork();
	}

	// Branch depending on is consumer/producer/forker
	if (pid) { // if mother process
		return mother_main(producer_pids, np, consumer_pids, nc, mqd, qname);
	}

	// Only the mother needs these lists
	free(producer_pids);
	free(consumer_pids);

	if (is_producer) {
		return producer_loop(id, nj, mqd);
	}

	// else is consumer
	return consumer_loop(id, mqd);
}
