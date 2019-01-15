all: librpc.a binder

LIBRPC_O = function.o rpcClient.o rpcServer.o serverDatabase.o serverInfo.o skeletonCallThread.o
librpc.a: ${LIBRPC_O}
	ar rcs librpc.a ${LIBRPC_O}

BINDER_O = binder.o binderDatabase.o function.o serverInfo.o
binder: ${BINDER_O}
	g++ -std=c++14 -Wall ${BINDER_O} -o binder

binder.o: binder.cpp
	g++ -std=c++14 -Wall binder.cpp -c

binderDatabase.o: binderDatabase.cpp
	g++ -std=c++14 -Wall binderDatabase.cpp -c

function.o: function.cpp
	g++ -std=c++14 -Wall function.cpp -c

rpcClient.o: rpcClient.cpp
	g++ -std=c++14 -Wall rpcClient.cpp -c

rpcServer.o: rpcServer.cpp
	g++ -std=c++14 -Wall rpcServer.cpp -c

serverDatabase.o: serverDatabase.cpp
	g++ -std=c++14 -Wall serverDatabase.cpp -c

serverInfo.o: serverInfo.cpp
	g++ -std=c++14 -Wall serverInfo.cpp -c

skeletonCallThread.o: skeletonCallThread.cpp
	g++ -std=c++14 -Wall -lpthread skeletonCallThread.cpp -c

client: client.o
	g++ -std=c++14 -L. client.o -lrpc -o client

server: server_functions.o server_function_skels.o server.o
	g++ -std=c++14 -pthread -L. server_functions.o server_function_skels.o server.o -lrpc -o server

client.o: client1.c
	g++ -std=c++14 -Wall client1.c -c -o client.o

server.o: server.c
	g++ -std=c++14 -Wall server.c -c

server_functions.o: server_functions.c
	g++ -std=c++14 -Wall server_functions.c -c

server_function_skels.o: server_function_skels.c
	g++ -std=c++14 -Wall server_function_skels.c -c

clean:
	rm *.o
	rm *.a
	rm server
	rm client
