all: init client deinit

init:
	g++ init.cpp -I. -o init -lrt -lpthread

client:
	g++ client.cpp -I. -o client -lrt -lpthread

deinit:
	g++ deinit.cpp -I. -o deinit -lrt -lpthread

clean:
	rm -f init client deinit
