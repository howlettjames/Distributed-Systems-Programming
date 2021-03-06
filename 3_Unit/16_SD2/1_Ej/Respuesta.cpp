#include <iostream>
#include <unistd.h>
#include "Respuesta.h"

using namespace std;

unsigned int Respuesta::requestId = 0;

Respuesta::Respuesta(int pl)
{
	socketlocal = new SocketDatagrama(pl);
}

Respuesta::~Respuesta()
{
	delete socketlocal;
}

void Respuesta::sendReply(char *respuesta)
{
	// CREAMOS UNA ESTRUCTURA MENSAJE PARA ENVIAR LA RESPUESTA
	struct mensaje men;

	// LLENAMOS LOS CAMPOS CORRESPONDIENTES
	men.messageType = 1;									// 0 = Solicitud, 1 = Respuesta
	men.requestId = message_recv.requestId;	
	men.operationId = message_recv.operationId;
	// EN ESTA PARTE ENVIAMOS EL O LOS ARGUMENTOS DE RESPUESTA OBTENIDOS DE REALIZAR LA OPERACIÓN SOLICITADA
	// COMO EN ESTE CASO SE TRATA DE UNA CADENA EXPRESANDO QUE SE HA REGISTRADO CORRECTAMENTE, USAREMOS STRLEN()
	memcpy(men.arguments, respuesta, strlen(respuesta));

	// CREAMOS UN DATAGRAMA PARA SER ENVIADO CON LA DIRECCIÓN IP Y PUERTO PREVIAMENTE GUARDADOS EN LAS VAR MIEMBRO DE LA CLASE
	PaqueteDatagrama paquete((char *) &men, sizeof(struct mensaje), ip, puerto);

	socketlocal->envia(paquete);
}

struct mensaje *Respuesta::getRequest(void)
{
	// RECIBIMOS LA SOLICITUD DEL CLIENTE EN UN DATAGRAMA VACÍO CON EL ESPACIO SUFICIENTE PARA ALMACENAR UN MENSAJE
	PaqueteDatagrama paquete(sizeof(struct mensaje));
	socketlocal->recibe(paquete);

	// COPIAMOS LOS DATOS DEL MENSAJE HACIA NUESTRA VARIABLE MIEMBRO "MESSAGE_RECV"
	memcpy(&message_recv, paquete.obtieneDatos(), sizeof(struct mensaje));
		
	// GUARDAMOS LA DIRECCIÓN DEL IP Y EL PUERTO DEL CLIENTE QUE HACE LA SOLICITUD, PARA POSTERIORMENTE IDENTIFICARLO EN NUESTRA PEQUEÑA BD
	memcpy(ip, paquete.obtieneDireccion(), strlen(paquete.obtieneDireccion()));
	ip[strlen(paquete.obtieneDireccion())] = '\0';
	puerto = paquete.obtienePuerto();
	
	// COMPROBAMOS EL ID DE SOLICITUD DEL MENSAJE DEL CLIENTE RECIBIDO, LOS IDS DEBEN IR A LA PAR
	// PRIMER CASO: SI ES IGUAL O MENOR A LA QUE YA TENEMOS GUARDADO, SIGNIFICA QUE ES UNA SOLICITUD QUE YA SE HA HECHO
	// EN CUYO CASO MODIFICAMOS EL ID DE OPERACIÓN A 0, INDICANDO AL SERVIDOR QUE NO REALICE NINGUNA OPERACIÓN
	if(message_recv.requestId < requestId)
	{
		cout << "Esta solicitud ya ha sido hecha." << endl;
		message_recv.operationId = 0;
	}
	else if(message_recv.requestId > requestId) 
	{
		// SE TRATA DE UNA NUEVA ID DE SOLICITUD ERRÓNEO, PUES LOS IDS DE SOLICITUD DEBEN IR A LA PAR
		cout << "Esta solicitud viene con un ID erroneo." << endl;
		message_recv.operationId = 0;
	}
	else // SI LOS IDS SON IGUALES, QUIERE DECIR QUE VAMOS A LA PAR CON LAS SOLICITUDES DEL CLIENTE Y SE PROCEDE A HACER LA OPERACIÓN
		requestId++;

	return &message_recv;
}