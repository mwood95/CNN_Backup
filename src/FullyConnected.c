#include <stdio.h>
#include <string.h>
#include "FullyConnected.h"

float FullyConnected(float **input, int row, int col)
{
	int r, c;
	float result = 0;
	for(r = 0; r < row; r++)
	{
		for(c = 0; c < col; c++)
		{
			result = result + input[r][c];
		}
	}
	result = result/(row*col);
	return result;
}
