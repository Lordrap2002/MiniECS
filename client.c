#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in server;
	char args[2][100], server_reply[2000];
	
	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) {
		printf("Error al crear el socket");
	}
	puts("Socket creado");
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("Error al conectar con el servidor");
		return 1;
	}
	
	puts("Connected\n");
	
	//keep communicating with server
	while(1){
		printf("Menu:\n"
				"1. Crear contenedor.\n"
				"2. Listar contenedores.\n"
				"3. Detener contenedor.\n"
				"4. Borrar contenedor.\n"
				"Opcion: ");
		scanf("%s", args[0]);
		if(atoi(args[0])  > 2){
			printf("Por favor escriba el nombre del contenedor: ");
			scanf("%s", args[1]);
		}
		//Send some data
		if(send(sock, args, sizeof(args), 0) < 0){
			puts("Error al enviar la peticion");
			return 1;
		}else{
            puts("peticion enviada");
        }
		//Receive a reply from the server
		
        memset(server_reply, 0, 2000 );
		if(recv(sock , server_reply , 2000 , 0) < 0) {
			puts("recv failed");
			break;
		} else {
            puts("recv ok");
        }
		
		puts("Server reply :");
		puts(server_reply);
		
		if(!strcmp(args[0], "-1")){
			sleep(10);
			break;
		}
	}
	close(sock);
	return 0;
}