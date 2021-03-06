#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> 	// Para struct timeval
#include <vector>  		// Vector
#include <algorithm> 	// sort()
#include <unistd.h>		// sleep()

#include "Respuesta.h"
#include "registro.h"

using namespace std;

#define PUERTO_A_ESCUCHAR 7200

// bool compare_function(string a, string b) { return a < b; }

int main(int argc, char const *argv[])
{
	int registros_fd;								// File descriptor del archivo en donde escribiremos los registros
	struct timeval timestamp;						// Estructura timeval permite almacenar la hora exacta
	vector<string> celulares;						// Vector en el cual guardaremos todas los celulares recibidos
	vector<string>::iterator index_to_insert;		// Iterador auxiliar que nos ayudará a saber donde insertar cada nuevo celular

	// CHECANDO QUE LA INVOCACIÓN SEA CORRECTA
	if(argc != 2)
	{
		printf("Forma de uso: %s <nombre_archivo_guardar_registros>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// ABRIENDO ARCHVIO PARA ESCRITURA DE REGISTROS
	if((registros_fd = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1)
	{
		perror(argv[1]);
		exit(-1);
	}

	// CREAMOS UN OBJETO RESPUESTA PARA ATENDER AL CLIENTE
	Respuesta respuesta(PUERTO_A_ESCUCHAR);

	// VAR PARA ALMACENAR EL MENSAJE RECIBIDO
	struct mensaje *mensaje_recv;
	
	// CREAMOS UNA VAR DE REGISTRO PARA ALMACENAR TEMPORALMENTE LOS REGISTROS RECIBIDOS
	struct registro_time registro_recibido;
	// EL SERVIDOR CORRE INDEFINIDAMENTE, ESCUCHANDO EN EL PUERTO 7200
	while(1)
	{
		// printf("Esperando solicitud...\n");
		
		// GUARDAMOS LA REFERENCIA AL MENSAJE RECIBIDO
		mensaje_recv = respuesta.getRequest();

		// IMPRIMIMOS LA INFORMACIÓN RECIBIDA 
		// cout << "SOLICITUD RECIBIDA" << endl;
		// cout << "Tipo de mensaje: " << mensaje_recv->messageType << endl;
		// cout << "ID de solicitud: " << mensaje_recv->requestId << endl;
		// cout << "ID de operación: " << mensaje_recv->operationId << endl;
		
		// COPIAMOS EL(LOS) ARGUMENTOS(REGISTRO) EN LA VAR REGISTRO_RECIBIDO
		memcpy(&registro_recibido, mensaje_recv->arguments, sizeof(struct registro));

		// EJECUTAMOS LA OPERACIÓN SOLICITADA
		if(mensaje_recv->operationId == guardar_registro)
		{
			// CREANDO UNA INSTANCIA DE OBJETO STRING A PARTIR DE LA CADENA ESTILO C CLÁSICA, EN ESTE CASO DEL CELULAR DE QUIEN SE HIZO EL REGISTRO
			string celularx(registro_recibido.celular);

			// HACEMOS LA BÚSQUEDA DE ESTE CELULAR SOBRE NUESTRO VECTOR DE CELULARES ORDENADO
			if(binary_search(celulares.begin(), celulares.end(), celularx))
			{
				cout << "Ya existía este celular: " << celularx << endl;
				// sleep(3);
				// EN CASO DE EXISTIR PREVIAMENTE EL CELULAR EN NUESTRA BD, REGRESAMOS UN TIMSTAMP CON SEGUNDOS Y MICROSEGUNDOS IGUALES A 0
				registro_recibido.timestamp.tv_sec = 0;
				registro_recibido.timestamp.tv_usec = 0;
			}
			else
			{
				// cout << "Este celular es nuevo, guardando el registro en la BD..." << endl;
				// BUSCAMOS EN QUE POSICIÓN DEBERÍA SER INSERTADO EL NUEVO CELULAR DE MANERA QUE SE MANTENGA ORDENADO EL VECTOR Y LO INSERTAMOS
				// PARA ASÍ NO TENER QUE LLAMAR A LA FUNCIÓN SORT() CADA QUE INSERTE UN NUEVO CELULAR
				index_to_insert = lower_bound(celulares.begin(), celulares.end(), celularx);
				celulares.insert(index_to_insert, celularx);

				// OBTENIENDO LA HORA EXACTA ACTUAL, PARA GUARDAR EL REGISTRO JUNTO CON LA HORA EN QUE FUE ALMACENADO EN LA BD
				gettimeofday(&registro_recibido.timestamp, NULL);
				// ESCRIBIENDO EL REGISTRO EN EL ARCHIVO INDICADO POR LÍNEA DE COMANDOS
				write(registros_fd, &registro_recibido, sizeof(struct registro_time));
				// HACEMOS FSYNC() PARA QUE EL REGISTRO SEA ESCRITO INMEDIATAMENTE A DISCO
				fsync(registros_fd);

				// IMPRIMIMOS LA INFORMACIÓN DEL REMITENTE
				// printf("REGISTRO\n");
				// printf("Cel: %s\n", registro_recibido.celular);
				// printf("CURP: %s\n", registro_recibido.CURP);
				// printf("Partido: %s\n", registro_recibido.partido);
				// printf("Segundos:Microsegundos -> %ld : %ld\n", registro_recibido.timestamp.tv_sec, registro_recibido.timestamp.tv_usec);
				// printf("Registro almacenado en el archivo: %s\n", argv[1]);
			}

			// ENVIAMOS LA RESPUESTA
			respuesta.sendReply((char *) &registro_recibido.timestamp, sizeof(struct timeval));
			// printf("Confirmacion enviada.\n\n");

			// IMPRIMIENDO LOS CELULARES
			// for(auto i = celulares.begin(); i != celulares.end(); ++i)
			// {
			// 	string stringx = *i;
			// 	cout << stringx << endl;
			// }
			// printf("\n");
		}
		else if(mensaje_recv->operationId == 0)
		{
			// SI EL ID DE OPERACIÓN ES CERO, EL SERVIDOR NO EFECTUARÁ NINGUNA OPERACIÓN, SE DESCARTA LA SOLICITUD REPETIDA
			continue;	
		}
	}

	return 0;
}