CC = g++

main: Logger.o main.o Server.o ThreadPool.o Task.o tinyxml2.o Utility.o
	$(CC) -o main Logger.o main.o Server.o ThreadPool.o Task.o tinyxml2.o Utility.o -lpthread

Logger.o: src/logger/Logger.cpp
	$(CC) -g -c src/logger/Logger.cpp

Server.o: src/server/Server.cpp
	$(CC) -g -c src/server/Server.cpp

ThreadPool.o: src/threadpool/ThreadPool.cpp
	$(CC) -g -c src/threadpool/ThreadPool.cpp

Task.o: src/threadpool/Task.cpp
	$(CC) -g -c src/threadpool/Task.cpp

tinyxml2.o: src/tinyxml2/tinyxml2.cpp
	$(CC) -g -c src/tinyxml2/tinyxml2.cpp

Utility.o: src/util/Utility.cpp
	$(CC) -g -c src/util/Utility.cpp

main.o: main.cpp
	$(CC) -g -c main.cpp

clean:
	rm -f *.o
	rm -f main

