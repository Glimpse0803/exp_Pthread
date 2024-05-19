#include<iostream>
#include<iomanip>
#include<pthread.h>
#include<semaphore.h>
#include <sys/time.h>
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

//并行算法:

const int worker_count = 7;
sem_t sem_main;
sem_t sem_workerstart[worker_count];
sem_t sem_workerend[worker_count];

typedef struct
{
	float** A;//待操作的数组
	int N;//数组的规模
	int k;//消去的轮次
	int t_id;//线程id

} threadParam_t;

//线程函数:
void* threadFunc(void* param)
{
	//获取参数:
	threadParam_t* p = (threadParam_t*)param;
	float** A = p->A;
	int N = p->N;
	int k = p->k; //消去的轮次
	int t_id = p->t_id; //线程编号

	for (int k = 0; k < N; k++) {
        sem_wait(&sem_workerstart[t_id]); // 阻塞，等待主线完成除法操作（操作自己专属的信号量）
        //循环划分任务
        for (int i = k + 1 + t_id; i < N; i += worker_count) {
            //消去
            for (int j = k + 1; j < N; ++j)
                A[i][j] = A[i][j] - A[i][k] * A[k][j];

            A[i][k] = 0.0;
        }
        sem_post(&sem_main);            // 唤醒主线程
        sem_wait(&sem_workerend[t_id]); //阻塞，等待主线程唤醒进入下一轮
    }

	pthread_exit(NULL);
}

//并行算法:
void parallelSolution(float** A, int N) {

    //初始化信号量
    sem_init(&sem_main, 0, 0);
    for (int i = 0; i < worker_count; i++) {
        sem_init(&sem_workerstart[i], 0, 0);
        sem_init(&sem_workerend[i], 0, 0);
    }

    //创建线程

    pthread_t handles[worker_count]; // 创建对应的 Handle
    threadParam_t param[worker_count]; // 创建对应的线程数据结构

	for (int t_id = 0; t_id < worker_count; t_id++) {
        param[t_id].A = A;
        param[t_id].N = N;
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id], NULL, threadFunc, (void *)&param[t_id]);
    }

    for (int k = 0; k < N; k++)
    {
        //主线程做除法操作
        for (int j = k + 1; j < N; j++) {
            A[k][j] = A[k][j] / A[k][k];
        }
        A[k][k] = 1.0;
        //开始唤醒工作线程
        for (int t_id = 0; t_id < worker_count; t_id++) {
            sem_post(&sem_workerstart[t_id]);
        }
        //主线程睡眠（等待所有的工作线程完成此轮消去任务）
        for(int t_id = 0;t_id < worker_count; t_id++) {
            sem_wait(&sem_main);
        }

        // 主线程再次唤醒工作线程进入下一轮次的消去任务
        for (int t_id = 0; t_id < worker_count; t_id++) {
            sem_post(&sem_workerend[t_id]);
        }
    }

    for (int t_id = 0; t_id < worker_count; t_id++) {
        pthread_join(handles[t_id],NULL);
    }

    //销毁所有信号量
    sem_destroy(&sem_main);
    sem_destroy(sem_workerend);
    sem_destroy(sem_workerstart);
}

int main() {
	float** A;
	float** B;
	int N = 1280;
	A = new float* [N];
	for (int i = 0; i < N; i++) {
		A[i] = new float[N];//申请空间;
	}
	B = new float* [N];
	for (int i = 0; i < N; i++) {
		B[i] = new float[N];//申请空间;
	}
	int step = 64;
	int counter1;
	int counter2;
	struct timeval start1;
	struct timeval end1;
	struct timeval start2;
	struct timeval end2;
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

		//并行算法:
		generateSample(B, i);
		counter2 = 0;
		gettimeofday(&start2, NULL);
		gettimeofday(&end2, NULL);
		while ((end2.tv_sec - start2.tv_sec) < 1) {
			counter2++;
			//serialSolution(A, i);
			parallelSolution(B, i);
			gettimeofday(&end2, NULL);
		}

		//用时统计:
		float time1 = (end1.tv_sec - start1.tv_sec) + float((end1.tv_usec - start1.tv_usec)) / 1000000;//单位s;
		float time2 = (end2.tv_sec - start2.tv_sec) + float((end2.tv_usec - start2.tv_usec)) / 1000000;//单位s;


		cout << fixed << setprecision(6);
        cout << setw(13) << "数组规模" <<  i << ": " << "单线程平均用时：" << setw(20) << time1 / counter1 << endl;
        cout << setw(13) << " " << "多线程平均用时：" << setw(20) << time2 / counter2 << endl;
        cout << endl;
	}
	return 0;
}
