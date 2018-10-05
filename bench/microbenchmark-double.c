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
#define LESS (2)
#define ITERATION 1

#define SEQ 1
#define RAN 0

int main() {
	int i, j, k;

	double *A;
	double t;
	double tsr, tsw, trr, trw; // trw for time random write
	double sumtsr = 0, sumtsw = 0, sumtrr = 0, sumtrw = 0;
	char page[4096];

	for (i = 0; i < 4096; i++) page[i] = 'a';

	int p = getpagesize();
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

	double x = 1.0;
	int d = sizeof(double);
	//printf("doulbe=%d\n", sizeof(double)); return;

for(j = 0; j < ITERATION; j++) {
    if (SEQ) {
	// sequential write
	tsw = mysecond();
	for (i = 0; i < GB / p * GBs; i++) {
		for (k = 0; k < p / d; k++) {
			//printf("GB / p * GBs=%d, p / d=%d, i * p / d + k * d=%d\n", GB / p * GBs, p / d, i * p / d + k * d);
			A[i * p / d + k] = x;
		}
		//strncpy(A + i * p, page, (size_t)p);
	}
	tsw = mysecond() - tsw;
	sumtsw += tsw;
	printf("Iteration %d: Seq Write Time: %f seconds. %lu MB\n", j, tsw, GB / 1024 / 1024 * GBs);

	//sleep(10);

	// sequential read
	tsr = mysecond();
	for (i = 0; i < GB / p * GBs; i++) {
                for (k = 0; k < p / d; k++) {
                        x = A[i * p / d + k];
                }
		//strncpy(page, A + i * p, (size_t)p);
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
		A[random_at_most((long)GB / p * GBs - 1) * p / d + random_at_most(p / d)] = x;
		//strncpy(A + random_at_most((long)GB / p * GBs - 1) * p, page, (size_t)p);
	}
	trw = mysecond() - trw;
	sumtrw += trw;
	printf("Iteration %d: Random Write Time: %f seconds. %lu KB\n", j, trw, GB / 1024 * GBs / LESS / p * d);

	//sleep(10);

	// random read 
	trr = mysecond();
	for (i = 0; i < GB / p * GBs / LESS; i++) {
		x = A[random_at_most((long)GB / p * GBs - 1) * p / d + random_at_most(p / d)];
		//strncpy(page, A + random_at_most((long)GB / p * GBs - 1) * p, (size_t)p);
	}
	trr = mysecond() - trr;
	sumtrr += trr;
	printf("Iteration %d: Random Read Time: %f seconds. %lu KB\n", j, trr, GB / 1024 * GBs / LESS / p * d);
    }
}
    if(SEQ) {
	printf("Average Seq Write Time: %f seconds. %lu MB. %f MB/s\n", sumtsw / ITERATION, GB / 1024 / 1024 * GBs, (double)GB / 1024 / 1024 * GBs / sumtsw * ITERATION);
	printf("Average Seq Read Time: %f seconds. %lu MB. %f MB/s\n", sumtsr / ITERATION, GB / 1024 / 1024 * GBs, (double)GB / 1024 / 1024 * GBs / sumtsr * ITERATION);
    }
    if (RAN) {
	printf("Average Random Write Time: %f seconds. %lu KB. %f KB/s\n", sumtrw / ITERATION, GB / 1024 * GBs / LESS / p * d, (double)GB / 1024 / 1024 * GBs / LESS / sumtrw * ITERATION / p * d);
	printf("Average Random Read Time: %f seconds. %lu KB. %f KB/s\n", sumtrr / ITERATION, GB / 1024 * GBs / LESS / p * d, (double)GB / 1024 / 1024 * GBs / LESS / sumtrr * ITERATION / p * d);
    }

	free(A);
	t = mysecond() - t;
	printf("Total time: %f seconds.\n", t);

	return 0;
}
