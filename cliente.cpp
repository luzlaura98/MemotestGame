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
#include <list>
#include <cstring> //memset

#define FILA_COL 4
#define TAM_TIEMPO 20
#define CANT_CASILLAS FILA_COL *FILA_COL

#define NOMBRE_MATRIZ_COMP "miMatriz"
#define NOMBRE_JUGADA "turnoCliente"
#define NOMBRE_ESTADO_PARTIDA "estadoPartida"
#define SEM_LLEGA_CLIENTE "llegaCliente"
#define SEM_NUEVA_JUGADA "nuevaJUgada"
#define SEM_TABLERO_LISTO "tableroListo"

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
bool juegoTerminado = false;
struct turnoServidor *tServidor;

template <size_t rows, size_t cols>
void mostrarMatriz(char (&array)[rows][cols])
{
	cout << "\t0\t1\t2\t3" << endl;
	for (size_t i = 0; i < rows; ++i)
	{
		cout << i << "\t";
		for (size_t j = 0; j < cols; ++j)
			cout << array[i][j] << '\t';
		cout << endl;
	}
}

void mostrarTablero()
{
	struct turnoServidor *rptr;
	int fd;

	/*fileDescriptor*/ fd = shm_open(NOMBRE_MATRIZ_COMP, O_CREAT | O_RDWR, 0600); //0600
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
	cout << "Turno #" << rptr->nro << "  Tiempo: " << rptr->tiempo << "s  Pares restantes: " << rptr->paresRestantes << endl;
	cout << endl;
	mostrarMatriz(rptr->tablero);

	tServidor = rptr;
}

struct turnoCliente *cargarTurnoClienteComp()
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
	struct turnoCliente *tc = (struct turnoCliente *)mmap(NULL, sizeof(struct turnoCliente),
														  PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (tc == MAP_FAILED)
	{
		cout << "mmap error" << endl;
		exit(EXIT_FAILURE);
	}

	close(fd);
	return tc;
}

void ingresarPorTeclado()
{
	string fila, colum;
	cout << "Fila: ";
	cin >> fila;
	cout << "Columna: ";
	cin >> colum;

	struct turnoCliente t;
	t.fila = atoi(fila.c_str());
	t.colum = atoi(colum.c_str());
	memcpy(cargarTurnoClienteComp(), &t, sizeof(struct turnoCliente));
	system("clear");
}

int main()
{
	sem_t *llegaCliente = sem_open(SEM_LLEGA_CLIENTE, O_CREAT, 0600, 0);
	sem_post(llegaCliente); //V(llegaCliente)
	sem_close(llegaCliente);

	tableroListo = sem_open(SEM_TABLERO_LISTO, O_CREAT, 0600, 0);
	nuevaJugada = sem_open(SEM_NUEVA_JUGADA, O_CREAT, 0600, 0);

	sem_wait(tableroListo);

	do
	{
		system("clear");
		mostrarTablero();

		cout << "Ingrese primer casilla: " << endl;
		ingresarPorTeclado();
		sem_post(nuevaJugada);
		sem_wait(tableroListo);
		mostrarTablero(); //1er casilla descubierta

		cout << "Ingrese segunda casilla: " << endl;
		ingresarPorTeclado();
		sem_post(nuevaJugada);
		sem_wait(tableroListo);
		mostrarTablero(); //2da casilla descubierta

		do
		{
			cout << "Presione Enter para continuar" << endl;
			cin.ignore(numeric_limits<streamsize>::max(), '\n'); //flush
		} while (cin.get() != '\n');

		//Si esas 2 casilla no son iguales, se vuelven a tapar
		sem_post(nuevaJugada);
		sem_wait(tableroListo);

	} while (tServidor->paresRestantes != 0);

	system("clear");
	cout << "Juego finalizado!" << endl;
	mostrarTablero();
	sem_close(nuevaJugada);
	sem_close(tableroListo);
}
