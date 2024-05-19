#include<iostream>
#include<iomanip>
#include<pthread.h>
#include<semaphore.h>
#include <sys/time.h>
using namespace std;
void generateSample(float** A, int N) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < i; j++) {
			A[i][j] = 0;//�����Ǹ�ֵΪ0;
		}
		A[i][i] = 1.0;//�Խ��߸�ֵΪ1;
		for (int j = i; j < N; j++) {
			A[i][j] = rand();//�����Ǹ�ֵΪ����ֵ;
		}
	}
	for (int k = 0; k < N; k++) {
		for (int i = k + 1; i < N; i++) {
			for (int j = 0; j < N; j++) {
				A[i][j] += A[k][j];//ÿһ�ж����ϱ��Լ��±�С����;
			}
		}
	}
}
void show(float** A, int N) {//��ӡ���;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			cout << fixed << setprecision(0) << A[i][j] << " ";
		}
		cout << endl;
	}
}

//�����㷨:
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

//�����㷨:

const int worker_count = 7;
sem_t sem_main;
sem_t sem_workerstart[worker_count];
sem_t sem_workerend[worker_count];

typedef struct
{
	float** A;//������������
	int N;//����Ĺ�ģ
	int k;//��ȥ���ִ�
	int t_id;//�߳�id

} threadParam_t;

//�̺߳���:
void* threadFunc(void* param)
{
	//��ȡ����:
	threadParam_t* p = (threadParam_t*)param;
	float** A = p->A;
	int N = p->N;
	int k = p->k; //��ȥ���ִ�
	int t_id = p->t_id; //�̱߳��

	for (int k = 0; k < N; k++) {
        sem_wait(&sem_workerstart[t_id]); // �������ȴ�������ɳ��������������Լ�ר�����ź�����
        //ѭ����������
        for (int i = k + 1 + t_id; i < N; i += worker_count) {
            //��ȥ
            for (int j = k + 1; j < N; ++j)
                A[i][j] = A[i][j] - A[i][k] * A[k][j];

            A[i][k] = 0.0;
        }
        sem_post(&sem_main);            // �������߳�
        sem_wait(&sem_workerend[t_id]); //�������ȴ����̻߳��ѽ�����һ��
    }

	pthread_exit(NULL);
}

//�ź���ͬ���㷨:
void signal(float** A, int N) {

    //��ʼ���ź���
    sem_init(&sem_main, 0, 0);
    for (int i = 0; i < worker_count; i++) {
        sem_init(&sem_workerstart[i], 0, 0);
        sem_init(&sem_workerend[i], 0, 0);
    }

    //�����߳�

    pthread_t handles[worker_count]; // ������Ӧ�� Handle
    threadParam_t param[worker_count]; // ������Ӧ���߳����ݽṹ

	for (int t_id = 0; t_id < worker_count; t_id++) {
        param[t_id].A = A;
        param[t_id].N = N;
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id], NULL, threadFunc, (void *)&param[t_id]);
    }

    for (int k = 0; k < N; k++)
    {
        //���߳�����������
        for (int j = k + 1; j < N; j++) {
            A[k][j] = A[k][j] / A[k][k];
        }
        A[k][k] = 1.0;
        //��ʼ���ѹ����߳�
        for (int t_id = 0; t_id < worker_count; t_id++) {
            sem_post(&sem_workerstart[t_id]);
        }
        //���߳�˯�ߣ��ȴ����еĹ����߳���ɴ�����ȥ����
        for(int t_id = 0;t_id < worker_count; t_id++) {
            sem_wait(&sem_main);
        }

        // ���߳��ٴλ��ѹ����߳̽�����һ�ִε���ȥ����
        for (int t_id = 0; t_id < worker_count; t_id++) {
            sem_post(&sem_workerend[t_id]);
        }
    }

    for (int t_id = 0; t_id < worker_count; t_id++) {
        pthread_join(handles[t_id],NULL);
    }

    //���������ź���
    sem_destroy(&sem_main);
    sem_destroy(sem_workerend);
    sem_destroy(sem_workerstart);
}

//barrierͬ���㷨:

pthread_barrier_t barrier_Divsion;
pthread_barrier_t barrier_Elimination;
void* _threadFunc(void* param)
{
	//��ȡ����:
	threadParam_t* p = (threadParam_t*)param;
	float** A = p->A;
	int N = p->N;
	int k = p->k; //��ȥ���ִ�
	int t_id = p->t_id; //�̱߳��

	for (int k = 0; k < N; k++) {
		if (t_id == 0){
            for (int j = k + 1; j < N; j++) {
				A[k][j] = A[k][j] / A[k][k];
			}
            A[k][k] = 1.0;
        }
        pthread_barrier_wait(&barrier_Divsion);
        //ѭ����������
        for (int i = k + 1 + t_id; i < N; i += worker_count) {
            //��ȥ
            for (int j = k + 1; j < N; ++j)
                A[i][j] = A[i][j] - A[i][k] * A[k][j];

            A[i][k] = 0.0;
        }
        pthread_barrier_wait(&barrier_Elimination);
    }

	pthread_exit(NULL);
}


void barrier(float** A, int N) {

    pthread_barrier_init(&barrier_Divsion, NULL, worker_count);
    pthread_barrier_init(&barrier_Elimination, NULL, worker_count);

    //�����߳�

    pthread_t handles[worker_count]; // ������Ӧ�� Handle
    threadParam_t param[worker_count]; // ������Ӧ���߳����ݽṹ

	for (int t_id = 0; t_id < worker_count; t_id++) {
        param[t_id].A = A;
        param[t_id].N = N;
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id], NULL, _threadFunc, (void *)&param[t_id]);
    }

    for (int t_id = 0; t_id < worker_count; t_id++) {
        pthread_join(handles[t_id],NULL);
    }

    //��������barrier
    pthread_barrier_destroy(&barrier_Divsion);
    pthread_barrier_destroy(&barrier_Elimination);
}


int main() {
	float** A;
	float** B;
	float** C;
	int N = 1280;
	A = new float* [N];
	for (int i = 0; i < N; i++) {
		A[i] = new float[N];//����ռ�;
	}
	B = new float* [N];
	for (int i = 0; i < N; i++) {
		B[i] = new float[N];//����ռ�;
	}
	C = new float* [N];
	for (int i = 0; i < N; i++) {
		C[i] = new float[N];//����ռ�;
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
		//�����㷨
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

		//pthread�㷨:
		generateSample(B, i);
		counter2 = 0;
		gettimeofday(&start2, NULL);
		gettimeofday(&end2, NULL);
		while ((end2.tv_sec - start2.tv_sec) < 1) {
			counter2++;
			signal(B, i);
			gettimeofday(&end2, NULL);
		}

		//pthread + SIMD�㷨:
		generateSample(C, i);
		counter3 = 0;
		gettimeofday(&start3, NULL);
		gettimeofday(&end3, NULL);
		while ((end3.tv_sec - start3.tv_sec) < 1) {
			counter3++;
			barrier(C, i);
			gettimeofday(&end3, NULL);
		}

		//��ʱͳ��:
		float time1 = (end1.tv_sec - start1.tv_sec) + float((end1.tv_usec - start1.tv_usec)) / 1000000;//��λs;
		float time2 = (end2.tv_sec - start2.tv_sec) + float((end2.tv_usec - start2.tv_usec)) / 1000000;//��λs;
		float time3 = (end3.tv_sec - start3.tv_sec) + float((end3.tv_usec - start3.tv_usec)) / 1000000;//��λs;

		cout << fixed << setprecision(6);
        cout << "�����ģ" <<  i << ": " << endl;
		cout << " " << setw(18) << "���߳�ƽ����ʱ��" << setw(20) << time1 / counter1 << endl;
        cout << " " << setw(18) << "�ź���ͬ��ƽ����ʱ��" << setw(20) << time2 / counter2 << endl;
		cout << " " <<setw(18) << "barrierͬ��ƽ����ʱ��" << setw(20) << time3 / counter3 << endl;
        cout << endl;
	}
	return 0;
}
