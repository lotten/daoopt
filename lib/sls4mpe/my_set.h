#ifndef _SLS4MPE_MY_SET_H_
#define _SLS4MPE_MY_SET_H_

#include <vector>
using namespace std;

namespace sls4mpe {

int contains(const int *array, int size, int element);
int contains2(int *array, int size, int element);
bool contains(vector<int> vec, int element); // some nicer.

void insert(int *array, int *size, int new_element);
void insertAndExtend(int new_element, int **array, int *size, int *reservedMemory);
void remove(int *array, int *size, int element);

int same_array(int *array1, int *array2, int size);
void copy_from_to(int *from, int *to, int size);
int hamming_dist(int *ass1, int*ass2, int size);

int sample_from_probs(double *probs, int num_elements);
int sample_from_scores(double *scores, int num_elements, double base);

void output(int num, const int* array);
void output(int num, double* array);
bool subset(int num1, const int* array1, int num2, const int* array2);
void addAllToFrom(int* array1, int* size1, const int* array2, const int size2);
void addAllToFrom(int* array, int* size, vector<int> vec);
void addAllToFrom(vector<int> *vec, int* array, int size);

}  // sls4mpe

#endif
