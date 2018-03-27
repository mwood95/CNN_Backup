#include <stdio.h>
#include <string.h>

void RLU(float **input, int row, int col)
{
	int r, c;

	for(r = 0; r < row; r++)
	{
		for(c = 0; c < col; c++)
		{
			if(input[r][c] < 0)
			{
				input[r][c] = 0;
			}
		}
	}
}
