#include<iostream>
#include<iomanip>
#include<pthread.h>
#include <sys/time.h>
#include<emmintrin.h> //SSE2
#include<pmmintrin.h> //SSE3
#include<tmmintrin.h> //SSSE3
#include<smmintrin.h> //SSE4.1
#include<nmmintrin.h> //SSSE4.2
#include<immintrin.h> //AVX、AVX2
using namespace std;
void generateSample(float** A, int N) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < i; j++) {
			A[i][j] = 0;//下三角赋值为0;
		}
		A[i][i] = 1.0;//对角线赋值为1;
		for (int j = i; j < N; j++) {
			A[i][j] = rand();//上三角赋值为任意值;
		}
	}
	for (int k = 0; k < N; k++) {
		for (int i = k + 1; i < N; i++) {
			for (int j = 0; j < N; j++) {
				A[i][j] += A[k][j];//每一行都加上比自己下标小的行;
			}
		}
	}
}
void show(float** A, int N) {//打印结果;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			cout << fixed << setprecision(0) << A[i][j] << " ";
		}
		cout << endl;
	}
}

//串行算法:
void serialSolution(float** A, int N) {
	for (int k = 0; k < N; k++) {
		for (int j = k + 1; j < N; j++) {
			A[k][j] /= A[k][k];
		}
		A[k][k] = 1.0;
		for (int i = k + 1; i < N; i++) {
			for (int j = k + 1; j < N; j++) {
				A[i][j] -= A[i][k] * A[k][j];
			}
			A[i][k] = 0;
		}
	}
}
//参数:
typedef struct
{
	float** A;//待操作的数组
	int N;//数组的规模
	int k;//消去的轮次
	int t_id;//线程id
	int worker_count;

} threadParam_t;

//pthread算法:
//线程函数:
void* _threadFunc(void* param)
{
	//获取参数:
	threadParam_t* p = (threadParam_t*)param;
	float** A = p->A;
	int N = p->N;
	int k = p->k; //消去的轮次
	int t_id = p->t_id; //线程编号
	int worker_count = p->worker_count;

	for (int i = k + 1 + t_id; i < N; i += worker_count) {
		for (int j = k + 1; j < N; ++j) {
			A[i][j] = A[i][j] - A[i][k] * A[k][j];
		}
		A[i][k] = 0;
	}
	pthread_exit(NULL);
}

void PthreadSolution(float** A, int N) {
	for (int k = 0; k < N; ++k) {
		//主线程做除法操作
		for (int j = k + 1; j < N; j++) {
			A[k][j] = A[k][j] / A[k][k];
		}
		A[k][k] = 1.0;

		//创建工作线程，进行消去操作
		int worker_count = 7; //工作线程数量
		pthread_t* handles = new pthread_t[worker_count]; // 创建对应的 Handle
		threadParam_t* param = new threadParam_t[worker_count]; // 创建对应的线程数据结构

		//分配任务
		for (int t_id = 0; t_id < worker_count; t_id++) {
			param[t_id].A = A;
			param[t_id].N = N;
			param[t_id].k = k;
			param[t_id].t_id = t_id;
			param[t_id].worker_count = worker_count;
		}

		//创建线程
		for (int t_id = 0; t_id < worker_count; t_id++) {
			pthread_create(&handles[t_id], NULL, _threadFunc, (void*)&param[t_id]);
		}

		//主线程挂起等待所有的工作线程完成此轮消去工作
		for (int t_id = 0; t_id < worker_count; t_id++) {
			pthread_join(handles[t_id], NULL);
		}
	}
}

//pthread+SIMD算法:
//线程函数:
void* threadFunc(void* param)
{
	//获取参数:
	threadParam_t* p = (threadParam_t*)param;
	float** A = p->A;
	int N = p->N;
	int k = p->k; //消去的轮次
	int t_id = p->t_id; //线程编号
	int worker_count = p->worker_count;

	for (int i = k + 1 + t_id; i < N; i += worker_count) {
		__m128 vaik = _mm_loadu_ps(&A[i][k]);
		for (int j = k + 1; j + 4 <= N; j += 4) {
			__m128 vakj = _mm_loadu_ps(&A[k][j]);
			__m128 vaij = _mm_loadu_ps(&A[i][j]);
			__m128 vx = _mm_mul_ps(vakj, vaik);
			vaij = _mm_sub_ps(vaij, vx);
			_mm_storeu_ps(&A[i][j], vaij);
			if (j + 8 > N) {//处理末尾
				while (j < N) {
					A[i][j] -= A[i][k] * A[k][j];
					j++;
				}
				break;
			}
		}
		A[i][k] = 0;
	}
	pthread_exit(NULL);
}

void Pthread_SIMD_Solution(float** A, int N) {
	for (int k = 0; k < N; ++k) {
		//主线程做除法操作,使用SIMD优化;
		__m128 vt = _mm_set1_ps(A[k][k]);
		for (int j = k + 1; j + 4 <= N; j += 4) {
			__m128 va = _mm_loadu_ps(&A[k][j]);
			va = _mm_div_ps(va, vt);
			_mm_storeu_ps(&A[k][j], va);
			if (j + 8 > N) {//处理末尾
				while (j < N) {
					A[k][j] /= A[k][k];
					j++;
				}
				break;
			}
		}
		A[k][k] = 1.0;

		//创建工作线程，进行消去操作
		int worker_count = 7; //工作线程数量
		pthread_t* handles = new pthread_t[worker_count]; // 创建对应的 Handle
		threadParam_t* param = new threadParam_t[worker_count]; // 创建对应的线程数据结构

		//分配任务
		for (int t_id = 0; t_id < worker_count; t_id++) {
			param[t_id].A = A;
			param[t_id].N = N;
			param[t_id].k = k;
			param[t_id].t_id = t_id;
			param[t_id].worker_count = worker_count;
		}

		//创建线程
		for (int t_id = 0; t_id < worker_count; t_id++) {
			pthread_create(&handles[t_id], NULL, threadFunc, (void*)&param[t_id]);
		}

		//主线程挂起等待所有的工作线程完成此轮消去工作
		for (int t_id = 0; t_id < worker_count; t_id++) {
			pthread_join(handles[t_id], NULL);
		}
	}
}

int main() {
	float** A;
	float** B;
	float** C;
	int N = 1280;
	A = new float* [N];
	for (int i = 0; i < N; i++) {
		A[i] = new float[N];//申请空间;
	}
	B = new float* [N];
	for (int i = 0; i < N; i++) {
		B[i] = new float[N];//申请空间;
	}
	C = new float* [N];
	for (int i = 0; i < N; i++) {
		C[i] = new float[N];//申请空间;
	}
	int step = 64;
	int counter1;
	int counter2;
	int counter3;
	struct timeval start1;
	struct timeval end1;
	struct timeval start2;
	struct timeval end2;
	struct timeval start3;
	struct timeval end3;
	cout.flags(ios::left);

	for (int i = step; i <= N; i += step) {
		//串行算法
		generateSample(A, i);
		counter1 = 0;
		gettimeofday(&start1, NULL);
		gettimeofday(&end1, NULL);
		while ((end1.tv_sec - start1.tv_sec) < 1) {
			counter1++;
			serialSolution(A, i);
			//parallelSolution(B, i);
			gettimeofday(&end1, NULL);
		}

		//pthread算法:
		generateSample(B, i);
		counter2 = 0;
		gettimeofday(&start2, NULL);
		gettimeofday(&end2, NULL);
		while ((end2.tv_sec - start2.tv_sec) < 1) {
			counter2++;
			//serialSolution(A, i);
			PthreadSolution(B, i);
			gettimeofday(&end2, NULL);
		}

		//pthread + SIMD算法:
		generateSample(C, i);
		counter3 = 0;
		gettimeofday(&start3, NULL);
		gettimeofday(&end3, NULL);
		while ((end3.tv_sec - start3.tv_sec) < 1) {
			counter3++;
			Pthread_SIMD_Solution(C, i);
			gettimeofday(&end3, NULL);
		}

		//用时统计:
		float time1 = (end1.tv_sec - start1.tv_sec) + float((end1.tv_usec - start1.tv_usec)) / 1000000;//单位s;
		float time2 = (end2.tv_sec - start2.tv_sec) + float((end2.tv_usec - start2.tv_usec)) / 1000000;//单位s;
		float time3 = (end3.tv_sec - start3.tv_sec) + float((end3.tv_usec - start3.tv_usec)) / 1000000;//单位s;

		cout << fixed << setprecision(6);
        cout << "数组规模" <<  i << ": " << endl;
		cout << " " << setw(18) << "单线程平均用时：" << setw(20) << time1 / counter1 << endl;
        cout << " " << setw(18) << "多线程平均用时：" << setw(20) << time2 / counter2 << endl;
		cout << " " <<setw(18) << "多线程+SIMD平均用时：" << setw(20) << time3 / counter3 << endl;
        cout << endl;
	}
	return 0;
}
