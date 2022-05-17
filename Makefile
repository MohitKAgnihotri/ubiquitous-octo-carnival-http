
all: server 

clean:
	rm -f *.o
	rm -f *.out
	rm -f *.log
	rm -rf server client

server: http_server.c http_server.h file.c file.h mime.c mime.h
	gcc -Wall -g3 -O0 -o server http_server.c file.c mime.c -lpthread
