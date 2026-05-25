CXX = g++
CXXFLAGS = -std=c++17 -Wall -I.
LIBS = -lpthread -lrt

.PHONY: all release debug test memcheck helgrind coverage clean

all: release

%.o: %.cpp bank.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

release: CXXFLAGS += -O2 -DNDEBUG
release: client init deinit

debug: CXXFLAGS += -g -O0
debug: client init deinit

client: client.o
	$(CXX) client.o -o client $(LIBS)

init: init.o
	$(CXX) init.o -o init $(LIBS)

deinit: deinit.o
	$(CXX) deinit.o -o deinit $(LIBS)

test: debug
	./init 2
	printf "get balance 0\nexit\n" | ./client
	printf "set max 1 100\nexit\n" | ./client
	printf "transfer 0 1 50\nexit\n" | ./client
	./deinit

memcheck: debug
	./init 2
	printf "set max 1 100\ntransfer 0 1 50\nexit\n" | valgrind --tool=memcheck --leak-check=full ./client
	./deinit

helgrind: debug
	./init 2
	printf "set max 1 100\ntransfer 0 1 50\nexit\n" | valgrind --tool=helgrind ./client
	./deinit

coverage: CXXFLAGS += --coverage
coverage: clean debug
	./init 2
	printf "set max 1 100\ntransfer 0 1 50\nexit\n" | ./client
	printf "get balance 0\nexit\n" | ./client
	./deinit
	gcov client.cpp init.cpp deinit.cpp

clean:
	rm -f *.o client init deinit *.gcno *.gcda *.gcov
