CC = gcc -pthread -std=gnu99 -ggdb

DEPS = threadpool/threadpool.h threadpool/threads.h utils.h included.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

OBJ_S = server.o threadpool/threadpool.o
server: $(OBJ_S)
	$(CC) -o $@ $^

OBJ_C = client.o
client: $(OBJ_C)
	$(CC) -o $@ $^

clean:
	rm -fr ./server
	rm -fr client
	find . -name "*.o" -type f -delete
