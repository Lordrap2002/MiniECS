#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void *crearContenedor(void *param);
void *listarContenedores();
void *detenerContenedor(void *param);
void *eliminarContenedor(void *param);

int totalContenedores, n, client_sock;
char nombres[10][15];

int main(int argc , char *argv[]) {
	int socket_desc, c, read_size, pid, opc, flag;
	struct sockaddr_in server, client;
	char args[2][100], *nom = malloc(100);
	totalContenedores = n = 0;

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Error al crear el socket");
	}
	printf("Socket creado.\n");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	
	//Bind the socket to the address and port number specified
	if( bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("Error al crear la conexion.\n");
		return 1;
	}
	printf("Conexion creada.\n");

	listen(socket_desc , 3);
	
	//Accept and incoming connection
	printf("Esperando peticiones...\n");
	c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	printf("Conexion acceptada.\n");
	flag = 1;
    while(flag){
        memset(args, 0, 2000);
        //Receive a message from client
        if(recv(client_sock , args, 200, 0) > 0) {

			opc = atoi(args[0]);
			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);   /* get the default attributes */

			switch(opc){
				case 1:
					strcpy(nom, args[1]);
					pthread_create(&tid, &attr, crearContenedor, nom);
					break;
				case 2:
					pthread_create(&tid, &attr, listarContenedores, NULL);
					break;
				case 3:
					strcpy(nom, args[1]);
					pthread_create(&tid, &attr, detenerContenedor,  nom);
					break;
				case 4:
					strcpy(nom, args[1]);
					pthread_create(&tid, &attr, eliminarContenedor, nom);
					break;
				case -1:
					flag--;
					break;
			}
			//pthread_join(tid, NULL);
        }else{
            printf("Error al recibir.\n");
			break;
        }
    }
	free(nom);
	close(client_sock);
	//shutdown(client_sock, SHUT_RDWR);
	return 0;
}

void *crearContenedor(void *param){
	char nombre[15] = "container", mensaje[100] = "Contenedor creado con el nombre: ", num = '0', l[2] = "\0";
	char *imagen = (char *) param;
	pthread_t self = pthread_self();
    pthread_detach(self);
	num += n;
	l[0] = num;
    strcat(nombre, l);
	strcpy(nombres[totalContenedores], nombre);
	strcat(mensaje, nombre);
	//crear contenedor
	int pid;
	pid = fork();
	if(pid < 0){
		printf("Error al crear el hijo.\n");
        pthread_exit(NULL);
	}else if(pid){//papá
		totalContenedores++;
		n++;
		wait(NULL);
	}else{//hijo
		//exec sudo docker run -di --name <nombre> <imagen:version>
		char *arg0 = "sudo", *arg1 = "docker", *arg2 = "run", *arg3 = "-di", *arg4 = "--name";
		execlp(arg0, arg0, arg1, arg2, arg3, arg4, nombre, imagen, NULL);
	}
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}

void *listarContenedores(){
	char datos[2000];
	pthread_t self = pthread_self();
    pthread_detach(self);
	int pid = fork(), tubo;
	char *mitubo = "/tmp/mitubo";
    mkfifo(mitubo, 0666);
	if(pid < 0){
		printf("Error al crear el hijo.\n");
		pthread_exit(NULL);
	}else if(pid){//papá
		tubo = -1;
		while (tubo == -1) {
			tubo = open(mitubo, O_RDONLY);
		}
		read(tubo, datos, 2000);
		wait(NULL);
		read(tubo, datos, 2000);
		close(tubo);
		send(client_sock, datos, sizeof(datos), 0);
	}else{//hijo
		tubo = open(mitubo, O_WRONLY);
		write(tubo, "prueba", sizeof("prueba"));
		if(dup2(tubo, STDOUT_FILENO) < 0) {
			printf("Unable to duplicate file descriptor of pipe hijo1.");
			return 0;
		}
		close(tubo);
		//exec sudo docker stop <nombre>
		char *arg0 = "sudo", *arg1 = "docker", *arg2 = "ps", *arg3 = "-a";
		execlp(arg0, arg0, arg1, arg2, arg3, NULL);
	}
	pthread_exit(NULL);
}

void *detenerContenedor(void *param){
	int i;
	char *nombre = (char *) param, mensaje[50];
	pthread_t self = pthread_self();
    pthread_detach(self);
	for(i = 0; i < totalContenedores; i++){
		if(!strcmp(nombre, nombres[i])){
			//detener contenedor
			int pid = fork();
			if(pid < 0){
				printf("Error al crear el hijo.\n");
				pthread_exit(NULL);
			}else if(pid){//papá
				wait(NULL);
			}else{//hijo
				//exec sudo docker stop <nombre>
				char *arg0 = "sudo", *arg1 = "docker", *arg2 = "stop";
				execlp(arg0, arg0, arg1, arg2, nombre, NULL);
			}
			strcpy(mensaje, "Contenedor detenido: ");
			strcat(mensaje, nombre);
			send(client_sock, mensaje, sizeof(mensaje), 0);
			pthread_exit(NULL);
		}
	}
	strcpy(mensaje, "Contenedor no encontrado.");
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}

void *eliminarContenedor(void *param){
	int flag = 0, i;
	char *nombre = (char *) param, mensaje[40];
	pthread_t self = pthread_self();
    pthread_detach(self);
	for(i = 0; i < totalContenedores; i++){
		if(!strcmp(nombre, nombres[i])){
			//eliminar contenedor
			int pid = fork();
			if(pid < 0){
				printf("Error al crear el hijo.\n");
				pthread_exit(NULL);
			}else if(pid){//papá
				flag++;
				totalContenedores--;
				wait(NULL);
			}else{//hijo
				//exec sudo docker rm <nombre>
				char *arg0 = "sudo", *arg1 = "docker", *arg2 = "rm";
				execlp(arg0, arg0, arg1, arg2, nombre, NULL);
			}
		}
		if(flag){
			strcpy(nombres[i], nombres[i + 1]);
		}
	}
	if(flag){
		strcpy(mensaje, "Contenedor eliminado: ");
		strcat(mensaje, nombre);
	}else{
		strcpy(mensaje, "Contenedor no encontrado.");
	}
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}
