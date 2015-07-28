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

    if(rol==SERVER)//SERVIDOR
    {
        /* Escoger la familia de direcciones */
        servaddr_in.sin_family = AF_INET;

        /* Escoger alguna dirección de red local de las disponibles */
        servaddr_in.sin_addr.s_addr = INADDR_ANY;

        /* Escoger el puerto para ofrecer el servicio */
        servaddr_in.sin_port = tpdu.port;

        if(tpdu.protocol == IPPROTO_TCP)//protocolo TCP
        {
            /* Crear el socket TCP */
            stcp = socket(servaddr_in.sin_family, SOCK_STREAM, 0);
            if (stcp == -1) {

                fprintf(stderr,"Unable to create datagram socket\n");
                return -1;
            }

            /* Asociar el socket TCP en la dirección de red local y puerto */
            if (bind(stcp, (const struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
                fprintf(stderr,"Unable to bind address (stcp)\n");
                return -1;
            }

            /* Crear una cola de escucha para el socket TCP */
            if (listen(stcp, 6) == -1 ) {
                fprintf(stderr, "Unable to listen \n");
                return -1;
            }

            return stcp;

        }

        if(tpdu.protocol == IPPROTO_UDP)
        {
            /* Crear el socket UDP */
            sudp = socket(servaddr_in.sin_family, SOCK_DGRAM, 0);
            if (sudp == -1) {
                fprintf(stderr, "Unable to create datagram socket\n");
                return -1;
            }

            /* Asociar el socket UDP en la dirección de red local y puerto */
            if (bind(sudp, (const struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
                fprintf(stderr, "Unable to bind address (sudp)\n");
                return -1;
            }

            return sudp;
        }

    }

    if(rol==CLIENT)//CLIENTE
    {
        /* Set up the peer address to which we will connect. */
        peeraddr_in.sin_family = AF_INET;
            /* Get the host information for the hostname that the
             * user passed in.
             */
        peeraddr_in.sin_addr= tpdu.server;
            /* Find the information for the "example" server
             * in order to get the needed port number.
		 */
        peeraddr_in.sin_port = tpdu.port;

        if(tpdu.protocol == IPPROTO_TCP)//protocolo TCP
        {
                /* Create the socket. */
                stcp = socket (peeraddr_in.sin_family, SOCK_STREAM, 0);
                if (stcp == -1) {
                    fprintf(stderr, "Unable to create socket\n");
                    return -1;
                }
                /* Try to connect to the remote server at the address
                * which was just built into peeraddr.
                */
                if (connect(stcp, (const struct sockaddr *)&peeraddr_in, sizeof(struct sockaddr_in)) == -1) {
                    fprintf(stderr, "Unable to connect to remote\n");
                    return -1;
                }

                return stcp;
        }

        if(tpdu.protocol == IPPROTO_UDP)
        {
                /* Create the socket. */
                sudp = socket (peeraddr_in.sin_family, SOCK_DGRAM, 0);
                if (sudp == -1)
                {
                    fprintf(stderr, "Unable to create socket\n");
                    return -1;
                }
                /* Bind socket to some local address so that the
                * server can send the reply back.  A port number
                * of zero will be used so that the system will
                * assign any available port number.  An address
                * of INADDR_ANY will be used so we do not have to
                * look up the internet address of the local host.
                */
                myaddr_in.sin_family = AF_INET;
                myaddr_in.sin_port = 0;
                myaddr_in.sin_addr.s_addr = INADDR_ANY;

                if (bind(sudp, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1)
                {
                    fprintf(stderr, "Unable to bind socket\n");
                    return -1;
                }

                return sudp;
        }
    }
    return -1;
}


ssize_t Enviar (int id_socket, const void *mensaje, size_t longitud, TPDU tpdu)
{

    struct sockaddr_in envaddr;
	size_t bitsend=0;
	char *aux = NULL;

	int ret;

	aux = ((char*)mensaje);

	switch (tpdu.protocol)
	{
		case IPPROTO_TCP:
			ret = send(id_socket,aux,(int)longitud, 0);
			break;

		case IPPROTO_UDP:

			memset(&envaddr, 0, sizeof(struct sockaddr));

            envaddr.sin_family = AF_INET;
            envaddr.sin_addr   = tpdu.server;
            envaddr.sin_port   = tpdu.port;

			ret = sendto(id_socket, aux,(int)longitud, 0, (struct sockaddr *)&envaddr, sizeof(struct sockaddr));
			break;

		default:
			fprintf(stderr,"Protocol type not supported\n");
			return -1;
	}

	return ret;

}

ssize_t Recibir (int id_socket, void *mensaje, size_t longitud, TPDU *tpdu)
{

    struct sockaddr_in recvaddr;
	socklen_t sockettam;
	int ret=0;
	char *aux = NULL;


    aux = ((char*)mensaje);

	switch (tpdu->protocol)
	{
		case IPPROTO_TCP:

			ret = recv(id_socket,aux,(int)longitud,0);
			aux[ret] = '\0';
			break;

		case IPPROTO_UDP:

            //necesitamos esto para guardar los datos del cliente para poder responderle, la funcion recvfrom los obtendra y los guardara aqui
			memset ((char *)&recvaddr,0, sizeof(struct sockaddr_in));

			sockettam = sizeof(struct sockaddr_in);

			ret = recvfrom(id_socket,aux,(int)longitud, 0, (struct sockaddr *)&recvaddr, &sockettam);
			aux[ret] ='\0';

			//aqui obtendra los datos del cliente para responderle
			if (ret >= 0)
			{
				tpdu->server = recvaddr.sin_addr;
				tpdu->port = recvaddr.sin_port;
			}

			break;

		default:
			fprintf(stderr,"Protocol type not supported\n");
			return -1;
	}

	return ret;
}

