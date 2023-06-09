#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#define MAX_CHILDREN 16
static int8_t numChildren;
typedef double MathFunc_t(double);


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
double integrateTrap(MathFunc_t *func, double rangeStart, double rangeEnd, size_t numSteps)
{
	double rangeSize = rangeEnd - rangeStart;
	double dx = rangeSize / numSteps;

	double area = 0;
	for (size_t i = 0; i < numSteps; i++)
	{
		double smallx = rangeStart + i * dx;
		double bigx = rangeStart + (i + 1) * dx;

		area += dx * (func(smallx) + func(bigx)) / 2; // Would be more efficient to multiply area by dx once at the end.
	}

	return area;
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

//decrease number of children when child exits SIGCHILD signal
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
			double area = integrateTrap(FUNCS[funcId], rangeStart, rangeEnd, numSteps);
			printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, area);
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
