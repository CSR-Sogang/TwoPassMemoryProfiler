#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

//int getpagesize(void); 

// random number b/w [0, max]
long random_at_most(long max) {
	unsigned long	num_bins = (unsigned long) max + 1,
			num_rand = (unsigned long) RAND_MAX + 1,
			bin_size = num_rand / num_bins,
			defect   = num_rand % num_bins;

	long x;

	do {
		x = random();
	} while (num_rand - defect <= (unsigned long)x);

	return x/bin_size;
}

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

#define GB (1024*1024*1024)
#define GBs 1
#define LESS (1)
#define ITERATION 3
#define SEQ 1
#define RAN 1


int main(int argv, char** argc) {
	int i, j;

	char *A;
	double t;
	double tsr, tsw, trr, trw; // trw for time random write
	double sumtsr = 0, sumtsw = 0, sumtrr = 0, sumtrw = 0;
	char page[4096];

	for (i = 0; i < 4096; i++) page[i] = 'a';

	int p = getpagesize();
	if (argv > 1) {
		p /= atoi(argc[1]);
	}
/*
	printf("pagesize=%d\n", p);

	int x = 2 * sizeof(unsigned int);
	printf("x=%d\n", x);
	return 0;
*/
	t = mysecond();

	printf("Ask for %d MB space.\n", GB / 1024 / 1024 * GBs);
	A = malloc((size_t)GB * GBs);
	if (A == NULL) {
		printf("malloc(): out of space\n");
		exit(-1);
	}

for(j = 0; j < ITERATION; j++) {
    if (SEQ) {
	// sequential write
	tsw = mysecond();
	for (i = 0; i < GB / p * GBs; i++) {
		strncpy(A + i * p, page, (size_t)p);
	}
	tsw = mysecond() - tsw;
	sumtsw += tsw;
	printf("Iteration %d: Seq Write Time: %f seconds. %lu MB\n", j, tsw, GB / 1024 / 1024 * GBs);

	//sleep(10);

	// sequential read
	tsr = mysecond();
	for (i = 0; i < GB / p * GBs; i++) {
		strncpy(page, A + i * p, (size_t)p);
	}
	tsr = mysecond() - tsr;
	sumtsr += tsr;
	printf("Iteration %d: Seq Read Time: %f seconds. %lu MB\n", j, tsr, GB / 1024 / 1024 * GBs);

    }
	//sleep(10);

    if (RAN) {
	// random write
	trw = mysecond();
	for (i = 0; i < GB / p * GBs / LESS; i++) {
		strncpy(A + random_at_most((long)GB / p * GBs - 1) * p, page, (size_t)p);
	}
	trw = mysecond() - trw;
	sumtrw += trw;
	printf("Iteration %d: Random Write Time: %f seconds. %lu MB\n", j, trw, GB / 1024 / 1024 * GBs / LESS);

	//sleep(10);

	// random read 
	trr = mysecond();
	for (i = 0; i < GB / p * GBs / LESS; i++) {
		strncpy(page, A + random_at_most((long)GB / p * GBs - 1) * p, (size_t)p);
	}
	trr = mysecond() - trr;
	sumtrr += trr;
	printf("Iteration %d: Random Read Time: %f seconds. %lu MB\n", j, trr, GB / 1024 / 1024 * GBs / LESS);
    }
}
    if(SEQ) {
	printf("Average Seq Write Time: %f seconds. %lu MB. %f MB/s\n", sumtsw / ITERATION, GB / 1024 / 1024 * GBs, (double)GB / 1024 / 1024 * GBs / sumtsw * ITERATION);
	printf("Average Seq Read Time: %f seconds. %lu MB. %f MB/s\n", sumtsr / ITERATION, GB / 1024 / 1024 * GBs, (double)GB / 1024 / 1024 * GBs / sumtsr * ITERATION);
    }
    if (RAN) {
	printf("Average Random Write Time: %f seconds. %lu MB. %f MB/s\n", sumtrw / ITERATION, GB / 1024 / 1024 * GBs / LESS, (double)GB / 1024 / 1024 * GBs / LESS / sumtrw * ITERATION);
	printf("Average Random Read Time: %f seconds. %lu MB. %f MB/s\n", sumtrr / ITERATION, GB / 1024 / 1024 * GBs / LESS, (double)GB / 1024 / 1024 * GBs / LESS / sumtrr * ITERATION);
    }

	free(A);
	t = mysecond() - t;
	printf("Total time: %f seconds.\n", t);

	return 0;
}
