all: microbenchmark-page microbenchmark-double traverse_list stream stream_100M stream_1G stream_10G stream_64G stream_100G

#INCLUDES=-I ../NVMallocLibrary/src

FLAGS=-rdynamic -pg -O3

#LIBS=../NVMallocLibrary/libnvm.a

microbenchmark-page: microbenchmark-page.c
	$(CC) $(FLAGS) -o $@ $^

microbenchmark-double: microbenchmark-double.c
	$(CC) $(FLAGS) -o $@ $^

traverse_list: traverse_list.cpp
	$(CXX) $(FLAGS) --std=c++11 -o $@ $^

stream: stream_malloc.c
	$(CC) $(FLAGS) -o $@ $^

stream_100M: stream_malloc.c
	$(CC) $(FLAGS) -DSTREAM_ARRAY_SIZE=4369066 -o $@ $^

stream_1G: stream_malloc.c
	$(CC) $(FLAGS) -DSTREAM_ARRAY_SIZE=44739242 -o $@ $^

stream_10G: stream_malloc.c
	$(CC) $(FLAGS) -DSTREAM_ARRAY_SIZE=447392426 -o $@ $^

stream_64G: stream_malloc.c
	$(CC) $(FLAGS) -DSTREAM_ARRAY_SIZE=2863311530 -o $@ $^

stream_100G: stream_malloc.c
	$(CC) $(FLAGS) -DSTREAM_ARRAY_SIZE=4473924266 -o $@ $^

clean:
	rm -rf microbenchmark-page microbenchmark-double traverse_list stream stream_100M stream_1G stream_10G stream_64G stream_100G
