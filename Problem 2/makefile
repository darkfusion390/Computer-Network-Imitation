output : client.o helper.o relay.o server.o
	gcc -o relay relay.o helper.o -lm
	gcc -o client client.o helper.o -lm
	gcc -o server server.o helper.o -lm
	make objClean

relay : client.o helper.o relay.o
	gcc -o relay relay.o helper.o -lm
	gcc -o client client.o helper.o -lm
	make objClean
	
relay.o : relay.c
	gcc -c relay.c

client.o : client.c
	gcc -c client.c

server.o : server.c
	gcc -c server.c

helper.o : helper.c
	gcc -c helper.c

objClean :
	rm *.o

clean :
	rm -f relay server client
