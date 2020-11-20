#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <list>
#include <cstring> //memset

#define FILA_COL 4
#define TAM_TIEMPO 20

#define CANT_CASILLAS FILA_COL *FILA_COL

#define NOMBRE_MATRIZ_COMP "miMatriz"
#define NOMBRE_JUGADA "turnoCliente"
#define SEM_LLEGA_CLIENTE "llegaCliente"
#define SEM_NUEVA_JUGADA "nuevaJUgada"
#define SEM_TABLERO_LISTO "tableroListo"

/**
 * Falta:
 * [ ]validar que sean números el ingreso de fila y colum del cliente y esté entre [0,CANT_CASILLAS]
 * [.]mostrar tiempo transcurrido
 * [.]borrar lo compartido en memoria al finalizar.
 * [ ]ignorar SIGINT
 * [ ]finalizar por la señal SIGUSR1 (servidor)
 * [.]arreglar en el cliente que agarre el fin de partida al final del while
 * [ ]controlar que si elije una casilla que ya está destapada pida de nuevo el numero
*/

using namespace std;

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

sem_t *tableroListo;
sem_t *nuevaJugada;

char tableroRes[FILA_COL][FILA_COL];
turnoServidor turnoShared;
struct turnoServidor *rptr;
struct turnoCliente *turnoCliente;
bool aciertos = 0;
time_t secondsInicio;

void iniciarPartida()
{
	//Lleno una lista con las casillas con las que
	//se va a llenar la matriz.
	//8 letras mayúsculas, cada letra debe aparecer 2 veces
	list<char> letras;
	char letra = 'A';
	for (int i = 0; i < FILA_COL * 2; i++)
	{
		letras.push_back(letra);
		letras.push_back(letra++);
	}

	turnoShared.nro = 1;
	secondsInicio = time(NULL);
	turnoShared.paresRestantes = CANT_CASILLAS / 2;

	//Lleno la matriz con la lista de letras aleatoriamente
	//mientras elimino las letras que ya se usaron. [No puntero]
	cout << "\t0\t1\t2\t3" << endl;
	for (int i = 0; i < FILA_COL; ++i)
	{
		cout << i << "\t";
		for (int j = 0; j < FILA_COL; ++j)
		{
			int posRandom = rand() % letras.size(); //desde 0 a size-1
			// Iterator apuntando al primer elemento
			list<char>::iterator it = letras.begin();
			advance(it, posRandom);
			cout << *it << '\t';
			turnoShared.tablero[i][j] = '-';
			tableroRes[i][j] = *it;
			letras.erase(it);
		}
		cout << endl;
	}
	int fd = shm_open(NOMBRE_MATRIZ_COMP, O_CREAT | O_RDWR, 0600); //0600
	if (fd == -1)
	{
		cout << "shm_open error" << endl;
		exit(EXIT_FAILURE);
	}
	if (ftruncate(fd, sizeof(struct turnoServidor)) == -1)
	{
		cout << "ftruncate error" << endl;
		exit(EXIT_FAILURE);
	}
	rptr = (struct turnoServidor *)mmap(NULL, sizeof(struct turnoServidor),
										PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (rptr == MAP_FAILED)
	{
		cout << "mmap error" << endl;
		exit(EXIT_FAILURE);
	}
	close(fd);
	memcpy(rptr, &turnoShared, sizeof(struct turnoServidor));
	snprintf(rptr->tiempo,sizeof(TAM_TIEMPO), "%.1f\r\n",difftime(time(NULL),secondsInicio));
}

void cargarTurnoClienteComp()
{
	/*fileDescriptor*/ int fd = shm_open(NOMBRE_JUGADA, O_CREAT | O_RDWR, 0600); //0600
	if (fd == -1)
	{
		cout << "shm_open error" << endl;
		exit(EXIT_FAILURE);
	}

	if (ftruncate(fd, sizeof(struct turnoCliente)) == -1)
	{
		cout << "ftruncate error" << endl;
		exit(EXIT_FAILURE);
	}

	turnoCliente = (struct turnoCliente *)mmap(NULL, sizeof(struct turnoCliente),
											   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (turnoCliente == MAP_FAILED)
	{
		cout << "mmap error" << endl;

		exit(EXIT_FAILURE);
	}
	close(fd);
}

struct turnoCliente actualizarTablero(int fila, int colum)
{
	cout << "t: " << fila << " " << colum << endl;
	rptr->tablero[fila][colum] = tableroRes[fila][colum];
	snprintf(rptr->tiempo,sizeof(TAM_TIEMPO), "%.1f\r\n",difftime(time(NULL),secondsInicio));
	struct turnoCliente t;
	t.fila = fila;
	t.colum = colum;
	return t;
}

void evaluarTurnos(struct turnoCliente t1, struct turnoCliente t2)
{
	if (tableroRes[t1.fila][t1.colum] == tableroRes[t2.fila][t2.colum])
	{
		cout << "Cliente acertó" << endl;
		rptr->paresRestantes = rptr->paresRestantes - 1;
	}
	else
	{
		rptr->tablero[t1.fila][t1.colum] = '-';
		rptr->tablero[t2.fila][t2.colum] = '-';
		cout << "Cliente no acertó" << endl;
	}
	rptr->nro = rptr->nro + 1;
	snprintf(rptr->tiempo,sizeof(TAM_TIEMPO), "%.1f\r\n",difftime(time(NULL),secondsInicio));
}

int main()
{
	cout << "Iniciando Servidor..." << endl;
	sem_t *llegaCliente = sem_open(SEM_LLEGA_CLIENTE, O_CREAT, 0600, 0);
	tableroListo = sem_open(SEM_TABLERO_LISTO, O_CREAT, 0600, 0);
	nuevaJugada = sem_open(SEM_NUEVA_JUGADA, O_CREAT, 0600, 0);

	cout << "Esperando cliente..." << endl;
	sem_wait(llegaCliente);
	sem_close(llegaCliente);
	sem_unlink(SEM_LLEGA_CLIENTE);
	cout << "Llegó cliente\nGenerando casillas" << endl;
	iniciarPartida();
	sem_post(tableroListo);

	struct turnoCliente t1;
	struct turnoCliente t2;

	do
	{
		sem_wait(nuevaJugada);
		cargarTurnoClienteComp();
		t1 = actualizarTablero(turnoCliente->fila, turnoCliente->colum);
		sem_post(tableroListo);

		sem_wait(nuevaJugada);
		cargarTurnoClienteComp();
		t2 = actualizarTablero(turnoCliente->fila, turnoCliente->colum);
		sem_post(tableroListo);

		sem_wait(nuevaJugada);
		evaluarTurnos(t1, t2);
		sem_post(tableroListo);

	}while(rptr->paresRestantes != 0);

	cout << "Partida finalizada" << endl;
	shm_unlink(NOMBRE_MATRIZ_COMP);
	shm_unlink(NOMBRE_JUGADA);
	sem_close(tableroListo);
	sem_close(nuevaJugada);
	sem_unlink(SEM_TABLERO_LISTO);
	sem_unlink(SEM_NUEVA_JUGADA);
}
