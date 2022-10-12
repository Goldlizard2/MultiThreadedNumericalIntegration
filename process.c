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


#define MAX_CHILDREN 3
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

	// Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}

void signalHandler()
{
	numChildren--;
}

int main(void)
{
	signal(SIGCHLD, signalHandler);
	double rangeStart;
	double rangeEnd;
	int fd[2];
	int child_status;
	pid_t childPid;
	size_t numSteps;
	size_t funcId;
	if (pipe(fd) == -1)
	{
		printf("An Error occured opening the pipe\n");
	}
	while (1)
	{

		if ((childPid = fork()) < 0)
		{
			perror("fork");
			exit(1);
		}

		else if (childPid == 0)
		{

			close(fd[1]);
			read(fd[0], &rangeStart, sizeof(double));
			read(fd[0], &rangeEnd, sizeof(double));
			read(fd[0], &numSteps, sizeof(size_t));
			read(fd[0], &funcId, sizeof(size_t));
			double area = integrateTrap(FUNCS[funcId], rangeStart, rangeEnd, numSteps);
			printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, area);
			close(fd[0]);
			exit(0);
		}
		else
		{
			if (numChildren >= MAX_CHILDREN)
			{
				wait(NULL);
			}
			numChildren++;
			if (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)){
				close(fd[0]);
				write(fd[1], &rangeStart, sizeof(double));
				write(fd[1], &rangeEnd, sizeof(double));
				write(fd[1], &numSteps, sizeof(size_t));
				write(fd[1], &funcId, sizeof(size_t));
//				printf("Input is %g %g %zu %zu \n", rangeStart, rangeEnd, numSteps, funcId);
				close(fd[1]);
				sleep(1);
			}
			else
			{
				while (wait(&child_status) > 0) {}
				exit(0);
			}
		}
	}
	exit(0);
}