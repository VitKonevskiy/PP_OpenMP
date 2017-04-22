#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <omp.h> 
using namespace std;


#define RAND_MAX_DOUBLE 1000

#define PRINT_ARRAY 0
#define PRINT_FOR_COMPARE 0
#define N 40000
#define N_THREADS 4

// Сортировка подсчетом для типа double по i-му байту
void CountingSort(double* arr_inp, double* arr_out, int size_arr, int byte_num, int start_index, int part_of_thread, int *counter, int offset, int j, int tmp)
{
	// Идея следующая:
	// Каждый байт может иметь 256 различных состояний (диапазон чисел от 0 до 255)
	// Пусть есть байты А и В с номерами состояний s_A и s_B соответственно
	// Байт A больше байта B, если s_A > s_B
	unsigned char* mas = (unsigned char*)arr_inp;

	//int counter[256];// Возможно 256 различных состояний одного байта
	//int offset;
	for (j = 0; j < 256; j++) 
	{
		counter[j] = 0;
	}
	//memset(counter, 0, sizeof(int) * 256); // Обнуляем массив
		// mas возвращает символ с кодом от 0 до 255 (иными словами номер состояния байта)
	for (int i = 0; i < part_of_thread; i++)
		counter[mas[8 * i + byte_num]]++; // counter показывает, сколько чисел типа double содержит определенный разряд

	// Теперь ищем номер состояния байта byte_num, который присутствует в каких-либо элементах double 
	//int j = 0;
	j = 0;
	for (j; j < 256; j++)
		if (counter[j] != 0)
			break;

	offset = counter[j];// Теперь offset показывает, сколько имеется элементов с определенным байтом (чтобы определить, сколько ячеек массива arr_out уйдет под числа,
						// содержащих байт с номером состояния j)
	counter[j] = 0;// Это характеризует смещение элементов, содержащих байт с номером состояния j. Причем такие элементы будут иметь "наименьший байт" и будут записаны в начале массива arr_out
	j++;

	// Далее считаем смещения и записываем их в counter

	for (j; j < 256; j++)
	{
		tmp = counter[j];
		counter[j] = offset;
		offset += tmp;
	}

	for (int i = 0; i < part_of_thread; i++)
	{
		arr_out[counter[mas[8 * i + byte_num]]] = arr_inp[i];// counter содержит всю необходимую информацию по корректному раскидыванию элементов
		counter[mas[8 * i + byte_num]]++;// Увеличиваем смещение на 1 для элемента (чтобы корректно его записать в ячейку массива arr_out)
	}
}


void LSDSortDouble(double* arr_inp, int start_index, int part_of_thread, int size_arr, double * arr_inp_tmp, double* arr_out_tmp,int *counter,int offset,int j,int tmp)
{
	// Создадим два массива, которые будут содержать по отдельности отрицательные и положительные элементы
	// Для каждого из них по 8 раз запустим соответсвующую сортировку подсчетом
	// После этого сливаем эти массивы и получаем результат


	for (int i = start_index; i < start_index + part_of_thread; i++)
		arr_inp_tmp[i-start_index] = arr_inp[i];

	// Сортируем массив

	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 0, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 1, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 2, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 3, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 4, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 5, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 6, start_index, part_of_thread, counter, offset, j, tmp);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 7, start_index, part_of_thread, counter, offset, j, tmp);
	//Записываем результат
	for (int i = start_index; i < start_index + part_of_thread; i++)
		arr_inp[i] = arr_inp_tmp[i-start_index];

}


void Collect(double *array, double *tmpArray, int start_index, int part_of_thread, int curr_step, int a,int b,int Size1,int Size2,int j)
{
	a = start_index;
	b = start_index + part_of_thread;
	Size1 = start_index + part_of_thread;
	Size2 = b + part_of_thread;
	j = start_index;
	if ((omp_get_thread_num() % curr_step) == 0)				//При выборе curr step =2 :  0  2  4  6  8 
	{
		while ((a != Size1) && (b != Size2))
		{
			if (array[a] <= array[b])
			{
				tmpArray[j] = array[a];
				a++;
			}
			else
			{
				tmpArray[j] = array[b];
				b++;
			}
			j++;
		}
		//Дописываем остаток
		if (a == Size1)
			for (j = b; j < Size2; j++)
				tmpArray[j] = array[j];
		else
			for (j = a; j < Size1; j++)
				tmpArray[j+ part_of_thread] = array[j];
	}
}


// Сортировка подсчетом для типа double по i-му байту для положительных чисел
void CountingSort_NonParallel(double* arr_inp, double* arr_out, int size_arr, int byte_num)
{
	// Идея следующая:
	// Каждый байт может иметь 256 различных состояний (диапазон чисел от 0 до 255)
	// Пусть есть байты А и В с номерами состояний s_A и s_B соответственно
	// Байт A больше байта B, если s_A > s_B
	unsigned char* mas = (unsigned char*)arr_inp;

	int counter[256];// Возможно 256 различных состояний одного байта
	int offset;

	memset(counter, 0, sizeof(int) * 256); // Обнуляем массив

										   // mas возвращает символ с кодом от 0 до 255 (иными словами номер состояния байта)
	for (int i = 0; i < size_arr; i++)
		counter[mas[8 * i + byte_num]]++; // counter показывает, сколько чисел типа double содержит определенный разряд

										  // Теперь ищем номер состояния байта byte_num, который присутствует в каких-либо элементах double 
	int j = 0;
	for (j; j < 256; j++)
		if (counter[j] != 0)
			break;

	offset = counter[j];// Теперь offset показывает, сколько имеется элементов с определенным байтом (чтобы определить, сколько ячеек массива arr_out уйдет под числа,
						// содержащих байт с номером состояния j)
	counter[j] = 0;// Это характеризует смещение элементов, содержащих байт с номером состояния j. Причем такие элементы будут иметь "наименьший байт" и будут записаны в начале массива arr_out
	j++;

	// Далее считаем смещения и записываем их в counter

	for (j; j < 256; j++)
	{
		int tmp = counter[j];
		counter[j] = offset;
		offset += tmp;
	}

	for (int i = 0; i < size_arr; i++)
	{
		arr_out[counter[mas[8 * i + byte_num]]] = arr_inp[i];// counter содержит всю необходимую информацию по корректному раскидыванию элементов
		counter[mas[8 * i + byte_num]]++;// Увеличиваем смещение на 1 для элемента (чтобы корректно его записать в ячейку массива arr_out)
	}

}

void LSDSortDouble_NonParallel(double* arr_inp, int size_arr)
{
	// Создадим два массива, которые будут содержать по отдельности отрицательные и положительные элементы
	// Для каждого из них по 8 раз запустим соответсвующую сортировку подсчетом
	// После этого сливаем эти массивы и получаем результат

	double* arr_inp_tmp;
	double* arr_out_tmp;
	int size_arr_plus = 0,
		size_arr_minus = 0;

	arr_inp_tmp = new double[size_arr];
	arr_out_tmp = new double[size_arr];

	for (int i = 0; i < size_arr; i++)
		arr_inp_tmp[i] = arr_inp[i];

	// Сортируем массив
	CountingSort_NonParallel(arr_inp_tmp, arr_out_tmp, size_arr, 0);
	CountingSort_NonParallel(arr_out_tmp, arr_inp_tmp, size_arr, 1);
	CountingSort_NonParallel(arr_inp_tmp, arr_out_tmp, size_arr, 2);
	CountingSort_NonParallel(arr_out_tmp, arr_inp_tmp, size_arr, 3);
	CountingSort_NonParallel(arr_inp_tmp, arr_out_tmp, size_arr, 4);
	CountingSort_NonParallel(arr_out_tmp, arr_inp_tmp, size_arr, 5);
	CountingSort_NonParallel(arr_inp_tmp, arr_out_tmp, size_arr, 6);
	CountingSort_NonParallel(arr_out_tmp, arr_inp_tmp, size_arr, 7);

	//Записываем результат
	for (int i = 0; i < N; i++)
		arr_inp[i] = arr_inp_tmp[i];
}

static int compare(const void * a, const void * b)
{
	if (*(double*)a > *(double*)b) return 1;
	else if (*(double*)a < *(double*)b) return -1;
	else return 0;
}


int main()
{
	int count = 0;

	srand(time(0));
	double *array = new double[N];
	double *TestArray = new double[N];
	double *NonParallelArray = new double[N];
	double *tmpArray = new double[N];

	bool TrueResult = true;
	for (int i = 0; i < N; i++)
	{
		array[i] = (double)(rand()) / RAND_MAX * RAND_MAX_DOUBLE;
		TestArray[i] = array[i];
		NonParallelArray[i] = array[i];
		if (PRINT_ARRAY)
		{
			printf("%lf \n", array[i]);
		}
	}


	double parralel_time_start = omp_get_wtime();
	int n;
	int start_index;


	double* arr_inp_tmp;
	double* arr_out_tmp;


	int j = 0;
	int tmp = 0;
	int *counter;
	int offset = 0;
	int a, b, Size1, Size2;

#pragma omp parallel num_threads(N_THREADS) private(start_index,offset,j,tmp,counter,arr_inp_tmp,arr_out_tmp,a,b,Size1,Size2) 
	{
		counter = new int[256];
		n = N / omp_get_num_threads();
		start_index = n*omp_get_thread_num();
		offset = 0;
		j = 0;
		tmp = 0;
		arr_out_tmp = new double[n];
		arr_inp_tmp = new double[n];

		a = start_index;
		b = 0;
		Size1 = a + n;
		Size2 = b + n;


		LSDSortDouble(array, start_index, n, N, arr_inp_tmp, arr_out_tmp, counter, offset, j, tmp);

#pragma omp barrier
		Collect(array, tmpArray, start_index, n, 2, a, b, Size1, Size2, j);			//После выполнения функции, ведущие потоки :   0	2	4	6
#pragma omp barrier
		Collect(tmpArray, array, start_index, 2 * n, 4, a, b, Size1, Size2, j);		//После выполнения функции, ведущие потоки :   0	4	
//#pragma omp barrier
//		Collect(array, tmpArray, start_index, 4 * n, 8, a, b, Size1, Size2, j);		//После выполнения функции, ведущие потоки :   0

		delete[]counter;
		delete[]arr_inp_tmp;
		delete[]arr_out_tmp;
	}
	double parralel_time_end = omp_get_wtime();



	/*---------Последовательная версия алгоритма-----------*/
	double NonParralel_time_start = omp_get_wtime();
	LSDSortDouble_NonParallel(NonParallelArray, N);
	double NonParralel_time_end = omp_get_wtime();
	/*---------Последовательная версия алгоритма-----------*/

	/*---------Выполняем проверку LSD сортировки с qsort-----------*/
	qsort(TestArray, N, sizeof(double), compare);

	if (PRINT_FOR_COMPARE)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%d   %lf     %lf\n", i, array[i], TestArray[i]);
		}
	}

	for (int i = 0; i < N; i++)
	{
		if (array[i] != TestArray[i])
		{
			TrueResult = false;
			if (PRINT_FOR_COMPARE)
			{
				printf("ERROR in %d \t", i);
				printf("%lf     %lf\n\n", array[i], TestArray[i]);
			}
		}
	}
	printf("\n Qsort =? LSD   %d \n", TrueResult);
	/*---------Выполняем проверку LSD сортировки с qsort-----------*/

	printf("\n Parallel Time =     %lf \n", parralel_time_end - parralel_time_start);
	printf("\n NON Parallel Time = %lf \n", NonParralel_time_end - NonParralel_time_start);
//	printf("\n Boost= %lf", (NonParralel_time_end - NonParralel_time_start) / (parralel_time_end - parralel_time_start));

	delete[]array;
	delete[]TestArray;
	delete[]NonParallelArray;

	system("pause");
	return 0;
}