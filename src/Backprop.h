float MSE(float target, float actual);
void backProp(float **filter, int row, int col, float **image, int ri, int ci, float target, float actual, int LR);
