#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int sock, opc, contenedores, i, confirm = 1;
	struct sockaddr_in server;
	char args[2][100], server_reply[2000];
	
	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) {
		printf("Error al crear el socket\n");
	}
	printf("Socket creado\n");
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("Error al conectar con el servidor\n");
		return 1;
	}
	
	printf("Connectado al servidor.\n");
	
	//keep communicating with server
	while(1){
		printf("Menu:\n"
				"1. Crear contenedor.\n"
				"2. Listar contenedores.\n"
				"3. Detener contenedor.\n"
				"4. Borrar contenedor.\n"
				"-1. Salir.\n"
				"Opcion: ");
		scanf("%s", args[0]);
		opc = atoi(args[0]);
		if(opc > 2){
			printf("Por favor escriba el nombre del contenedor: ");
			scanf("%s", args[1]);
		}else if(opc == -1){
			break;
		}
		//Send some data
		if(send(sock, args, sizeof(args), 0) < 0){
			printf("Error al enviar la peticion.\n");
			return 1;
		}else{
            printf("Peticion enviada.\n");
        }
		//Receive a reply from the server
		
		if(opc - 2){
			memset(server_reply, 0, 2000 );
			if(recv(sock , server_reply , 2000 , 0) < 0){
				printf("Sin respuesta del servidor.\n");
				break;
			}else{
				printf("Respuesta del servidor recibida.\n");
			}
			printf("Respuesta del servidor: ");
			printf("%s\n", server_reply);
		}else{
			if(recv(sock, &contenedores, sizeof(int), 0) < 0){
				printf("Sin respuesta del servidor.\n");
				break;
			}
			printf("Cantidad de contenedores: %d.\n", contenedores);
			for(i = 0; i < contenedores; i++){
				sleep(2);
				send(sock , &confirm, sizeof(int), 0);
				memset(server_reply, 0, 2000 );
				if(recv(sock, server_reply, 2000, 0) < 0){
					printf("Sin respuesta del servidor.\n");
					break;
				}
				printf("%d. %s.\n", (i + 1), server_reply);
			}
		}
	}
	close(sock);
	return 0;
}