#include <stdio.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>	//write
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdlib.h>

int child_processes = 0;

/*
 One function that prints the system call and the error details
 and then exits with error code 1. Non-zero meaning things didn't go well.
 */
void fatal_error(const char *syscall)
{
    perror(syscall);
    exit(1);
}

void sigchld_handler(int signo) {
    /* Let's call wait() for all children so that process accounting gets stats */
    while (1) {
        int wstatus;
        pid_t pid = waitpid(-1, &wstatus, WNOHANG);
		/*
		WNOHANG
    	This flag specifies that waitpid should return immediately instead of waiting, 
		if there is no child process ready to be noticed.
		*/
        if (pid == 0 || pid == -1) {
            break;
        }
        child_processes++;
    }
}

/*
 * When Ctrl-C is pressed on the terminal, the shell sends our process SIGINT. This is the
 * handler for SIGINT, like we setup in main().
 *
 * We use the getrusage() call to get resource usage in terms of user and system times and
 * we print those. This helps us see how our server is performing.
 * */
void print_stats(int signo) {
    double          user, sys;
    struct rusage   myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
        fatal_error("getrusage()");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        fatal_error("getrusage()");

    user = (double) myusage.ru_utime.tv_sec +
                    myusage.ru_utime.tv_usec/1000000.0;
    user += (double) childusage.ru_utime.tv_sec +
                     childusage.ru_utime.tv_usec/1000000.0;
    sys = (double) myusage.ru_stime.tv_sec +
                   myusage.ru_stime.tv_usec/1000000.0;
    sys += (double) childusage.ru_stime.tv_sec +
                    childusage.ru_stime.tv_usec/1000000.0;

    printf("\nuser time = %g, sys time = %g, child_processes = %d\n", user, sys, child_processes);
    exit(0);
}


int main(int argc , char *argv[]) {
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;  // https://github.com/torvalds/linux/blob/master/tools/include/uapi/linux/in.h
	char client_message[2000];

	signal(SIGINT, print_stats);
	
	// Create socket
    // AF_INET (IPv4 protocol) , AF_INET6 (IPv6 protocol) 
    // SOCK_STREAM: TCP(reliable, connection oriented)
    // SOCK_DGRAM: UDP(unreliable, connectionless)
    // Protocol value for Internet Protocol(IP), which is 0
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

    /* setup the SIGCHLD handler with SA_RESTART */
	// https://pubs.opengroup.org/onlinepubs/9699919799/functions/sigaction.html
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);  // https://es.wikipedia.org/wiki/SIGCHLD

	/*
	'sigfillset' initializes a signal set to contain all signals. 
	The signal set sa_mask is the set of signals that are blocked when the 
	signal handler is being executed. So, in your case, when you are executing 
	a signal handler all signals are blocked and you don't have to worry 
	for another signal interrupting your signal handler.
	*/
	
	//Bind the socket to the address and port number specified
	if( bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	// Listen
    // It puts the server socket in a passive mode, where it waits for the client 
    // to approach the server to make a connection. The backlog, defines the maximum 
    // length to which the queue of pending connections for sockfd may grow. If a connection 
    // request arrives when the queue is full, the client may receive an error with an 
    // indication of ECONNREFUSED.
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

    while(1) {
		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0) {
			perror("accept failed");
			return 1;
		}
		puts("Connection accepted");
	
		int pid = fork();
		if (pid == 0) {
			signal(SIGINT, SIG_DFL);
			// informs the kernel that there is no user signal handler for the 
			// given signal, and that the kernel should take default action for 
			// it (the action itself may be to ignore the signal, to terminate the program
			memset (client_message, 0, 2000);
			//Receive a message from client
			if (recv(client_sock , client_message , 2000 , 0) > 0) {
				printf("received message: %s\n", client_message);
				//Send the message back to client
				send(client_sock , client_message , strlen(client_message), 0);
			} else {
				puts("recv failed");
			}
			exit(0);
		} else {
			close(client_sock);
		}
    }
	return 0;
}

// https://unixism.net/2019/04/linux-applications-performance-part-ii-forking-servers/