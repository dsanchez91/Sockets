/*
 ** Fichero: transporte.h
 ** Autores:
 ** Alvaro Valiente Herrero 
 ** Diego Sanchez Moreno
 ** Usuario: i8112600
 */

#ifndef TRANSPORTE_H_INCLUDED
#define TRANSPORTE_H_INCLUDED

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <stropts.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN    2000
#define PORT 2600
#define CLIENT 1
#define SERVER 0


typedef struct TPDU
{
    struct  in_addr  server;
    int protocol;
    unsigned short int port;
}TPDU;


int crearSocket (TPDU tpdu, int rol);
ssize_t Enviar (int id_socket, const void *mensaje, size_t longitud, TPDU tpdu);
ssize_t Recibir (int id_socket, void *mensaje, size_t longitud, TPDU *tpdu);

#endif
