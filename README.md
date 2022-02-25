# N Producer / M Consumers Demo

By: Jeffrey Ryan

Date: 24 Feb 2022

## Description
This project demonstrates a solution to the multiple producers and multiple consumers problem. In this code, producers read secure
random bytes from /dev/urandom into an odd 64-bit unsigned integer. The producers will then pass this data to the consumers, who will 
filter out all composite integers using the Miller-Rabin primality test, printing only the prime numbers.

## Implementation
The parent process first creates a POSIX message queue (by default named `/ryanj_p1mq`), then forks to create NP producer processes,
and NC consumer processes. The producers push data, in the form of `struct job_t`, onto the queue using `mq_send()`, exiting when they are
finished. The consumers pull `job_t` messages using `mq_timedreceive()` and process that data. The parent process waits on each indivdual
producer process, and when they have exited, sends all of its children processes (at this point only consumers) a `SIGUSR1` signal.
The consumers utilize a signal handler to set `producers_are_done` flag to 1, which in conjuction with timing out on `mq_timedreceive()`,
lets the consumer processes know when to finish.

## Compilation
   
    make

## Usage

    ryanj_proj1.exe [-p #producers] [-c #consumers] [-j #jobs] [-n queue_name] [-r] [-h]
        -p #producers : set number of producers (defaults to 3)
        -c #consumers : set number of consumers (defaults to 3)
        -j #jobs : set number of jobs per producer (defaults to 10)
        -n queue_name : set name of message queue (defaults to /ryanj_p1mq
        -r : remove queue from system, then exit
        -h : show this message, then exit

## Files

* is_prime.c & is_prime.h: Homemade Miller-Rabin primality test API for uint64_t
* ryanj_proj1.c: Everything else. Consumer, Producer, and Forker process logic
