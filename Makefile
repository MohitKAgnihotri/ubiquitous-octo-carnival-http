
all: server client

clean:
	rm -f *.o
	rm -f *.out
	rm -f *.log
	rm -rf server client

server: http_server.c http_server.h file.c file.h mime.c mime.h
	gcc -o server http_server.c http_server.h file.c file.h mime.c mime.h -lpthread

client: http_client.c http_client.h
	gcc -o client http_client.c http_client.h -lpthread