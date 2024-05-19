#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <sys/time.h>
#include <iomanip>
using namespace std;

//�̺߳��������ṹ��:
typedef struct {
    int t_id; // �߳� id
    int** EE;//����Ԫ��
    int** ER;//��Ԫ��
    int COL;//���������
    int eeROW;//����Ԫ�еĸ���
    int erROW;//��Ԫ�ӵĸ���
    int* flag;//��Ǹ����׷�����Ԫ�Ӵ������

} threadParam_t;

void* threadFunc(void* param) {
    threadParam_t* p = (threadParam_t*)param;
    int t_id = p->t_id;
    int** EE = p->EE;
    int** ER = p->ER;
    int COL = p->COL;
    int eeROW = p->eeROW;
    int erROW = p->erROW;
    int* flag = p->flag;

    for (int i = t_id; i < eeROW; i += 7) {
        int byte = 0;
        int bit = 0;
        int N = (COL + 31) / 32;
        while (true) {
            while (byte < N && EE[i][byte] == 0) {
                byte++;
                bit = 0;
            }
            if (byte >= N) {
                break;
            }
            int temp = EE[i][byte] << bit;
            while (temp >= 0) {
                bit++;
                temp <<= 1;
            }
            int& isExist = flag[COL - 1 - (byte << 5) - bit];
            if (!isExist == 0) {
                int* er = isExist > 0 ? ER[isExist - 1] : EE[~isExist];
                for (int j = 0; j < N; j++) {
                    EE[i][j] ^= er[j];
                }
            }
            else {
                isExist = ~i;
                break;
            }
        }
    }
    pthread_exit(NULL);
}

//���߳��㷨:
bool Pthread(int selection) {
    //selection ������ȡ�ĸ��ļ�
    string Folders[] = { "1_130_22_8", "2_254_106_53", "3_562_170_53", "4_1011_539_263", "5_2362_1226_453",
    "6_3799_2759_1953","7_8399_6375_4535", "11_85401_5724_756" };
    struct Size {
        int a;
        int b;
        int c;//�ֱ�Ϊ������������Ԫ�Ӹ����ͱ���Ԫ�и���
    } fileSize[] = { {130, 22, 8}, {254, 106, 53}, {562, 170, 53}, {1011, 539, 262}, {2362, 1226, 453},
    {3799, 2759, 1953},{8399, 6375, 4535},{85401,5724,756} };

    ifstream erFile;
    ifstream eeFile;
    erFile.open("/home/data/Groebner/" + Folders[selection] + "/1.txt", std::ios::binary);//��Ԫ���ļ�
    eeFile.open("/home/data/Groebner/" + Folders[selection] + "/2.txt", std::ios::binary);//����Ԫ���ļ�
    //ofstream resFile("/home/ubuntu/BingXing/Pthread/data/" + Folders[selection] + "/res_of_pThread.txt", ios::trunc);//�����д�ļ�

    int COL = fileSize[selection].a;
    int erROW = fileSize[selection].b;
    int eeROW = fileSize[selection].c;
    int N = (COL + 31) / 32;

    int** ER = new int* [erROW];
    int** EE = new int* [eeROW];
    int* flag = new int[COL] {0};

    //��ȡ��Ԫ��:
    for (int i = 0; i < erROW; i++) {
        ER[i] = new int[N] {0};
        int col;
        char ch = ' ';
        erFile >> col;
        int r = COL - 1 - col;
        ER[i][r >> 5] = 1 << (31 - (r & 31));
        erFile.get(ch);
        flag[col] = i + 1;
        while (erFile.peek() != '\r') {
            erFile >> col;
            int diff = COL - 1 - col;
            ER[i][diff >> 5] += 1 << (31 - (diff & 31));
            erFile.get(ch);
        }
    }

    //��ȡ����Ԫ��:
    for (int i = 0; i < eeROW; i++) {
        EE[i] = new int[N] {0};
        int col;
        char ch = ' ';
        while (eeFile.peek() != '\r') {
            eeFile >> col;
            int diff = COL - 1 - col;
            EE[i][diff >> 5] += 1 << (31 - (diff & 31));
            eeFile.get(ch);
        }
        eeFile.get(ch);
    }

    int worker_count = 7;//�߳�����
    pthread_t* handles = new pthread_t[worker_count];//����Handle
    threadParam_t* param = new threadParam_t[worker_count];//������Ӧ���߳����ݽṹ

    for (int t_id = 0; t_id < worker_count; t_id++) {
        param[t_id].t_id = t_id;
        param[t_id].EE = EE;
        param[t_id].ER = ER;
        param[t_id].COL = COL;
        param[t_id].eeROW = eeROW;
        param[t_id].erROW = erROW;
        param[t_id].flag = flag;
    }

    //�����߳�
    for (int t_id = 0; t_id < worker_count; t_id++)
        pthread_create(&handles[t_id], NULL, threadFunc, (void*)&param[t_id]);

    //���̹߳���ȴ����еĹ����߳���ɴ�����ȥ����
    for (int t_id = 0; t_id < worker_count; t_id++)
        pthread_join(handles[t_id], NULL);


    //���õ��Ľ��д�ص��ļ���
    // for (int i = 0; i < eeROW; i++) {
    //     int count = COL - 1;
    //     for (int j = 0; j < N; j++) {
    //         int dense = EE[i][j];
    //         for (int k = 0; k < 32; k++) {
    //             if (dense == 0) {
    //                 break;
    //             }
    //             else if (dense < 0) {
    //                 resFile << count - k << ' ';
    //             }
    //             dense <<= 1;
    //         }
    //         count -= 32;
    //     }
    //     resFile << '\n';
    // }
    //�ͷſռ�:
    for (int i = 0; i < erROW; i++) {
        delete[] ER[i];
    }
    delete[] ER;

    for (int i = 0; i < eeROW; i++) {
        delete[] EE[i];
    }
    delete[] EE;
    delete[] flag;
    return true;
}
//���߳��㷨:
bool Single_thread(int selection) {
    //selection ������ȡ�ĸ��ļ�
    string Folders[] = { "1_130_22_8", "2_254_106_53", "3_562_170_53", "4_1011_539_263", "5_2362_1226_453",
    "6_3799_2759_1953","7_8399_6375_4535", "11_85401_5724_756" };
    struct Size {
        int a;
        int b;
        int c;//�ֱ�Ϊ������������Ԫ�Ӹ����ͱ���Ԫ�и���
    } fileSize[] = { {130, 22, 8}, {254, 106, 53}, {562, 170, 53}, {1011, 539, 262}, {2362, 1226, 453},
    {3799, 2759, 1953},{8399, 6375, 4535},{85401,5724,756} };

    ifstream erFile;
    ifstream eeFile;
    erFile.open("/home/data/Groebner/" + Folders[selection] + "/1.txt", std::ios::binary);//��Ԫ���ļ�
    eeFile.open("/home/data/Groebner/" + Folders[selection] + "/2.txt", std::ios::binary);//����Ԫ���ļ�
    //ofstream resFile("/home/ubuntu/BingXing/Pthread/data/" + Folders[selection] + "/res_of_singleThread.txt", ios::trunc);//�����д�ļ�

    int COL = fileSize[selection].a;
    int erROW = fileSize[selection].b;
    int eeROW = fileSize[selection].c;
    int N = (COL + 31) / 32;

    int** ER = new int* [erROW];
    int** EE = new int* [eeROW];
    int* flag = new int[COL] {0};

    //��ȡ��Ԫ��:
    for (int i = 0; i < erROW; i++) {
        ER[i] = new int[N] {0};
        int col;
        char ch = ' ';
        erFile >> col;
        int r = COL - 1 - col;
        ER[i][r >> 5] = 1 << (31 - (r & 31));
        erFile.get(ch);
        flag[col] = i + 1;
        while (erFile.peek() != '\r') {
            erFile >> col;
            int diff = COL - 1 - col;
            ER[i][diff >> 5] += 1 << (31 - (diff & 31));
            erFile.get(ch);
        }
    }

    //��ȡ����Ԫ��:
    for (int i = 0; i < eeROW; i++) {
        EE[i] = new int[N] {0};
        int col;
        char ch = ' ';
        while (eeFile.peek() != '\r') {
            eeFile >> col;
            int diff = COL - 1 - col;
            EE[i][diff >> 5] += 1 << (31 - (diff & 31));
            eeFile.get(ch);
        }
        eeFile.get(ch);
    }

    for (int i = 0; i < eeROW; i++) {
        int byte = 0;
        int bit = 0;
        int N = (COL + 31) / 32;
        while (true) {
            while (byte < N && EE[i][byte] == 0) {
                byte++;
                bit = 0;
            }
            if (byte >= N) {
                break;
            }
            int temp = EE[i][byte] << bit;
            while (temp >= 0) {
                bit++;
                temp <<= 1;
            }
            int& isExist = flag[COL - 1 - (byte << 5) - bit];
            if (!isExist == 0) {
                int* er = isExist > 0 ? ER[isExist - 1] : EE[~isExist];
                for (int j = 0; j < N; j++) {
                    EE[i][j] ^= er[j];
                }
            }
            else {
                isExist = ~i;
                break;
            }
        }
    }

    //���õ��Ľ��д�ص��ļ���
    // for (int i = 0; i < eeROW; i++) {
    //     int count = COL - 1;
    //     for (int j = 0; j < N; j++) {
    //         int dense = EE[i][j];
    //         for (int k = 0; k < 32; k++) {
    //             if (dense == 0) {
    //                 break;
    //             }
    //             else if (dense < 0) {
    //                 resFile << count - k << ' ';
    //             }
    //             dense <<= 1;
    //         }
    //         count -= 32;
    //     }
    //     resFile << '\n';
    // }
    //�ͷſռ�:
    for (int i = 0; i < erROW; i++) {
        delete[] ER[i];
    }
    delete[] ER;

    for (int i = 0; i < eeROW; i++) {
        delete[] EE[i];
    }
    delete[] EE;
    delete[] flag;
    return true;
}

int main() {
    int counter1;
    int counter2;
    struct timeval start1;
    struct timeval end1;
    struct timeval start2;
    struct timeval end2;
    cout.flags(ios::left);
    for (int i = 0; i <= 7; i += 1) { //�����ļ�:
        //��ͳ�㷨
        counter1 = 0;
        gettimeofday(&start1, NULL);
        gettimeofday(&end1, NULL);
        while ((end1.tv_sec - start1.tv_sec) < 1) {
            counter1++;
            Single_thread(i);
            gettimeofday(&end1, NULL);
        }

        //���߳��㷨:
        counter2 = 0;
        gettimeofday(&start2, NULL);
        gettimeofday(&end2, NULL);
        while ((end2.tv_sec - start2.tv_sec) < 1) {
            counter2++;
            Pthread(i);
            gettimeofday(&end2, NULL);
        }

        //��ʱͳ��:
        float time1 = (end1.tv_sec - start1.tv_sec) + float((end1.tv_usec - start1.tv_usec)) / 1000000;//��λs;
        float time2 = (end2.tv_sec - start2.tv_sec) + float((end2.tv_usec - start2.tv_usec)) / 1000000;//��λs;


        cout << fixed << setprecision(6);
        cout << setw(10) << "���ݼ�" <<  i << ": " << "���߳�ƽ����ʱ��" << setw(20) << time1 / counter1 << endl;
        cout << setw(10) << " " << "���߳�ƽ����ʱ��" << setw(20) << time2 / counter2 << endl;
        cout << endl;
    }
    return 0;
}
