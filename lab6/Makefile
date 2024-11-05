all: client server

server: server.c
	gcc -o $@ $<

client: client.c
	gcc -o $@ $<

clean:
	rm client server
