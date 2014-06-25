void VectorPrint(float *a);
void VectorPrintd(double *a);
void VectorCopy3(float *a, float *b);
void VectorCopy(float *a, float *b, int n);
void VectorAdd(float *a, float *b, float *c);
void VectorSubtract(float *a, float *b, float *c);
float VectorDist(float *a, float *b);
void VectorNormalize(float *a);
void VectorMA(float *in, float c, float *b, float *out);
float DotProduct(float *a, float *b);
double DotProductd(double *a, double *b);
void CrossProduct(float *in1, float *in2, float *out);

void VectorSet2(float a1, float a2, float *a);
void VectorSet3(float a1, float a2, float a3, float *a);
void VectorSet4(float a1, float a2, float a3, float a4, float *a);

int VectorEqual(float *a, float *b);
