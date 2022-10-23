#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MAX_CHILDREN 12	
#define NUMBER_OF_THREADS 64

static int8_t numChildren = 0;
typedef double MathFunc_t(double);


typedef struct ThreadDetails
{
	double rangeStart;
	double rangeEnd;
	double *integrationSum;
	size_t numSlices;
	size_t funcId;
	MathFunc_t *func;
	pthread_mutex_t *lock;

} ThreadDetails;

double gaussian(double x)
{
	return exp(-(x * x) / 2) / (sqrt(2 * M_PI));
}

double chargeDecay(double x)
{
	if (x < 0)
	{
		return 0;
	}
	else if (x < 1)
	{
		return 1 - exp(-5 * x);
	}
	else
	{
		return exp(-(x - 1));
	}
}

#define NUM_FUNCS 3
static MathFunc_t *const FUNCS[NUM_FUNCS] = {&sin, &gaussian, &chargeDecay};

// Integrate using the trapezoid method.
void *integrateTrap(void *NewThreadDetails)
{
	ThreadDetails *currentThread = (ThreadDetails *)NewThreadDetails;
	double rangeSize = currentThread->rangeEnd - currentThread->rangeStart;
	double dx = rangeSize / currentThread->numSlices;

	double section_area = 0;
	for (size_t i = 0; i < currentThread->numSlices; i++)
	{
		double smallx = currentThread->rangeStart + i * dx;
		double bigx = currentThread->rangeStart + (i + 1) * dx;

		section_area += dx * (currentThread->func(smallx) + currentThread->func(bigx)) / 2; // Would be more efficient to multiply area by dx once at the end.
	}

	pthread_mutex_lock(currentThread->lock);
	*currentThread->integrationSum += section_area;
	pthread_mutex_unlock(currentThread->lock);

	return NULL;
}

bool getValidInput(double *start, double *end, size_t *numSteps, size_t *funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	// Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);
	fflush(stdin);
	// Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}
void signalHandler()
{
	numChildren--;
}

double seconds(struct timespec start, struct timespec stop) {
  double diff = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / (double)1e9;
  return diff;
}

int main(void)
{
	signal(SIGCHLD, signalHandler);
	double rangeStart;
	double rangeEnd;
	int child_status;
	pid_t childPid;
	size_t numSteps;
	size_t funcId;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

	while ((getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)))
	{
		
		
		if ((childPid = fork()) < 0)
		{
			perror("fork");
			exit(1);
		}
		numChildren++;
		if (childPid == 0)
		{
			
			double integrationSum = 0;
			ThreadDetails threadetails[NUMBER_OF_THREADS];
			pthread_t threads[NUMBER_OF_THREADS];
			double slicesPerThread = numSteps / NUMBER_OF_THREADS;
			double rangeinc = (rangeEnd - rangeStart) / NUMBER_OF_THREADS;
			
			double integrationBounds = rangeStart;
			
			//set up the struct with the details for each thread
			for (int i = 0; i < NUMBER_OF_THREADS; i++)
			{
				threadetails[i].integrationSum = &integrationSum;
				threadetails[i].func = FUNCS[funcId];
				threadetails[i].rangeStart = integrationBounds;
				integrationBounds += rangeinc;
				threadetails[i].rangeEnd = integrationBounds;
				threadetails[i].numSlices = slicesPerThread;
				threadetails[i].lock = &lock;
			}
			for (int thread_num = 0; thread_num < NUMBER_OF_THREADS; thread_num++){
				pthread_create(&threads[thread_num], NULL, integrateTrap, &threadetails[thread_num]);
			} 

			for (int j = 0; j < NUMBER_OF_THREADS; j++)
			{
				pthread_join(threads[j], NULL);
			}
			printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, integrationSum);
			exit(0);
		}
		else
		{
			
		if (numChildren >= MAX_CHILDREN)
		{
			wait(NULL);
		}
		}
	}
	while(wait(&child_status) > 0); // wait for all children to finish before exiting
	exit(0);
}