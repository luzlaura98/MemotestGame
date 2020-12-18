CC = g++
all: cliente servidor
	

cliente:
	$(CC) -o cliente cliente.cpp -std=c++17 -pthread -lrt

servidor:
	$(CC) -o servidor servidor.cpp -std=c++17 -pthread -lrt

clean:
	rm -f cliente servidor