output : client.o server.o helper.o
	gcc -o client client.o helper.o 
	gcc -o server server.o helper.o 
	make objClean

profile : client.o server.o helper.o profile.o
	gcc -o client client.o helper.o 
	gcc -o server server.o helper.o 
	gcc -o profile profile.o
	make objClean
	
client.o : client.c
	gcc -c client.c

server.o : server.c
	gcc -c server.c

helper.o : helper.c
	gcc -c helper.c

profile.o : profile.c
	gcc -c profile.c

objClean :
	rm -f *.o

clean :
	rm -f client
	rm -f server
	rm -f profile
