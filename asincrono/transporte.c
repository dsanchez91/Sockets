/*
 ** Fichero: transporte.c
 ** Autores:
 ** Alvaro Valiente Herrero
 ** Diego Sanchez Moreno 
 ** Usuario: i8112600
 */

#include "transporte.h"


int crearSocket (TPDU tpdu, int rol)
{
    int sudp;//socket tcp
    int stcp;//socket udp

	struct sockaddr_in peeraddr_in;	/* for peer socket address */
	struct sockaddr_in servaddr_in;	/* for server socket address */
	struct sockaddr_in myaddr_in;	/* for client udp socket address */

    memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));

    //Parte del servidor
    if(rol==SERVER)
    {
        //Familia de direcciones
        servaddr_in.sin_family = AF_INET;

        //Direcciones de red disponibles
        servaddr_in.sin_addr.s_addr = INADDR_ANY;

        //Puerto para la comunicacion
        servaddr_in.sin_port = tpdu.port;

        //protocolo TCP
        if(tpdu.protocol == IPPROTO_TCP)
        {
            //Creamos el socket TCP
            stcp = socket(servaddr_in.sin_family, SOCK_STREAM, 0);
            if (stcp == -1) {

                fprintf(stderr,"Unable to create datagram socket\n");
                return -1;
            }

            //Asociamos el socket a la direccion ip y al puerto
            if (bind(stcp, (const struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
                fprintf(stderr,"Unable to bind address (stcp)\n");
                return -1;
            }

            //Creamos la cola de escucha para el socket
            if (listen(stcp, 6) == -1 )
            {
                fprintf(stderr, "\nUnable to listen \n");
                return -1;
            }

            //devolvemos el socket creado y configurado
            return stcp;
        }

        //protocolo UDP
        if(tpdu.protocol == IPPROTO_UDP)
        {
            //Creamos el socket UDP
            sudp = socket(servaddr_in.sin_family, SOCK_DGRAM, 0);
            if (sudp == -1) {
                fprintf(stderr, "Unable to create datagram socket\n");
                return -1;
            }

            //Asociamos el socket a la direccion ip y al puerto
            if (bind(sudp, (const struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
                fprintf(stderr, "Unable to bind address (sudp)\n");
                return -1;
            }

            //devolvemos el socket creado y configurado
            return sudp;
        }
    }

    //Parte del cliente
    if(rol==CLIENT)
    {
        //Familia de direcciones
        peeraddr_in.sin_family = AF_INET;

        //Direccion a la que vamos a conectar, previamente obtenida en el cliente
        peeraddr_in.sin_addr= tpdu.server;

        //Puerto para la conexion
        peeraddr_in.sin_port = tpdu.port;

        //protocolo TCP
        if(tpdu.protocol == IPPROTO_TCP)
        {
            //Creamos el socket
            stcp = socket (peeraddr_in.sin_family, SOCK_STREAM, 0);
            if (stcp == -1)
            {
                fprintf(stderr, "Unable to create socket\n");
                return -1;
            }

            //Conectamos el socket a la direccion y puerto antes configurados
            if (connect(stcp, (const struct sockaddr *)&peeraddr_in, sizeof(struct sockaddr_in)) == -1) {
                fprintf(stderr, "\nUnable to connect to remote\n");
                return -1;
            }

            //devolvemos el socket creado y conectado
            return stcp;
        }

        //protocolo UDP
        if(tpdu.protocol == IPPROTO_UDP)
        {
            //Creamos el socket
            sudp = socket (peeraddr_in.sin_family, SOCK_DGRAM, 0);
            if (sudp == -1)
            {
                fprintf(stderr, "Unable to create socket\n");
                return -1;
            }

            /*
            Hacemos bind al socket indicandole la familia de direcciones,
            una direccion disponibles y un puerto efimero disponible (para esto usamos el 0)
            */
            myaddr_in.sin_family = AF_INET;
            myaddr_in.sin_port = 0;
            myaddr_in.sin_addr.s_addr = INADDR_ANY;

            if (bind(sudp, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1)
            {
                fprintf(stderr, "Unable to bind socket\n");
                return -1;
            }

            //devolvemos el socket creado y bindeado
            return sudp;
        }
    }
}


ssize_t Enviar (int id_socket, const void *mensaje, size_t longitud, TPDU tpdu)
{

    struct sockaddr_in envaddr;
	char *aux = NULL;
	int ret;

    //castemos el mensaje a chat
	aux = ((char*)mensaje);

    //segun que protocolo realizaremos unas operaciones u otras
	switch (tpdu.protocol)
	{
		case IPPROTO_TCP:
            //enviamos con la funcion send
			ret = send(id_socket,aux,(int)longitud, 0);
        break;

		case IPPROTO_UDP:

			memset(&envaddr, 0, sizeof(struct sockaddr));

            //configuramos los datos a los que vamos a enviar
            envaddr.sin_family = AF_INET;
            envaddr.sin_addr   = tpdu.server;
            envaddr.sin_port   = tpdu.port;

            //enviamos con la funcion send to
			ret = sendto(id_socket, aux,(int)longitud, 0, (struct sockaddr *)&envaddr, sizeof(struct sockaddr));
        break;

		default:
			fprintf(stderr,"\nProtocol type not supported\n");
			return -1;
        break;
	}

    //devolvemos el return de las funciones send/sendto
	return ret;
}

ssize_t Recibir (int id_socket, void *mensaje, size_t longitud, TPDU *tpdu)
{

    struct sockaddr_in recvaddr;
	socklen_t sockettam;
	int ret=0;
	char *aux = NULL;

    //casteamos a char el mensaje
    aux = ((char*)mensaje);

    //dependiendo del protocolo haremos uan cosa u otra
	switch (tpdu->protocol)
	{
		case IPPROTO_TCP:
            //recibimos con la funcion recv
			ret = recv(id_socket,aux,(int)longitud,0);
			//ya que esta funcion no acepta los '\0', se lo añadimos manualmente
			aux[ret] = '\0';
        break;

		case IPPROTO_UDP:
            //necesitamos esta variables para guardar los datos del cliente para poder responderle, la funcion recvfrom los obtendra y los guardara aqui
			memset ((char *)&recvaddr,0, sizeof(struct sockaddr_in));

			sockettam = sizeof(struct sockaddr_in);

            //recibimos los datos con la funcion recvfrom
			ret = recvfrom(id_socket,aux,(int)longitud, 0, (struct sockaddr *)&recvaddr, &sockettam);
			//ya que esta funcion no acepta los '\0', se lo añadimos manualmente
			aux[ret] ='\0';

			//aqui obtendra los datos del cliente para responderle
			if (ret >= 0)
			{
				tpdu->server = recvaddr.sin_addr;
				tpdu->port = recvaddr.sin_port;
			}
        break;

		default:
			fprintf(stderr,"\nProtocol type not supported\n");
			return -1;
        break;
	}
    //devolvemos el return de las funciones send/sendto
	return ret;
}

