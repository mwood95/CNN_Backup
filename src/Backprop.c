#include <stdio.h>
#include <string.h>
#include "Backprop.h"


/*************************** Function Prototypes *********************************/
float MSE(float target, float actual);
void backProp(float **filter, int row, int col, float **image, int ri, int ci, float target, float actual, int LR);
/*********************************************************************************/

/*********************************************************************************
 *  This function is not currently being used in the context of the overall
 *  network, but can be used for statistical analysis later.
 ********************************************************************************/
float MSE(float target, float actual)
{
	 float result = 0;
	 result =.5*(target - actual)*(target - actual);
	 return result;
}

/*******************************************************************************************************************************
 * Function: 	backProp
 *
 * Inputs:		filter:	Double pointer to the filter to be modified
 * 				row_f: 	Integer value of the number of rows in the filter
 * 				col_f:	Integer value of the number of columns in the filter
 * 				image:	Double pointer to the image that has been processed by the network
 * 				row_i:	Integer value of the number of rows in the image
 * 				col_i:	Integer value of the number of columns in the image
 * 				target:	Expected value that the network should have obtained
 * 				actual:	Actual value that the network obtained for the given image
 * 				LR:		Learning Rate of the network
 *
 *
 * Description:	This function provides the algorithm for obtaining new filter values to decrease the error on the next image.
 * 				The algorithm calculates the gradient of each filter value with respect to the mean squared error and
 * 				updates the corresponding filter value. (See documentation for a detailed explanation of theory behind
 * 				this).
 * *****************************************************************************************************************************/
void backProp(float **filter, int row_f, int col_f, float **image, int row_i, int col_i, float target, float actual, int LR)
{
	// Used for statistical analysis
	float error = MSE(target, actual);
	// New weight variable
	float wn;
	// Loop variables for filter
	int r, c;
	// Loop variables for image
	int a, b;

	// Selects filter pixel of interest
	for(r = 0; r < row_f; r++)
	{
		for(c = 0; c < col_f; c++)
		{
			// Reset new weight variable for next pass
			wn = 0;
			// Pass through and calculate filter pixel contribution to output
			for(a = 0; a < row_i; a++)
			{
				for(b = 0; b < col_i; b++)
				{
					if((a + r < (col_i - 2)) & (b + c < row_i - 2))
					{
						wn = wn + image[a + r][b + c];
					}
				}
			}
			// Average contribution
			wn = wn/(row_i*col_i);

			// Determine if filter values should decrease or increase based on target value
			if(target == 0)
			{
				filter[r][c] = filter[r][c] - LR*wn;
			}
			else
			{
				filter[r][c] = filter[r][c] + LR*wn;
			}
		}
	}
}
