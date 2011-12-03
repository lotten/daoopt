#include "sls4mpe/my_set.h"
#include <stdio.h>
#include <vector>

#include "sls4mpe/random_numbers.h" // for sampling.
#include <stdlib.h> // for exit
#include <math.h> // for pow
using namespace std;

namespace sls4mpe {

/********************************************************
         DATA STRUCTURE FOR SET
 ********************************************************/
int contains(const int *array, int size, int element){ // call by value (size)
	for(int i=0; i<size; i++){
		if(array[i] == element) return 1;
	}
	return 0;
}

bool contains(vector<int> vec, int element){
	for(int i=0; i<vec.size(); i++){
		if(vec[i] == element) return true;
	}
	return false;
}

int contains2(int *array, int size, int element){ // call by value (size)
	int i;
	for(i=0; i<size; i++){
		if(array[i] == element){
			int j;
			for(j=i+1; j<size; j++){
				if(array[j] == element) return 1;
			}
			return 0;
		}
	}
	return 0;
}

void insert(int *array, int *size, int new_element){ // call by reference
//	printf("Inserting%d: \n", new_element);
	int i;
	for(i=0; i<(*size); i++){
		if(array[i] == new_element) return;
	}
	array[(*size)++] = new_element;
/*	printf("After putting in %d: [", new_element);
	for(i=0; i<(*size)-1; i++){
		printf("%d, ", array[i]);
	}
	printf("%d]\n", array[(*size)-1]);*/
}

void insert(vector<int> (*vec), int new_element){ // call by reference
	if( !contains((*vec), new_element) ){
		(*vec).push_back(new_element);
	}
}

void remove(int *array, int *size, int element){ // call by reference
//	printf("Deleting %d: \n", element);
	int i;
	for(i=0; i<(*size); i++){
		if(array[i] == element){
			if( *size > 1 ) array[i] = array[(*size)-1];
			(*size)--;

/*			int j;
			printf("After deleting %d: [", element);
			for(j=0; j<(*size)-1; j++){
				printf("%d, ", array[j]);
			}
			if(*size > 0) printf("%d]\n", array[(*size)-1]);
			printf("size after remove: %d\n",(*size));*/

			return;
		}
	}
}

void insertAndExtend(int new_element, int **array, int *size, int *reservedMemory){ // call by reference
//	output((*size), (*array));
//	printf("  -> Inserting %d: into that set of size %d\n", new_element, (*size));
	int i;
	for(i=0; i<(*size); i++){
		if((*array)[i] == new_element){
//			printf("contained. Return\n");
			return;
		}
	}

	//=== If the set is too large, create a new and bigger one.
	if ((*reservedMemory) == (*size)){
//		printf("extending size from %d to %d\n", (*size), (*reservedMemory)*2);
		int* newArray = new int[(*reservedMemory)*2];
		for(i=0; i<(*size); i++) newArray[i] = (*array)[i];
// one should delete the old array. Throws error, though!
//	delete[] (*array);
		(*array) = newArray;
		(*reservedMemory) *= 2;
	}

	(*array)[(*size)++] = new_element;

/*
	printf("After putting in %d: with size %d [", new_element, (*size));
	for(i=0; i<(*size)-1; i++){
		printf("%d, ", (*array)[i]);
	}
	printf("%d]\n", (*array)[(*size)-1]);
	printf("Done\n");
*/
}

int same_array(int *array1, int *array2, int size){
	for(int i=0; i<size; i++){
		if(array1[i] != array2[i]) return 0;
	}
	return 1;
}

void copy_from_to(int *from, int *to, int size){
	for(int i=0; i<size; i++) to[i] = from[i];
}

int hamming_dist(int *ass1, int*ass2, int size){
	int result = 0;
	for(int i=0; i<size; i++) 
		if(ass1[i] != ass2[i]) result++;
	return result;
}


int sample_from_probs(double *probs, int num_elements){
	double sum=0;
	int i;
	for(i=0; i<num_elements; i++) sum+= probs[i];
	double randnum = ran01(&seed) * sum;
	sum = 0.0;
	for(i=0; i<num_elements; i++){
		sum+= probs[i];
		if( randnum <= sum ) break;
	}
	if( i>= num_elements ){
		printf("Error in sampling, didn't sample any of the values, sum=%lf\n, randnum=%lf", sum, randnum);
		exit(-1);
	}
	return i;
}

int sample_from_scores(double* scores, int num_elements, double base){
	double *probs = new double[num_elements];
	for(int i=0; i<num_elements; i++){
		probs[i] = pow(base, scores[i]);
//		printf("%.2lf ", scores[i]);
	}
	int result = sample_from_probs(probs,num_elements);
//	printf(" --> index %d, soore %.5lf\n", result, scores[result]);
	delete probs;
	return result;
}

void output(int num, const int* array){
	printf("[");
	for(int i=0; i<num-1; i++){
		printf("%d ", array[i]);
	}
	if(num>0)	printf("%d]", array[num-1]);
	else printf("]");
}

void output(int num, double* array){
	printf("[");
	for(int i=0; i<num-1; i++){
		printf("%lf ", array[i]);
	}
	if(num>0)	printf("%lf]", array[num-1]);
	else printf("]");
}

bool subset(int num1, const int* array1, int num2, const int* array2){
	for(int i=0; i<num1; i++){
		if(!contains(array2, num2, array1[i])) return false;
	}
	return true;
}

void addAllToFrom(int* array1, int* size1, const int* array2, const int size2){
	for(int i=0; i<size2; i++) insert(array1, size1, array2[i]);
}

void addAllToFrom(int* array, int* size, vector<int> vec){
	for(int i=0; i<vec.size(); i++) insert(array, size, vec[i]);
}

void addAllToFrom(vector<int> *vec, int* array, int size){
	for(int i=0; i<size; i++) insert(vec, array[i]);
}

/*void setDifference(int num1, const int* array1, int num2, const int* array2, int* result){
	int numResult = num1;
	int* result = new int[numResult];
	copy_from_to(array1, result, numResult);
	for(j=0; j<num2; j++)	remove(result, &numResult, array2[j]);
}

void without(int num, const int* array, int element){
	int numResult = num;
	int* result = new int[numResult];
	copy_from_to(array, result, numResult);
	remove(result, numResult, element);
}*/

}  // sls4mpe
