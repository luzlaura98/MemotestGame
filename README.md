# MemotestGame
A Linux memotest game using shared memory and semaphores.

## About the project
This project was done for **Sistemas Operativos**, an Universidad Nacional de La Matanza's subject.

### Consigna
Implementar el clásico juego de la memoria “Memotest”, pero alfabético.
Para ello deberá crear dos procesos no emparentados que utilicen memoria compartida y se sincronicen con
semáforos.
Deberá existir un proceso “Cliente”, cuya tarea será mostrar por pantalla el estado actual del tablero y leer
desde teclado el par de casillas que el usuario quiere destapar.
También existirá un proceso “Servidor”, que será el encargado de actualizar el estado del tablero en base al
par de casillas ingresado, así como controlar la finalización partida.
Características del diseño:
* El tablero tendrá 16 casillas (4 filas x 4 columnas)
* Se debe garantizar que no se pueda ejecutar más de un cliente a la vez conectado al mismo servidor
* Se deberá garantizar que solo pueda haber un servidor por computadora
* Cada vez que se genere una nueva partida, el servidor deberá rellenar de manera aleatoria el tablero
con 8 pares de letras mayúsculas (A-Z). Cada letra seleccionada solo deberá aparecer dos veces en
posiciones también aleatorias
* El servidor se ejecutará y quedará a la espera de que un cliente se ejecute
* Tanto el cliente como el servidor deberán ignorar la señal SIGINT (Ctrl-C)
* El servidor deberá finalizar al recibir una señal SIGUSR1, siempre y cuando no haya ninguna partida
en progreso
* El cliente deberá mostrar por pantalla el tiempo para saber cuánto se tarda en resolver el juego

**Criterios de corrección (Obligatorio)**
* Funciona según enunciado Obligatorio
* Compila sin errores con el makefile entregado Obligatorio
* Cumple con todas las características de diseño indicadas Obligatorio
* Cierra correctamente los recursos utilizados al finalizar Obligatorio
* No hay pérdida de información Obligatorio
* El ejecutable cuenta con una ayuda (-h, --help)

## Getting Started
1. Compile server and client
```
make all
```
2. Execute server and client on individual terminals for each one
```
./servidor
```
```
./cliente
```
