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

typedef struct{
	char nombre[15], status;
} Contenedor;

//funciones de los hilos
void *crearContenedor(void *param);
void *listarContenedores();
void *detenerContenedor(void *param);
void *eliminarContenedor(void *param);

//variables globales
int totalContenedores, n, client_sock;
Contenedor contenedores[10];

int verificarLog(char *nombre, int tipo){
	int i, flag = 0;
	char nom[15], stat;
	FILE *log;
	log = fopen("log.txt", "r");
	for(i = 0; i < totalContenedores; i++){
        fscanf(log, "%15s %c", nom, &stat);
		if(!strcmp(nom, nombre)){
			if((tipo == 1) || (tipo == 2 && stat == 'r') || (tipo == 3 && stat == 's')){
				flag++;
			}
			break;
		}
    }
	fclose(log);
	return flag;
}

void actualizarLog(char *nombre, int tipo){
	int i, flag = 0;
	FILE *log;
	if(tipo){
		log = fopen("log.txt", "r");
		for(i = 0; i < totalContenedores; i++){
			fscanf(log, "%15s %c\n", contenedores[i].nombre, &contenedores[i].status);
			if(!strcmp(contenedores[i].nombre, nombre)){
				if(tipo == 1){
					contenedores[i].status = 's';
				}else{
					contenedores[i].status = 'd';
				}
			}
		}
		fclose(log);
		log = fopen("log.txt", "w");
		for(i = 0; i < totalContenedores; i++){
			if(!(!strcmp(nombre, contenedores[i].nombre) && tipo == 2)){
				fprintf(log, "%-15s %c\n", contenedores[i].nombre, contenedores[i].status);
			}
		}
		fclose(log);
	}else{
		log = fopen("log.txt", "a");
		fprintf(log, "%-15s r\n", nombre);
		fclose(log);
	}
	return;
}


int main(int argc , char *argv[]) {
	int socket_desc, c, read_size, pid, opc, flag;
	struct sockaddr_in server, client;
	char args[2][100], *nom = malloc(100);
	totalContenedores = n = 0;
	//crear el socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if(socket_desc == -1){
		printf("Error al crear el socket");
	}
	printf("Socket creado.\n");
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	//Unir el socket a la direccion y al puerto especificados
	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error al crear la conexion.\n");
		return 1;
	}
	printf("Conexion creada.\n");
	listen(socket_desc , 3);
	//Aceptar las conexiones entrantes
	printf("Esperando peticiones...\n");
	c = sizeof(struct sockaddr_in);
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	printf("Conexion acceptada.\n");
	flag = 1;
    while(flag){
        memset(args, 0, 2000);
        //Recibir mensaje del cliente
        if(recv(client_sock , args, 200, 0) > 0) {
			opc = atoi(args[0]);
			//crear el hilo
			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			//mandar el hilo con la funcion dependiendo de la peticion del cliente
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
	//liberar la memoria y cerrar el socket
	free(nom);
	close(client_sock);
	//shutdown(client_sock, SHUT_RDWR);
	return 0;
}

void *crearContenedor(void *param){
	char nombre[15] = "container", mensaje[100] = "Contenedor creado con el nombre: ", num = '0', l[2] = "\0";
	char *imagen = (char *) param;
	//Contenedor contenedores[totalContenedores];
	pthread_t self = pthread_self();
    pthread_detach(self);
	//generar nombre evitando repetir
	num += n;
	l[0] = num;
	strcat(nombre, l);
	strcat(mensaje, nombre);
	if(!verificarLog(nombre, 1)){
		//crear hijo
		int pid;
		pid = fork();
		if(pid < 0){
			printf("Error al crear el hijo.\n");
			pthread_exit(NULL);
		}else if(pid){//papá
			actualizarLog(nombre, 0);
			//actualizar numero de contenedores
			totalContenedores++;
			n++;
			wait(NULL);
		}else{//hijo crea contenedor
			//exec sudo docker run -di --name <nombre> <imagen:version>
			char *arg0 = "sudo", *arg1 = "docker", *arg2 = "run", *arg3 = "-di", *arg4 = "--name";
			execlp(arg0, arg0, arg1, arg2, arg3, arg4, nombre, imagen, NULL);
		}
	}else{
		strcpy(mensaje, "El contenedor ya existe");
	}
	//enviar al cliente nombre del contenedor creado
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}

void *listarContenedores(){
	char datos[2000];
	//Contenedor contenedores[totalContenedores];
	pthread_t self = pthread_self();
    pthread_detach(self);
	//crear hijo
	int pid = fork(), tubo;
	//crear tubo para enviar datos de hijo a padre
	char *mitubo = "/tmp/mitubo";
    mkfifo(mitubo, 0666);
	if(pid < 0){
		printf("Error al crear el hijo.\n");
		pthread_exit(NULL);
	}else if(pid){//papá espera mensaje del hijo
		tubo = -1;
		while (tubo == -1) {
			tubo = open(mitubo, O_RDONLY);
		}
		read(tubo, datos, 2000);
		wait(NULL);
		read(tubo, datos, 2000);
		close(tubo);
		//enviar datos al cliente
		send(client_sock, datos, sizeof(datos), 0);
	}else{//hijo obtiene la descripcion de los contenedores
		tubo = open(mitubo, O_WRONLY);
		write(tubo, "No hay contenedores", sizeof("No hay contenedores"));
		if(dup2(tubo, STDOUT_FILENO) < 0) {
			printf("Unable to duplicate file descriptor of pipe hijo1.");
			return 0;
		}
		close(tubo);
		//exec cat log.txt
		char *arg0 = "cat", *arg1 = "log.txt";
		execlp(arg0, arg0, arg1, NULL);
	}
	pthread_exit(NULL);
}

void *detenerContenedor(void *param){
	int i;
	char *nombre = (char *) param, mensaje[50];
	//Contenedor contenedores[totalContenedores];
	pthread_t self = pthread_self();
    pthread_detach(self);
	//buscar contenedor dentro de los creados por el servidor
	if(verificarLog(nombre, 2)){
		//crear hijo
		int pid = fork();
		if(pid < 0){
			printf("Error al crear el hijo.\n");
			pthread_exit(NULL);
		}else if(pid){//papá
			wait(NULL);
		}else{//hijo detiene contenedor
			//exec sudo docker stop <nombre>
			char *arg0 = "sudo", *arg1 = "docker", *arg2 = "stop";
			execlp(arg0, arg0, arg1, arg2, nombre, NULL);
		}
		strcpy(mensaje, "Contenedor detenido: ");
		strcat(mensaje, nombre);
		//enviar confirmacion al cliente
		send(client_sock, mensaje, sizeof(mensaje), 0);
		actualizarLog(nombre, 1);
		pthread_exit(NULL);
	}
	//enviar respuesta en caso de no encontrar el contenedor
	strcpy(mensaje, "El contenedor no existe o ya está detenido.");
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}

void *eliminarContenedor(void *param){
	int flag = 0, i;
	char *nombre = (char *) param, mensaje[50];
	//Contenedor contenedores[totalContenedores];
	pthread_t self = pthread_self();
    pthread_detach(self);
	if(verificarLog(nombre, 3)){
		//crear hijo
		int pid = fork();
		if(pid < 0){
			printf("Error al crear el hijo.\n");
			pthread_exit(NULL);
		}else if(pid){//papá
			flag++;
			actualizarLog(nombre, 2);
			totalContenedores--;
			wait(NULL);
		}else{//hijo elimina contenedor
			//exec sudo docker rm <nombre>
			char *arg0 = "sudo", *arg1 = "docker", *arg2 = "rm";
			execlp(arg0, arg0, arg1, arg2, nombre, NULL);
		}
	}
	//enviar respuesta al cliente
	if(flag){
		strcpy(mensaje, "Contenedor eliminado: ");
		strcat(mensaje, nombre);
	}else{
		strcpy(mensaje, "El contenedor no existe o no está detenido.");
	}
	send(client_sock, mensaje, sizeof(mensaje), 0);
	pthread_exit(NULL);
}
