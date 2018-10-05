

CC=gcc -pg -rdynamic
CXX=g++ -pg -rdynamic
#CXX=g++
CFLAGS=-I. 

SRC=./src

DEPS = $(SRC)/calltrack.h 
OBJ = $(SRC)/calltrack.o

all: baseline dynamic

baseline: $(OBJ)
	ar rcs libcalltrack.a ${SRC}/calltrack.o 

dynamic:
	$(CXX) -I./src -shared -fPIC -mcmodel=large -o libdcalltrack.so ${SRC}/dcalltrack.cpp -ldl
	$(CXX) -I./src -shared -fPIC -mcmodel=large -D_WRITE_ADDR -o libgetalloc.so ${SRC}/dcalltrack.cpp -ldl
	$(CXX) -I./src -shared -fPIC -mcmodel=large -o libdnvm.so ${SRC}/dstack.cpp ../NVMallocLibrary/libnvm.a -ldl
	$(CXX) -I./src -shared -fPIC -mcmodel=large -D_NO_SHOWSTACK -o libdinfo.so ${SRC}/dstack.cpp -ldl
	$(CXX) -std=c++11 -I. -shared -fPIC -mcmodel=large -o libdcalltrack_mt.so ${SRC}/dcalltrack_mt.cpp -ldl
	$(CXX) -I./src -I../NVMallocLibrary/src -fPIC -c -o ${SRC}/deepmap.o ${SRC}/deepmap.cpp -ldl
	ar -x ../NVMallocLibrary/libnvm.a 
	$(CXX) -shared -o libdeepmap.so *.o ${SRC}/deepmap.o -ldl
	$(CXX) -I./src -shared -fPIC -mcmodel=medium -DSHOW_SIZE -DSHOW_FREE -o libfastpass.so ${SRC}/dcalltrack.cpp -ldl
	$(CXX) -I./src -shared -g -fPIC -mcmodel=medium -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DREAD_VAR_HASH_VALUE -DIDENT_TRANSFER -o libslowpass.so ${SRC}/dcalltrack.cpp -ldl
clean: 
	rm -f ${SRC}/*.o *.o libcalltrack.a *.so
