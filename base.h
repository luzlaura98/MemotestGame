#ifndef BASE_H
#define BASE_H

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>
#include <signal.h>

#define FILA_COL 4
#define TAM_TIEMPO 20

#define CANT_CASILLAS FILA_COL *FILA_COL

#define NOMBRE_MATRIZ_COMP "miMatriz"
#define NOMBRE_JUGADA "turnoCliente"
#define SEM_LLEGA_CLIENTE "llegaCliente"
#define SEM_NUEVA_JUGADA "nuevaJUgada"
#define SEM_TABLERO_LISTO "tableroListo"
#define PID_SERVIDOR "pidServidor"

#define AYUDA_MEMOTEST "------------------Ayuda------------------\n\
-Sinopsis:\n\
\tEs el clásico juego de la memoria “Memotest”, pero alfabético.\n\
\tSe creará dos procesos no emparentados que utilizan memoria compartida y se sincronizan con semáforos.\n\n\
-Parámetros:\n\
\tNo posee.\n\n\
-Como usar:\n\
\t1.Deberá ejecutar el proceso “cliente”, cuya tarea será mostrar por pantalla el estado actual del tablero y leer desde teclado el par de casillas que el usuario quiere destapar.\n\
\t2.También deberá ejecutar el proceso “servidor”, que será el encargado de actualizar el estado del tablero en base al par de casillas ingresado, así como controlar la finalización partida."

struct turnoServidor
{
	char tablero[FILA_COL][FILA_COL];
	char tiempo[TAM_TIEMPO];
	int nro;
	int paresRestantes;
};

struct turnoCliente
{
	int fila;
	int colum;
};

#endif