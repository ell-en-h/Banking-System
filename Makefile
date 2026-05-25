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
	printf "1\n0\n8\n" | ./client
	printf "6\n1\n100\n8\n" | ./client
	printf "5\n0\n1\n50\n8\n" | ./client
	./deinit

memcheck: debug
	./init 2
	printf "6\n1\n100\n8\n" | valgrind --tool=memcheck --leak-check=full ./client
	./deinit

helgrind: debug
	./init 2
	printf "6\n1\n100\n8\n" | valgrind --tool=helgrind ./client
	./deinit

coverage: CXXFLAGS += --coverage
coverage: clean debug
	./init 2
	printf "6\n1\n100\n8\n" | ./client
	printf "5\n0\n1\n50\n8\n" | ./client
	./deinit
	gcov client.cpp init.cpp deinit.cpp

clean:
	rm -f *.o client init deinit *.gcno *.gcda *.gcov
