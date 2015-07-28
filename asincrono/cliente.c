/*
 ** Fichero: cliente.c
 ** Autores:
 ** Alvaro Valiente Herrero 
 ** Diego Sanchez Moreno 
 ** Usuario: i8112600
 */

#include "transporte.h"

int writeAnswers(int f,char *protocol,char *hostname,int kport,int eport, char *ip,int error,int mode,char *data);

int main (int argc,char* argv[])
{
    TPDU parameters;
    int stcp,sudp;
    char data[BUFLEN];
    struct hostent *hp;
    int fd;
    int protocol;
    sigset_t mask;
    int ret;
    int efiport;
    int addrlen;
    struct sockaddr_in myaddr_in;
    int fp;
    char titulo[50]="";

    //Comprobracion de parametros de entrada
    if(argc != 4)
    {
        fprintf(stderr,"\nError! How to use it: %s [hostname] [protocol TCP|UDP] [ordenes.txt|ordenes1.txt|ordenes2.txt]\n",argv[0]);
    }
    else
    {
        if(strcmp(argv[2],"UDP") != 0 && strcmp(argv[2],"TCP") != 0)
        {
            fprintf(stderr,"\nError! The protocol has to be TCP or UDP\n");
        }

        if(strcmp(argv[3],"ordenes.txt") != 0 && strcmp(argv[3],"ordenes1.txt") != 0 && strcmp(argv[3],"ordenes2.txt") != 0 )
        {
            fprintf(stderr,"\nError! The archive has to be ordenes.txt or ordenes1.txt or ordenes2.txt \n");
        }
    }

    //Comprobamos que protocolo es
	if(strcmp(argv[2],"UDP") == 0)
	{
        protocol = IPPROTO_UDP;
	}
	else
	{
	    protocol = IPPROTO_TCP;
	}

	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));

    //Obtenemos los datos del host
    hp = gethostbyname (argv[1]);
    if (hp == NULL)
    {
        fprintf(stderr, "%s not found in /etc/hosts\n",argv[1]);
        return 1;
    }

    //Rellenamos los parametros de configuracion
    if(protocol == IPPROTO_UDP)
    {
        parameters.protocol = IPPROTO_UDP;
    }
    else
    {
        parameters.protocol = IPPROTO_TCP;
    }

    parameters.port = htons(PORT);
    parameters.server = *((struct in_addr *)(hp->h_addr));

    //creamos el socket dependiendo del protocolo
    if(protocol == IPPROTO_UDP)
    {
        //creamos el socket UDP
        sudp = crearSocket(parameters,CLIENT);
		if(sudp == -1)
		{
		    return 1;
		}

        //aqui obtenemos la informacion del socket, buscamos el puerto efimero
		addrlen = sizeof(struct sockaddr_in);
        if(getsockname(sudp, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
        {
            fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
            exit(1);
        }
        efiport = myaddr_in.sin_port;
    }
    else
    {
        //creamos el socket TCP
		stcp = crearSocket(parameters,CLIENT);
		if(stcp == -1)
		{
		    return 1;
		}

		//aqui obtenemos la informacion del socket, buscamos el puerto efimero
		addrlen = sizeof(struct sockaddr_in);
        if(getsockname(stcp, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
        {
            fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
            exit(1);
        }
        efiport = myaddr_in.sin_port;
    }

    //abrimos el fichero que hemos pasado por parametros para lectura
    fd = fopen(argv[3],"r");
    if(fd==NULL)
    {
        fprintf(stderr,"\nCan't open text file\n");
        return 1;
    }

    //nombramos al fichero con el nombre del puerto efimero
    sprintf(titulo,"%d.txt",efiport);

    //abrimos el fichero para el log
    fp = fopen(titulo, "a");
    if(fp==NULL)
    {
        fprintf(stderr,"\nCan't open log file\n");
        return 1;
    }

    //recorrer el fichero e ir enviado y recibiendo los datos contenidos en el
    while(1)
    {
        //obtenemos una frase del fichero
        fgets(data,BUFLEN,fd);

        //protocolo UDP
        if(protocol == IPPROTO_UDP)
        {
            //enviamos la frase al servidor
            ret =Enviar(sudp,data,sizeof(data),parameters);

            //comprobamos si hay problemas en el envio
            if(ret < 0)
            {
                writeAnswers(fp,"UDP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),1,0,data);
            }
            else
            {
                //comprobamos si enviamos la palabra FIN para finalizar el cliente
                if(strcmp(data,"FIN\n") == 0 || strcmp(data,"FIN") == 0)
                {
                    close(sudp);
                    close(fd);
                    close(fp);
                    return 0;
                }

                writeAnswers(fp,"UDP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),0,0,data);

                //recibimos los datos
                ret =Recibir(sudp,&data,sizeof(data),&parameters);

                //comprobamos si se ha producido algun error en el envio
                if(ret < 0)
                {
                    writeAnswers(fp,"UDP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),1,1,data);

                }
                else
                {
                    writeAnswers(fp,"UDP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),0,1,data);
                }
            }
        }

        //protocolo TCP
        if(protocol == IPPROTO_TCP)
        {
            //enviamos los datos
            ret =Enviar(stcp,data,sizeof(data),parameters);

            //comprobamos si se ha producido algun error en el envio
            if(ret < 0)
            {
                writeAnswers(fp,"TCP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),1,0,data);
            }
            else
            {
                //comprobamos si enviamos FIN para finalizar el cliente
                if(strcmp(data,"FIN\n") == 0 || strcmp(data,"FIN") == 0)
                {
                    close(sudp);
                    close(fd);//fichero del cual obtenemos los datos
                    close(fp);//fichero del log
                    return 0;
                }

                writeAnswers(fp,"TCP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),0,0,data);

                //recibimos los datos
                ret =Recibir(stcp,&data,sizeof(data),&parameters);

                //comprobamos si se ha producido algun error en la recepcion
                if(ret < 0)
                {
                    writeAnswers(fp,"TCP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),1,1,data);
                }
                else
                {
                    writeAnswers(fp,"TCP",hp->h_name,ntohs(parameters.port),ntohs(efiport),inet_ntoa(parameters.server),0,1,data);
                }
            }
        }
    }
}

int writeAnswers(int f,char *protocol,char *hostname,int kport,int eport, char *ip,int error,int mode,char *data)
{

    char fecha[128];
    time_t tiempo = time(0);
    //obtenemos el tiempo local
    struct tm *tlocal = localtime(&tiempo);

    //formateamos la fecha y hora
    strftime(fecha,128,"%d/%m/%y %H:%M:%S",tlocal);

    switch(error)
    {
        case 1://Error

            switch(mode)
            {
                case 0://envio
                    fprintf(f, "\n%s - %s client (Hostname: %s Ip: %s Know-Port: %d Ephemeral-Port: %d ): Error sending data",fecha,protocol,hostname,ip,kport,eport);
                break;

                case 1://recepcion
                    fprintf(f, "%s - %s client (Hostname: %s Ip: %s Know-Port: %d Ephemeral-Port: %d ): Error receiving data\n",fecha,protocol,hostname,ip,kport,eport);
                break;
            }

        break;

        case 0://normal

            switch(mode)
            {
                case 0://envio
                    fprintf(f, "\n%s - %s client (Hostname: %s Ip: %s Know-Port: %d Ephemeral-Port: %d ): Data sent: %s",fecha,protocol,hostname,ip,kport,eport,data);
                break;

                case 1://recepcion
                    fprintf(f, "%s - %s client (Hostname: %s Ip: %s Know-Port: %d Ephemeral-Port: %d ): Data received: %s\n",fecha,protocol,hostname,ip,kport,eport,data);
                break;
            }

        break;
    }

    return 0;
}


    //envio y recepcion de datos
    /*while(1)
    {
       printf("\nType a text to send: ");
       fgets(data,BUFLEN,stdin);

       if(protocol == IPPROTO_UDP)
       {
            ret =Enviar(sudp,data,sizeof(data),parameters);
            if(ret < 0)
            {
                fprintf(stderr,"\nError sending data. Please type again the text");

            }
            else
            {
                if(strcmp(data,"FIN\n") == 0 || strcmp(data,"FIN") == 0)
                {
                    printf("\nConnection off\n");
                    close(sudp);
                    return 0;
                }

                printf("\n[Client UDP to %s(%s):%d] I've send: %s",hp->h_name,inet_ntoa(parameters.server),ntohs(parameters.port),data);

                ret =Recibir(sudp,&data,sizeof(data),&parameters);
                if(ret < 0)
                {
                    fprintf(stderr,"\nError receiving data. Please type again the text");
                }
                else
                {
                     printf("\n[Client UDP from %s(%s):%d] I've receive: %s",hp->h_name,inet_ntoa(parameters.server),ntohs(parameters.port),data);
                }
            }
        }
        else
        {
            ret =Enviar(stcp,data,sizeof(data),parameters);
            if(ret < 0)
            {
                fprintf(stderr,"\nError sending data. Please type again the text");
            }
            else
            {
                if(strcmp(data,"FIN\n") == 0 || strcmp(data,"FIN") == 0)
                {
                    printf("\nConnection off\n");
                    close(sudp);
                    return 0;
                }

                printf("\n[Client TCP to %s(%s):%d] I've send: %s",hp->h_name,inet_ntoa(parameters.server),ntohs(parameters.port),data);

                ret =Recibir(stcp,&data,sizeof(data),&parameters);

                if(ret < 0)
                {
                    fprintf(stderr,"\nError receiving data. Please type again the text");
                }
                else
                {
                    printf("\n[Client TCP from %s(%s):%d] I've receive: %s",hp->h_name,inet_ntoa(parameters.server),ntohs(parameters.port),data);
                }

            }
        }
    }*/

    //comprobacion de parametros
    /*if(argc != 3)
    {
        fprintf(stderr,"\nError! How to use it: %s [hostname] [protocol TCP|UDP]\n",argv[0]);
        return 1;
    }
    else
    {
        if( (strcmp(argv[2],"UDP") != 0 && strcmp(argv[2],"TCP") != 0) )
        {
            fprintf(stderr,"\nError! The protocol has to be TCP or UDP\n");
            return 1;
        }
    }*/

