#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>

void *runner(void *param);

int main(int argc , char *argv[]) {
	int socket_desc, client_sock, c, read_size, pid, opc;
	struct sockaddr_in server, client;
	char args[2][100];

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Error al crear el socket");
	}
	puts("Socket creado");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	
	//Bind the socket to the address and port number specified
	if( bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Esperando peticiones...");
	c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");
    while(1){
        memset(args, 0, 2000);
        //Receive a message from client
        if (recv(client_sock , args, 2000 , 0) > 0) {
            printf("Opcion: %s, nombre: %s.\n", args[0], (atoi(args[0]) > 2 ? args[1] : ""));

			pthread_t tid;
			pthread_attr_t attr;

			pthread_attr_init(&attr);   /* get the default attributes */
			pthread_create(&tid, &attr, runner, args[0]);   /* create the thread */
			pthread_join(tid, NULL);
            //Send the message back to client
            send(client_sock, "client_message", strlen("client_message"), 0);
			printf("%d\n", atoi(args[0]));
			opc = atoi(args[0]);
			printf("%d\n", opc);
			if(opc == -1){
				printf("aa");
				sleep(5);
				break;
			}
			/*
			pid = fork();
			if(pid < 0){
				printf("Error al crear el hijo");
				return 0;
			}else if(pid > 0){
			}else{
				sleep(10);
				return 0;
			}*/
        }else{
            puts("recv failed");
        }
    }
	return 0;
}

void *runner(void *param) {
	char *message = (char *) param;
	printf("soy el hilo y el mensaje es: %s\n", message);
	pthread_exit(0);
}