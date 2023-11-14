/*
 * tacmsgrouter - simple pipe read and write to demonstrate sockets
 * 
 * Copyright (C) 2022  Resilience Theatre
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h> 
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "log.h"
#include "ini.h"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#define MAX 1024
#define PORT 8080
#define SA struct sockaddr
#define BUF_SIZE 1024

char *incoming_pipe="";
char *outgoing_pipe="";
char *tacmsg_remote_addr="";

void write_pipe(char *string);
void write_socket(char *string);

/* This will read pipe and write_socket. 
 * Write loop to local pipe with write_pipe() 
 */
void *mainthread(void *ptr)
{
	char outbuf[BUF_SIZE];
	log_debug("[%d] mainthread ()",getpid());
	FILE * fp;
	size_t len = 1024;
	char *line = malloc(len);
	ssize_t read;
	
	while ( 1 ) 
	{
		fp = fopen(incoming_pipe, "r");
		if (fp == NULL)
			exit(EXIT_FAILURE);
			
			log_info("[%d] pipe reading started",getpid());
			
			while ((read = getline(&line, &len, fp)) != -1) {
				line[strcspn(line, "\n")] = 0;
				log_debug("[%d] Got message: %s [%d] [%d]",getpid(), line, strlen(line),len);		
				memset(outbuf,0,BUF_SIZE);
				strcpy(outbuf, line);	// change this
				log_debug("[%d] Writing to pipe: %s [%d]",getpid(), outbuf, strlen(outbuf));
				write_pipe(outbuf);		// write back to own UI
				
                // This is disabled when we're running all alone
                if(0) 
                {
                    log_debug("[%d] Writing to TCP socket: %s [%d]",getpid(), outbuf, strlen(outbuf));
					int sockfd;
					struct sockaddr_in servaddr;
					log_debug("[%d] SENDING msg to socket: %s [%d]",getpid(),outbuf,sizeof(outbuf));

					sockfd = socket(AF_INET, SOCK_STREAM, 0);
					if (sockfd == -1) {
						log_error("[%d] Write socket create failed",getpid());  
						exit(0);
					}
					else {
						log_debug("[%d] Write socket created",getpid());  
					}
					bzero(&servaddr, sizeof(servaddr));
					servaddr.sin_family = AF_INET;
					servaddr.sin_addr.s_addr = inet_addr(tacmsg_remote_addr);
					servaddr.sin_port = htons(PORT);  
					if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
						log_error("[%d] Server connection failed",getpid());
						exit(0);
					}
					else {
						log_debug("[%d] Write socket connected to server",getpid());   
					}
					write(sockfd, outbuf, sizeof(outbuf));
					close(sockfd);
                }
			}
		fclose(fp);
	}
	log_info("[%d] exiting ",getpid());
	if (line)
		free(line);
	exit(EXIT_SUCCESS);
	return ptr;
}

/* This will create listening TCP socket for messages coming
 * in from anohter tacmsgrouter. These should be forwarded to 
 * local pipe (for UI).
 */
void *serverthread(void *ptr)
{
    int sockfd, connfd;
    unsigned int socket_len;
    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_error("[%d] Socket create failed",getpid()); 
        exit(0);
    }
    else {
        log_debug("[%d] Socket successfully created",getpid());
	}
    bzero(&servaddr, sizeof(servaddr));
  
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        log_error("[%d] Socket bind failed",getpid());
        exit(0);
    }
    else
    {
        log_debug("[%d] Socket successfully binded",getpid());
	}
    if ((listen(sockfd, 5)) != 0) {
         log_error("[%d] Server listen failed",getpid());
        exit(0);
    }
    else
    {
        log_info("[%d] TCP server listening port %d",getpid(),PORT);  
    }
    socket_len = sizeof(cli);
   
	while ( 1 )
	{
		connfd = accept(sockfd, (SA*)&cli, &socket_len);
		if (connfd < 0) {
			log_error("[%d] server accept failed",getpid());
			exit(0);
		}
		else
		{
			log_debug("[%d] server accept the client",getpid());
		}
		char buff[MAX];
		bzero(buff, MAX);   
		log_debug("[%d] serverthread () wait socket read",getpid());
		// read the message from client and copy it in buffer
		read(connfd, buff, sizeof(buff));
		log_debug("[%d] Socket receive: %s",getpid(),buff);
		
		write_pipe(buff);	
		
		bzero(buff, MAX);
	}
    close(sockfd);    
    return  ptr;
}


void write_socket(char *string) 
{
    int sockfd;
    struct sockaddr_in servaddr;
    log_info("[%d] SENDING msg to socket: %s [%d]",getpid(),string,sizeof(string));    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_error("[%d] Write socket create failed",getpid());  
        exit(0);
    }
    else {
        log_debug("[%d] Write socket created",getpid());  
	}
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(tacmsg_remote_addr);
    servaddr.sin_port = htons(PORT);  
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        log_error("[%d] Server connection failed",getpid());
        exit(0);
    }
    else {
        log_debug("[%d] Write socket connected to server",getpid());   
	}
	write(sockfd, string, sizeof(string));
    close(sockfd);
}

void write_pipe(char *string) {
	FILE *fptr;
	log_debug("[%d] Writing to pipe: %s payload: %s [%d] ",getpid(), outgoing_pipe, string,strlen(string));
	fptr = fopen(outgoing_pipe,"w");
	if(fptr == NULL)
	{
		log_error("[%d] cannot open pipe file for writing",getpid());
		exit(1);             
	}
	fprintf(fptr,"%s \n",string);
	fclose(fptr);
}

int main(int argc, char *argv[])
{
	char *ini_file=NULL;
	int c=0;
	int log_level=LOG_INFO;
	while ((c = getopt (argc, argv, "dhi:")) != -1)
	switch (c)
	{
		case 'd':
			log_level=LOG_DEBUG;
			break;
		case 'i':
			ini_file = optarg;
			break;
		case 'h':
			log_info("[%d] tacmsgrouter - read & write gwsocket fifo pipes ",getpid());
			log_info("[%d] Usage: -i [ini_file] ",getpid());
			log_info("[%d]        -d debug log ",getpid());
			log_info("[%d] ",getpid());
			return 1;
		break;
			default:
			break;
	}
	if (ini_file == NULL) 
	{
		log_error("[%d] ini file not specified, exiting. ", getpid());
		return 0;
	}
	/* Set log level: LOG_INFO, LOG_DEBUG */
	log_set_level(log_level);
	
	/* read ini-file */
	ini_t *config = ini_load(ini_file);
	ini_sget(config, "tacmsgrouter", "outgoing_pipe", NULL, &outgoing_pipe); 
	ini_sget(config, "tacmsgrouter", "incoming_pipe", NULL, &incoming_pipe); 
	ini_sget(config, "tacmsgrouter", "remote_server", NULL, &tacmsg_remote_addr); 
	log_info("[%d] Reading messages from: %s ",getpid(),incoming_pipe); 
	log_info("[%d] Writing messages to: %s ",getpid(),outgoing_pipe); 
	log_info("[%d] Forward locally received messages to remote server: %s ",getpid(),tacmsg_remote_addr); 

	// TCP socket server
	pthread_t thread1;
    int thr = (intptr_t)1;
	pthread_create(&thread1, NULL, *serverthread, (void *)(intptr_t) thr); 

	// Pipe read thread
	pthread_t thread2;
    int thr2 = (intptr_t)2;
    pthread_create(&thread2, NULL, *mainthread, (void *)(intptr_t) thr2);
    
    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);

    return 0;
}
