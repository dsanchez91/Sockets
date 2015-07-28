/*
 ** Fichero: sincrono.c
 ** Autores:
 ** Alvaro Valiente Herrero 
 ** Diego Sanchez Moreno
 ** Usuario: i8112600
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stropts.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "transporte.h"
#include <sys/file.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "transporte.h"

struct sockaddr_in peeraddr_in;  /* remote socket address */
int stcp,sudp;//sockets
TPDU parameters_tcp,parameters_udp,parameterstcp_log;//estructura con los datos de la conexion
int pidTCP;//guardaremos el pid del proceso que se encarga de la parte de TCP


int syncServer(int argc, char*argv[]);//funcion principal del servidor
void EspecializateSyncServerTCP();//funcion de especializacion en servidor TCP
void EspecializateSyncServerUDP();//funcion de especializacion en servidor TCP
void morse(char cIn,char *cOut);//traduce un caracter a morse
void translate_morse(char *in,char*out);//traduce una cadena a morse
int writeLog(char *protocol,TPDU tpdu, int sent,int translated);//escribe un log
int imtcp(int s);//funcion que realiza las funciones de "conversacion" TCP
int imtcp(int s);//funcion que realiza las funciones de "conversacion" UDP

//funcion que se ejecuta cuando le llega la señal SIGTERM y que finaliza el servidor
void finalizar()
{
    //cerramos los sockets
    close(stcp);
    close(sudp);

    printf("\nEl servidor sincrono termino correctamente\n");

    //matamos al hijo que se encarga de la parte de TCP
    kill(pidTCP,SIGTERM);

    //salimos del proceso
    exit(0);
}


int main(int argc, char *argv[])//en esta funcion hacemos el proceso daemon
{
    setpgrp();

	switch (fork())
	{
		case -1:
			fprintf(stderr, "\n%s: unable to fork daemon\n", argv[0]);
			exit(1);

		case 0:
			syncServer(argc,argv);
			break;
		default:

			break;
	}
	return 0;
}


int syncServer(int argc, char*argv[])
{
    struct sigaction vec2;
    //Registrar SIGTERM para la finalizacion ordenada del programa servidor
    vec2.sa_handler = (void *) finalizar;
    vec2.sa_flags = 0;
    if( sigaction(SIGTERM, &vec2, (struct sigaction *) 0) == -1)
    {
        perror(" sigvector(SIGTERM)");
        fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
        return -1;
    }

    switch (fork())
	{
		case -1:
			fprintf(stderr, "\n%s: unable to fork daemon\n", argv[0]);
			exit(1);

		case 0:
            //el hijo pasa a ser TCP
            pidTCP = getpid();//guardamos el pid del hijo para luego, cuando el padre muera, muera tambien el hijo
			EspecializateSyncServerTCP();
			break;
		default:
            //el padre continua su funcion como UDP
            EspecializateSyncServerUDP();
			break;
	}
}

void EspecializateSyncServerTCP()
{
    int addrlen;
    int stcpnew;

    //configurar servidor TCP
    parameters_tcp.protocol   = IPPROTO_TCP;
    parameters_tcp.port = htons(PORT);
    parameters_tcp.server.s_addr = htonl(INADDR_ANY);

    //creamos el socket TCP
    stcp=crearSocket(parameters_tcp,SERVER);
    if (stcp == -1)
    {
        fprintf(stderr, "\nUnable to create socket\n");
        exit(1);
    }

    //este bucle se estara ejecutando siempre y esperara por la peticion de dialogo de un cliente
    while(1)
    {
        addrlen = sizeof(struct sockaddr_in);

        //obtenemos el socket para la nueva conexion
        //aqui obtenemos la direccion del cliente con el que conectamos
        stcpnew = accept(stcp, (struct sockaddr *) &peeraddr_in, &addrlen);
        if (stcpnew == -1 )
        {
            fprintf(stderr,"\nAccept call failed\n");
            exit(1);
        }

        //Creamos un hijo que se encargara del dialogo con ese cliente
        switch(fork())
        {
            case -1:
                fprintf(stderr,"\nError fork\n");
            break;

            case 0:
                close(stcp);
                imtcp(stcpnew);
            break;

            default:
                close(stcpnew);
            break;
        }
    }
}


void EspecializateSyncServerUDP()
{
    char datos[BUFLEN];
    char datosmorse[BUFLEN];
    int ret;
    int sudpnew;

    //configurar servidor UDP
    parameters_udp.protocol   = IPPROTO_UDP;
    parameters_udp.port = htons(PORT);
    parameters_udp.server.s_addr = htonl(INADDR_ANY);

    //creamos el socket UDP
    sudp=crearSocket(parameters_udp,SERVER);
    if (stcp == -1)
    {
        fprintf(stderr, "\nUnable to create socket\n");
        exit(1);
    }

    //este bucle estara siempre ejecutandose esperando por las peticiones de dialogo de los clientes
    while(1)
    {
        //Necesitamos realizar una primera recepcion de los datos para conocer la direccion y el puerto del cliente
        ret=Recibir(sudp,&datos,sizeof(datos),&parameters_udp);

        //comprobamos que no haya problemas en la recepcion
        if(ret < 0)
        {
            fprintf(stderr,"\nError receiving data\n");
            fflush(stdout);
        }
        else
        {
            //comprobamos que no nos llegue la palabra FIN
            if(strcmp(datos,"FIN\n") == 0 || strcmp(datos,"FIN") == 0)
            {
                writeLog("UDP",parameters_udp,0,0);
            }
            else
            {
                //la ip esta en peeraddr_in.sin_addr y viene de la funcion accept

                /*printf("\n[Server UDP from %s:%d] I've receive: %s",inet_ntoa(parameters_udp.server),ntohs(parameters_udp.port),datos);
                fflush(stdout);*/

                //traducimos a morse
                translate_morse(datos,datosmorse);

                /*
                Creamos el nuevo socket para la nueva peticion (gracias a esto podremos llevar cuenta del numero de peticiones
                de cada cliente en particlar, si no lo hicieramos asi, no podriamos llevar esa cuenta. Por tanto estamos
                imitando de cierta manera el comportamiento de TCP.

                Si no quisiesemos llevar la cuenta no haria falta crear un socket nuevo, con el mismo socket atenderiamos
                todas las nuevas peticiones
                */
                sudpnew = crearSocket(parameters_udp,CLIENT);

                //enviamos por el nuevo socket
                ret=Enviar(sudpnew,datosmorse,sizeof(datosmorse),parameters_udp);

                //comprobamos que no haya problemas en el envio
                if(ret < 0)
                {
                    fprintf(stderr,"\nError sending data\n");
                    fflush(stdout);
                }
                /*else
                {
                    printf("\n[Server UDP to %s:%d] I've send: %s",inet_ntoa(parameters_udp.server),ntohs(parameters_udp.port),datosmorse);
                    fflush(stdout);
                }*/
            }
        }

        /*
        Ahora si creamos un nuevo hijo que se encargara de mantener esa "conversacion" con el cliente.
        Antes no hemos podido hacerlo porque hasta que no hemos recibido la informacion no conociamos los datos
        del cliente, una vez conocidos, delegamos el trabajo de "conversacion" a un hijo, como en TCP,
        */
        switch(fork())
        {
            case -1:
                fprintf(stderr,"\nError fork\n");
            break;
            case 0:
                close(sudp);
                imudp(sudpnew);
            break;

            default:
                close(sudpnew);
            break;
        }
    }
}

int imtcp(int s)
{
    char datos[BUFLEN];
    char datosmorse[BUFLEN];
    int ret;
    int translatedTCP = -1;
    int sentTCP = -1;

    //creamos una estructura tpdu nueva para escribir en el log con los datos del cliente en TCP
    parameterstcp_log.server = peeraddr_in.sin_addr;
    parameterstcp_log.port = parameters_tcp.port;

    //escribimos en el log que se ha producido una nueva conexion(las variables a -1 controlan eso)
    writeLog("TCP",parameterstcp_log,sentTCP, translatedTCP);

    //inicializamos variables
    sentTCP = 0;
    translatedTCP = 0;

    //este sera el bucle de envio y recepcion de informacion
    while(1)
    {
        //recibimos los datos
        ret=Recibir(s,&datos,sizeof(datos),&parameters_tcp);

        //comprobamos que no haya habido ningun problema con la recepcion
        if(ret < 0)
        {
            fprintf(stderr,"\nError receiving data\n");
            fflush(stdout);
        }
        else
        {
            //si nos llega la palabra fin, finalizamos la conexion
            if(strcmp(datos,"FIN\n") == 0 || strcmp(datos,"FIN") == 0)
            {
                writeLog("TCP",parameterstcp_log,sentTCP, translatedTCP);
                close(s);
                exit(0);
            }
            else
            {
                //la ip esta en peeraddr_in.sin_addr y viene de la funcion accept

                /*printf("\n[Server TCP from %s:%d] I've receive: %s\n",inet_ntoa(peeraddr_in.sin_addr),ntohs(parameters_tcp.port),datos);
                fflush(stdout);*/

                //traducimos a morse
                translate_morse(datos,datosmorse);
                translatedTCP++;

                //enviamos los datos traducidos
                ret=Enviar(s,datosmorse,sizeof(datosmorse),parameters_tcp);

                //comprobamos que no se hayan producido problemas en el envio
                if(ret < 0)
                {
                    fprintf(stderr,"\nError sending data\n");
                    fflush(stdout);
                }
                else
                {
                    sentTCP++;
                    /*printf("\n[Server TCP to %s:%d] I've send: %s\n",inet_ntoa(peeraddr_in.sin_addr),ntohs(parameters_tcp.port),datosmorse);
                    fflush(stdout);*/
                }
            }
        }
    }
}


int imudp(int s)
{
    char datos[BUFLEN];
    char datosmorse[BUFLEN];
    int ret;
    int translatedUDP = 1;
    int sentUDP = 1;

    //indicamos que se ha producido una nueva conexion (las variables a -1 controlan eso)
    writeLog("UDP",parameters_udp,-1,-1);

    //bucle que se ocupa de la recepcion y envio de datos
    while(1)
    {
        //recibimos los datos
        ret=Recibir(s,&datos,sizeof(datos),&parameters_udp);

        //comprobamos que no se haya producido ningun error en la recepcion
        if(ret < 0)
        {
            fprintf(stderr,"Error receiving data\n");
            fflush(stdout);
        }
        else
        {
            //si nos llega la palabra FIN, finalizamos la conexion
           if(strcmp(datos,"FIN\n") == 0 || strcmp(datos,"FIN") == 0)
            {
                writeLog("UDP",parameters_udp,sentUDP, translatedUDP);
                close(s);
                exit(0);
            }
            else
            {
                //la ip esta en parameters_udp.server y viene de la funciona recibir
                /*printf("\n[Server UDP from %s:%d] I've receive: %s\n",inet_ntoa(parameters_udp.server),ntohs(parameters_udp.port),datos);
                fflush(stdout);*/

                //traducimos a morse
                translate_morse(datos,datosmorse);
                translatedUDP++;

                //enviamos los datos traducidos
                ret=Enviar(s,datosmorse,sizeof(datosmorse),parameters_udp);

                //comprobamos si se ha producido algun error en el envio
                if(ret < 0)
                {
                    fprintf(stderr,"\nError sending data\n");
                    fflush(stdout);
                }
                else
                {
                    sentUDP++;
                    /*printf("\n[Server UDP to %s:%d] I've send: %s\n",inet_ntoa(parameters_udp.server),ntohs(parameters_udp.port),datosmorse);
                    fflush(stdout);*/
                }
            }
        }
    }
}


void translate_morse(char *in,char*out)
{
    int i,j,k=0;
    char morseaux[8];

    for(i=0;i<strlen(in)-2;i++)
    {
        morse(in[i],morseaux);

        for(j=0;j<strlen(morseaux);j++)
        {
            out[k] = morseaux[j];
            k++;
        }

        out[k]= '#';
        k++;
    }
    out[k] = '\0';
}

void morse(char cIn,char *cOut)
{
   char code[8];

  switch(toupper(cIn))
  {
  // Letter Morse
  case 'A': strcpy(code, ".-");  break;
  case 'B': strcpy(code, "-..."); break;
  case 'C': strcpy(code, "-.-."); break;
  case 'D': strcpy(code, "-..");  break;
  case 'E': strcpy(code, ".");  break;
  case 'F': strcpy(code, "..-."); break;
  case 'G': strcpy(code, "--.");  break;
  case 'H': strcpy(code, "...."); break;
  case 'I': strcpy(code, "..");  break;
  case 'J': strcpy(code, "-.-."); break;
  case 'K': strcpy(code, "-.-");  break;
  case 'L': strcpy(code, ".-.."); break;
  case 'M': strcpy(code, "--");  break;
  case 'N': strcpy(code, "-.");  break;
  case 'O': strcpy(code, "---");  break;
  case 'P': strcpy(code, ".--."); break;
  case 'Q': strcpy(code, "--.-"); break;
  case 'R': strcpy(code, ".-.");  break;
  case 'S': strcpy(code, "...");  break;
  case 'T': strcpy(code, "-");  break;
  case 'U': strcpy(code, "..-");  break;
  case 'V': strcpy(code, "...-"); break;
  case 'W': strcpy(code, ".--");  break;
  case 'X': strcpy(code, "-..-"); break;
  case 'Y': strcpy(code, "-.--"); break;
  case 'Z': strcpy(code, "--.."); break;

  // Digit Morse
  case '0': strcpy(code, "-----"); break;
  case '1': strcpy(code, ".----"); break;
  case '2': strcpy(code, "..---"); break;
  case '3': strcpy(code, "...--"); break;
  case '4': strcpy(code, "....-"); break;
  case '5': strcpy(code, "....."); break;
  case '6': strcpy(code, "-...."); break;
  case '7': strcpy(code, "--..."); break;
  case '8': strcpy(code, "---.."); break;
  case '9': strcpy(code, "----."); break;

  // Others
  case '.': strcpy(code, ".-.-.-"); break;
  case ',': strcpy(code, "--..--"); break;
  case '?': strcpy(code, "..--.."); break;

    }
    strcpy(cOut,code);
}

int writeLog(char *protocol,TPDU tpdu, int sent,int translated)
{

    FILE *fp=NULL;
    time_t tiempo = time(0);

    //obtenemos el tiempo local
    struct tm *tlocal = localtime(&tiempo);
    char fecha[128];
    char *hostname;
    struct hostent *host = NULL;
    int port;


    //formateamos la fecha y hora
    strftime(fecha,128,"%d/%m/%y %H:%M:%S",tlocal);

    host=gethostbyaddr((char*)&tpdu.server, sizeof(struct in_addr), AF_INET);
    if(host == NULL)
    {
        hostname = inet_ntoa(tpdu.server);
    }
    else
    {
        hostname = host->h_name;
    }

    //formateamos el puerto
    port = ntohs(tpdu.port);

    //abrimos el fichero para añadir
	fp = fopen("log.txt", "a");
    if(fp==NULL)
    {
        fprintf(stderr,"No se puede abrir el archivo");
        return 1;
    }

    //si es la primera peticion sera un cliente nuevo
    if(sent==-1)
    {
        fprintf(fp, "\n%s - (New Client) Host: %s Ip: %s Protocol: %s Port: %d \n",
                fecha,hostname,inet_ntoa(tpdu.server),protocol,port);
    }
    else//si no, es que el cliente ha cerrado la comunicacion
    {
        fprintf(fp, "\n%s - (Finished Client) Host: %s Ip: %s Protocol: %s Port: %d Petitions from this client: %d. Traductions for this client: %d \n",
                fecha,hostname,inet_ntoa(tpdu.server),protocol,port,sent,translated);
    }

    //cerramos el fichero
    fclose(fp);
    return 0;
}

