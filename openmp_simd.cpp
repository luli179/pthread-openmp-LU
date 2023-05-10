#include<iostream>
#include <sys/time.h>                                                                                           
#include <unistd.h>
#include<omp.h>
#include <nmmintrin.h>
using namespace std;

const unsigned long Converter = 1000 * 1000; // 1s == 1000 * 1000 us
const int N = 4096;
float matrix[N][N];
const int NUM_THREADS = 12;
//print function
void matrix_print()
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            cout << matrix[i][j] << ' ';
        }
        cout << endl;
    }
}
//initial function
void setmatrix()
{
    srand(2);
    for (int i = 0; i < N; i++)
    {
        matrix[i][i] = 1.0;
        for (int j = 0; j < N; j++)
        {
            if (i < j)
                matrix[i][j] = rand() % 4;
            if (i > j)
                matrix[i][j] = 0.0;
        }
    }
    int i = 0, j = 0;
    for (int k = 0; k < N; k++)
    {
        i = rand() % N;
        j = rand() % N;
        for (int m = 0; m < N; m++)
        {
            matrix[i][m] += matrix[j][m];
        }
    }
}
int main(void)
{
    //initial
    setmatrix();



    //start time
    struct timeval val;
    int ret = gettimeofday(&val, NULL);
    if (ret == -1)
    {
        printf("Error: gettimeofday()\n");
        return ret;
    }


    //operation
    int i = 0, j = 0, k = 0;
    __m128 vaik, vakj, vaij, vx;
#pragma omp parallel num_threads(NUM_THREADS),private(i,j,k,vaik,vakj,vaij,vx)
    for (k = 0; k < N; k++)
    {
#pragma omp single  
        for (j = k + 1; j < N; j++)
            matrix[k][j] /= matrix[k][k];//除法操作
        matrix[k][k] = 1.0;
#pragma omp for schedule(dynamic)//动态任务划分
        for (i = k + 1; i < N; i++)
        {
            vaik = _mm_set1_ps(matrix[i][k]);
            for (j = k + 1; j % 4 != 0; j++)
            {
                matrix[i][j] = matrix[i][j] - matrix[i][k] * matrix[k][j];
            }
            for (; j + 4 <= N; j += 4)
            {
                __m128 vakj = _mm_load_ps(&matrix[k][j]);//sse指令向量化加速
                __m128 vaij = _mm_load_ps(&matrix[i][j]);
                __m128 vx = _mm_mul_ps(vakj, vaik);
                vaij = _mm_sub_ps(vaij, vx);
                _mm_store_ps(&matrix[i][j], vaij);
            }
            for (; j < N; j++)
                matrix[i][j] = matrix[i][j] - matrix[k][j] * matrix[i][k];
            matrix[i][k] = 0.0;
        }
    }



    //end time
    struct timeval newVal;
    ret = gettimeofday(&newVal, NULL);
    if (ret == -1)
    {
        printf("Error: gettimeofday()\n");
        return ret;
    }

    //print time
    printf("start: sec --- %ld, usec --- %ld\n", val.tv_sec, val.tv_usec);
    printf("end:   sec --- %ld, usec --- %ld\n", newVal.tv_sec, newVal.tv_usec);
    //time sub
    unsigned long diff = (newVal.tv_sec * Converter + newVal.tv_usec) - (val.tv_sec * Converter + val.tv_usec);
    printf("diff:  sec --- %ld, usec --- %ld\n", diff / Converter, diff % Converter);
    //matrix_print();
    return 0;
}