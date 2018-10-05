CC=gcc -g

all: test1 test2

test1: 
	$(CC) test1.c -o test1
test2:
	$(CC) -I../../NVMallocLibrary -o test2 test2.c ../../NVMallocLibrary/libnvm.a

clean:
	rm test1
