#include <math.h>
#include <stdio.h>

void VectorPrint(float *a)
{
	printf("(%8.3f %8.3f %8.3f)\n", a[0],a[1],a[2]);
}

void VectorPrintd(double *a)
{
	printf("(%8.3lf %8.3lf %8.3lf)\n", a[0],a[1],a[2]);
}

void VectorSet2(float a1, float a2, float *a)
{
	a[0] = a1;
	a[1] = a2;
}

void VectorSet3(float a1, float a2, float a3, float *a)
{
	a[0] = a1;
	a[1] = a2;
	a[2] = a3;
}

void VectorSet4(float a1, float a2, float a3, float a4, float *a)
{
	a[0] = a1;
	a[1] = a2;
	a[2] = a3;
	a[3] = a4;
}

void VectorCopy3(float *a, float *b)
{
	b[0]=a[0];
	b[1]=a[1];
	b[2]=a[2];
}
void VectorCopy(float *a, float *b, int n)
{
	for (int i=0; i<n; i++) b[i] = a[i];
}

void VectorAdd(float *a, float *b, float *c)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
}

void VectorSubtract(float *a, float *b, float *c)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
}
	
float VectorDist(float *a, float *b)
{
	float d[3];
	d[0] = a[0] - b[0];
	d[1] = a[1] - b[1];
	d[2] = a[2] - b[2];
	return sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
}

void VectorNormalize(float *a)
{
	float d = sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
	a[0] /= d;
	a[1] /= d;
	a[2] /= d;
}

void VectorMA(float *in, float c, float *b, float *out) 
{
	out[0] = in[0] + c*b[0];
	out[1] = in[1] + c*b[1];
	out[2] = in[2] + c*b[2];
}

float DotProduct(float *a, float *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

double DotProductd(double *a, double *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void CrossProduct(float *in1, float *in2, float *out)
{
	out[0] = in1[1] * in2[2] - in1[2] * in2[1]; 
	out[1] = in1[2] * in2[0] - in1[0] * in2[2]; 
	out[2] = in1[0] * in2[1] - in1[1] * in2[0];
}


int VectorEqual(float *a, float *b)
{
	float EPS=1e-3;
	if (fabs(a[0]-b[0]) > EPS) return 0;
	if (fabs(a[1]-b[1]) > EPS) return 0;
	if (fabs(a[2]-b[2]) > EPS) return 0;
	return 1;
}
