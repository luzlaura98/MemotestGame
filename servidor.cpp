#include <iostream>
#include <stdlib.h>
#include <stdio.h>
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

struct turnoServidor *rptr;
struct turnoCliente *turnoCliente;
pid_t *pidServidorMC;

char tableroRes[FILA_COL][FILA_COL];
time_t secondsInicio;

bool todoTerminado = false;

void cargarTableroComp()
{
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

	struct turnoServidor turnoShared;
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
	memcpy(rptr, &turnoShared, sizeof(struct turnoServidor));
	snprintf(rptr->tiempo, sizeof(TAM_TIEMPO), "%.1f\r\n", difftime(time(NULL), secondsInicio));
}

struct turnoCliente actualizarTablero(int fila, int colum)
{
	cout << "t: " << fila << " " << colum << endl;
	rptr->tablero[fila][colum] = tableroRes[fila][colum];
	snprintf(rptr->tiempo, sizeof(TAM_TIEMPO), "%.1f\r\n", difftime(time(NULL), secondsInicio));
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
	snprintf(rptr->tiempo, sizeof(TAM_TIEMPO), "%.1f\r\n", difftime(time(NULL), secondsInicio));
}

//Método que se ejecutará al atrapar las señales.
void recibirSignal(int signum)
{
	if (signum == SIGUSR1)
	{
		cout << endl
			 << "SIGUSR1 recibido" << endl;
		if (!todoTerminado)
		{
			cout << "La partida está en progreso, el servidor no puede finalizar." << endl;
		}
		else
		{
			shm_unlink(NOMBRE_MATRIZ_COMP);
			shm_unlink(NOMBRE_JUGADA);
			sem_close(tableroListo);
			sem_close(nuevaJugada);
			sem_unlink(SEM_TABLERO_LISTO);
			sem_unlink(SEM_NUEVA_JUGADA);
			shm_unlink(PID_SERVIDOR);
			cout << "Partida finalizada" << endl;
			exit(EXIT_SUCCESS);
		}
	}
	else if (signum == SIGINT)
	{
		cout << endl
			 << "SIGINT recibido, pero ignorado :(" << endl;
	}
}

void checkUnSoloServidor()
{
	int fd = shm_open(PID_SERVIDOR, O_CREAT | O_EXCL, 0600);
	if (fd == -1) //ya existe un pid servidor guardado en mem
	{
		cout << "Ups, ya hay un servidor funcionando." << endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		close(fd);
		fd = shm_open(PID_SERVIDOR, O_CREAT | O_RDWR, 0600);
		ftruncate(fd, sizeof(pid_t));
		pidServidorMC = (pid_t *)mmap(NULL, sizeof(pid_t), PROT_WRITE, MAP_SHARED, fd, 0);
		*pidServidorMC = getpid();
	}
}

int main()
{
	checkUnSoloServidor();
	signal(SIGUSR1, recibirSignal); //señal y metodo. Atrapa esa señal y exec el metodo
	signal(SIGINT, recibirSignal);
	cout << "Iniciando Servidor..." << endl;
	sem_t *llegaCliente = sem_open(SEM_LLEGA_CLIENTE, O_CREAT, 0600, 0); //es creado si aún no existe

	cout << "Esperando cliente..." << endl;
	sem_wait(llegaCliente); //espera un solo cliente
	sem_close(llegaCliente);
	sem_unlink(SEM_LLEGA_CLIENTE);

	tableroListo = sem_open(SEM_TABLERO_LISTO, O_CREAT, 0600, 0);//Una vez que llega el cliente, el servidor los crea
	nuevaJugada = sem_open(SEM_NUEVA_JUGADA, O_CREAT, 0600, 0);
	cargarTableroComp();
	cargarTurnoClienteComp();

	cout << "Llegó cliente...\nGenerando casillas..." << endl;
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

	} while (rptr->paresRestantes != 0);

	cout << "Esperando para finalizar..." << endl;
	todoTerminado = true;
	while(true){}
}
