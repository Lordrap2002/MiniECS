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
#include <semaphore.h>

//estructuras
typedef struct{
	char nombre[15], status;
} Contenedor;

typedef struct{
    int socket_client;
    char nom[100], imagen[100];
} Param;

//funciones de los hilos
void *crearHilo(void *para);
void *crearContenedor(void *para);
void *listarContenedores(void *para);
void *detenerContenedor(void *para);
void *eliminarContenedor(void *para);

//variables globales
int socket_desc;
Contenedor contenedores[10];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t semaforo;

//funcion que verifica si existe el contenedor
int verificarLog(char *nombre, int tipo){
	int i, flag = 0, n;
	char nom[15], stat;
	FILE *log;
	log = fopen("size.txt", "r");
	fscanf(log, "%d", &n);
	fclose(log);
	log = fopen("log.txt", "r");
	for(i = 0; i < n; i++){
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

//funcion que actualiza el archivo que se usa como registro
void actualizarLog(char *nombre, int tipo){
	int i, flag = 0, n;
	FILE *log, *log1;
	log = fopen("size.txt", "r");
	fscanf(log, "%d", &n);
	fclose(log);
	if(tipo){
		log = fopen("log.txt", "r");
		for(i = 0; i < n; i++){
			fscanf(log, "%15s %c\n", contenedores[i].nombre, &contenedores[i].status);
			if(!strcmp(contenedores[i].nombre, nombre)){
				if(tipo == 1){
					contenedores[i].status = 's';
				}else{
					log1 = fopen("size.txt", "w");
					fprintf(log1, "%d", n - 1);
					fclose(log1);
				}
			}
		}
		fclose(log);
		log = fopen("log.txt", "w");
		for(i = 0; i < n; i++){
			if(!(!strcmp(nombre, contenedores[i].nombre) && tipo == 2)){
				fprintf(log, "%-15s %c\n", contenedores[i].nombre, contenedores[i].status);
			}
		}
		fclose(log);
	}else{
		log = fopen("size.txt", "w");
		fprintf(log, "%d", ++n);
		fclose(log);
		log = fopen("log.txt", "a");
		fprintf(log, "%-15s r\n", nombre);
		fclose(log);
	}
	return;
}


int main(int argc , char *argv[]) {
	int client_sock, c, read_size, pid, opc, flag;
	struct sockaddr_in server, client;
	char args[3][100];
	//crear semaforo
	sem_init(&semaforo, 0, 1);
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
	listen(socket_desc , 10);
	//Aceptar las conexiones entrantes
	printf("Esperando peticiones...\n");
	c = sizeof(struct sockaddr_in);
	flag = 20;
    while(1){
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0) {
			continue;
		}
		printf("Conexion acceptada.\n");
		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		int val = client_sock;
		pthread_create(&tid, &attr, crearHilo, &val);
		//close(client_sock);
		//pthread_join(tid, NULL);
    }
	//liberar la memoria y cerrar el socket
	close(socket_desc);
	sem_destroy(&semaforo); 
	//shutdown(socket_desc, SHUT_RDWR);
	printf("Apagado\n");
	return 0;
}

void *crearHilo(void *para){
	int *s = (int *) para, opc;
	int sock = *s;
	char args[3][100];
	pthread_t self = pthread_self();
    pthread_detach(self);
	//memset(args, 0, 2000);
	//Recibir mensaje del cliente
	while(1){
		if(recv(sock , args, 300, 0) > 0) {
			opc = atoi(args[0]);
			//crear el hilo
			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			//mandar el hilo con la funcion dependiendo de la peticion del cliente
			Param *parametro = malloc(sizeof(Param));
			parametro->socket_client = sock;
			switch(opc){
				case 1:
					strcpy(parametro->imagen, args[1]);
					strcpy(parametro->nom, args[2]);
					pthread_create(&tid, &attr, crearContenedor, parametro);
					break;
				case 2:
					pthread_create(&tid, &attr, listarContenedores, parametro);
					break;
				case 3:
					strcpy(parametro->nom, args[1]);
					pthread_create(&tid, &attr, detenerContenedor,  parametro);
					break;
				case 4:
					strcpy(parametro->nom, args[1]);
					pthread_create(&tid, &attr, eliminarContenedor, parametro);
					break;
				case -1:
					break;
			}
		}else{
			printf("Conexion finalizada.\n");
			break;
		}
	}
	close(sock);
	pthread_exit(NULL);
}

void *crearContenedor(void *para){
	Param *par = (Param *) para; 
	char *nombre = par->nom, mensaje[100] = "Contenedor creado con el nombre: ";
	pthread_t self = pthread_self();
    pthread_detach(self);
	pthread_mutex_lock(&mutex);
	//sleep(5);
	//verificar nombre evitando repetir
	if(!verificarLog(nombre, 1)){
		actualizarLog(nombre, 0);	
		pthread_mutex_unlock(&mutex);
		//crear hijo
		int pid;
		pid = fork();
		if(pid < 0){
			printf("Error al crear el hijo.\n");
			pthread_exit(NULL);
		}else if(pid){//papá
			strcat(mensaje, nombre);
			wait(NULL);
		}else{//hijo crea contenedor
			//exec sudo docker run -di --name <nombre> <imagen:version>
			char *arg0 = "sudo", *arg1 = "docker", *arg2 = "run", *arg3 = "-di", *arg4 = "--name";
			execlp(arg0, arg0, arg1, arg2, arg3, arg4, nombre, par->imagen, NULL);
		}
	}else{
		strcpy(mensaje, "El contenedor ya existe");
	}
	//enviar al cliente nombre del contenedor creado
	send(par->socket_client, mensaje, sizeof(mensaje), 0);
	//close(par->socket_client);
	free(par);
	pthread_exit(NULL);
}

void *listarContenedores(void *para){
	Param *par = (Param *) para; 
	char datos[2000];
	pthread_t self = pthread_self();
    pthread_detach(self);
	//cambiar semaforo
	sem_wait(&semaforo);
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
		//sleep(5);
		send(par->socket_client, datos, sizeof(datos), 0);
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
	//close(par->socket_client);
	free(par);
	sem_post(&semaforo);
	pthread_exit(NULL);
}

void *detenerContenedor(void *para){
	Param *par = (Param *) para;
	int i;
	char *nombre = par->nom, mensaje[50];
	pthread_t self = pthread_self();
    pthread_detach(self);
	pthread_mutex_lock(&mutex);
	//sleep(5);
	//buscar contenedor dentro de los creados por el servidor
	if(verificarLog(nombre, 2)){
		actualizarLog(nombre, 1);	
		pthread_mutex_unlock(&mutex);
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
		send(par->socket_client, mensaje, sizeof(mensaje), 0);
		pthread_exit(NULL);
	}
	//enviar respuesta en caso de no encontrar el contenedor
	strcpy(mensaje, "El contenedor no existe o ya está detenido.");
	send(par->socket_client, mensaje, sizeof(mensaje), 0);
	//close(par->socket_client);
	free(par);
	pthread_exit(NULL);
}

void *eliminarContenedor(void *para){
	Param *par = (Param *) para; 
	int flag = 0, i;
	char *nombre = par->nom, mensaje[50];
	pthread_t self = pthread_self();
    pthread_detach(self);
	pthread_mutex_lock(&mutex);
	//sleep(5);
	if(verificarLog(nombre, 3)){
		actualizarLog(nombre, 2);
		pthread_mutex_unlock(&mutex);
		//crear hijo
		int pid = fork();
		if(pid < 0){
			printf("Error al crear el hijo.\n");
			pthread_exit(NULL);
		}else if(pid){//papá
			flag++;
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
	send(par->socket_client, mensaje, sizeof(mensaje), 0);
	//close(par->socket_client);
	free(par);
	pthread_exit(NULL);
}
