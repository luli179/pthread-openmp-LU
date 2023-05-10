#include<iostream>
#include <sys/time.h>                                                                                           
#include <unistd.h>
#include <nmmintrin.h>
#include<pthread.h>
#include<semaphore.h>
using namespace std;

const unsigned long Converter = 1000 * 1000; // 1s == 1000 * 1000 us
const int N = 2048;//规模
float matrix[N][N];
const int NUM_THREADS = 12;//工作线程数

//线程数据结构
typedef struct {
    int t_id;
}threadparam_t;

//信号量
sem_t sem_main;
sem_t sem_workstart[NUM_THREADS];
sem_t sem_workend[NUM_THREADS];

//线程函数
void* threadFunc(void* param)
{
    threadparam_t* p = (threadparam_t*)param;
    int t_id = p->t_id;
    for (int k = 0; k < N; k++)
    {
        sem_wait(&sem_workstart[t_id]);//阻塞，等待主线程完成乘法
        int j = 0;
        for (int i = k + 1 + t_id; i < N; i += NUM_THREADS)
        {
            __m128 vaik = _mm_set1_ps(matrix[i][k]);
            for (j = k + 1; j % 4 !=0; j++)
            {
                matrix[i][j] = matrix[i][j] - matrix[i][k] * matrix[k][j];
            }
            for (; j + 4 <= N; j += 4)
            {
                __m128 vakj = _mm_load_ps(&matrix[k][j]);//通过sse指令向量化加速
                __m128 vaij = _mm_load_ps(&matrix[i][j]);
                __m128 vx = _mm_mul_ps(vakj, vaik);
                vaij = _mm_sub_ps(vaij, vx);
                _mm_store_ps(&matrix[i][j], vaij);
            }
            for (; j < N; j++)
                matrix[i][j] = matrix[i][j] - matrix[k][j] * matrix[i][k];
            matrix[i][k] = 0.0;
        }
        sem_post(&sem_main);//唤醒主线程
        sem_wait(&sem_workend[t_id]);//阻塞
    }
    pthread_exit(NULL);
}
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
    //初始化信号量
    sem_init(&sem_main, 0, 0);
    for (int i = 0; i < NUM_THREADS; i++)
    {
        sem_init(&sem_workstart[i], 0, 0);
        sem_init(&sem_workend[i], 0, 0);
    }
    //创建线程
    pthread_t handles[NUM_THREADS];
    threadparam_t param[NUM_THREADS];
    for (int t_id = 0; t_id < NUM_THREADS; t_id++)
    {
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id], NULL, threadFunc, (void*)&param[t_id]);
    }
    for (int k = 0; k < N; k++)
    {
        //主线程完成除法操作
        for (int j = k + 1; j < N; j++)
        {
            matrix[k][j] = matrix[k][j] / matrix[k][k];
        }
        matrix[k][k] = 1.0;
        //唤醒工作线程
        for (int t_id = 0; t_id < NUM_THREADS; t_id++)
        {
            sem_post(&sem_workstart[t_id]);
        }
        //主线程睡眠
        for (int t_id = 0; t_id < NUM_THREADS; t_id++)
        {
            sem_wait(&sem_main);
        }
        //工作线程睡眠
        for (int t_id = 0; t_id < NUM_THREADS; t_id++)
        {
            sem_post(&sem_workend[t_id]);
        }

    }
    for (int t_id = 0; t_id < NUM_THREADS; t_id++)
    {
        pthread_join(handles[t_id], NULL);
    }
    sem_destroy(&sem_main);
    //销毁
    for (int i = 0; i < NUM_THREADS; i++)
    {
        sem_destroy(&sem_workstart[i]);
        sem_destroy(&sem_workend[i]);
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