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

struct turnoServidor *tServidor;
struct turnoCliente *tc;

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

/**Verifica que el string es un numero válido:
 * 	-Es un numero positivo
 * 	-Dentro del rango de casillas
 * Devuelve el numero y -1 si no es valido
*/
int esNumeroValido(string input)
{
	char *endp;
	int num = strtol(input.c_str(), &endp, 10);
	if (*endp != '\0')
	{
		cout << "No es un número válido.";
		num = -1;
	}
	else if (num >= FILA_COL || num < 0)
	{
		cout << "El número ingresado no pertenece a un rango válido [0-" << FILA_COL - 1 << "]" <<endl;
		num = -1;
	}
	return num;
}

int ingresarNumero()
{
	string input;
	int num;
	cin >> input;
	while ((num = esNumeroValido(input)) == -1)
	{
		cout << " Intente una vez más: " ;
		cin >> input;
	}
	return num;
}

bool esUbicacionCasillaValida(int f, int c){
	return tServidor->tablero[f][c] == '-';
}

void ingresarPorTeclado()
{
	int fila, colum;
		cout << "Fila: ";
		fila = ingresarNumero();
		cout << "Columna: ";
		colum = ingresarNumero();

	while(!esUbicacionCasillaValida(fila, colum))
	{
		cout << "La ubicación especificada ya tiene un valor." << endl;
		cout << "Fila: ";
		fila = ingresarNumero();
		cout << "Columna: ";
		colum = ingresarNumero();
	}
	tc->fila = fila;
	tc->colum = colum;
	system("clear");
}

void mostrarTablero()
{
	cout << "Turno #" << tServidor->nro << "  Tiempo: " << tServidor->tiempo << "s  Pares restantes: " << tServidor->paresRestantes << endl;
	cout << endl;
	mostrarMatriz(tServidor->tablero);
}

void cargarTablero()
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
	tServidor = (struct turnoServidor *)mmap(NULL, sizeof(struct turnoServidor),
											 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (tServidor == MAP_FAILED)
	{
		cout << "mmap error" << endl;
		exit(EXIT_FAILURE);
	}
	close(fd);
}

void cargarTurnoClienteComp()
{
	int fd = shm_open(NOMBRE_JUGADA, O_CREAT | O_RDWR, 0600); //0600
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
	tc = (struct turnoCliente *)mmap(NULL, sizeof(struct turnoCliente),
									 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (tc == MAP_FAILED)
	{
		cout << "mmap error" << endl;
		exit(EXIT_FAILURE);
	}
	close(fd);
}

void recibirSignal(int signum)
{
	cout << "SIGINT recibido, pero ignorado :(" << endl;
}

void killServidor()
{
	int fd = shm_open(PID_SERVIDOR, O_RDONLY, 0600);
	if (fd == -1)
	{
		cout << "Error" << endl;
		exit(EXIT_FAILURE);
	}
	ftruncate(fd, sizeof(pid_t));
	pid_t *pidServidorMC = (pid_t *)mmap(NULL, sizeof(pid_t), PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	kill(*pidServidorMC, SIGUSR1); //envía una señal SIGUSR1 a ese proceso
}

void checkServidorOcupado()
{
	int fd = shm_open(NOMBRE_MATRIZ_COMP, O_CREAT | O_EXCL, 0600);
	if (fd == -1)
	{
		cout << "Ups, servidor ocupado con otro cliente." << endl;
		exit(EXIT_FAILURE);
	}
	close(fd);
}

int main()
{
	signal(SIGINT, recibirSignal);
	checkServidorOcupado();
	sem_t *llegaCliente = sem_open(SEM_LLEGA_CLIENTE, O_CREAT, 0600, 0);
	sem_post(llegaCliente);
	sem_close(llegaCliente);
	tableroListo = sem_open(SEM_TABLERO_LISTO, O_CREAT, 0600, 0);
	nuevaJugada = sem_open(SEM_NUEVA_JUGADA, O_CREAT, 0600, 0);
	cargarTablero();
	cargarTurnoClienteComp();

	cout << "Esperando para comenzar!" << endl;
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
	cout << "*************Juego finalizado!************" << endl;
	mostrarTablero();
	killServidor();
}
