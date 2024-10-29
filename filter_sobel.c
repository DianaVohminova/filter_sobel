#include <math.h>
#include <stdio.h>		 // библиотека для ввода/вывода
#include <stdlib.h> 		//функции для управления памятью
#include <pthread.h> 		//для работы с потоками
#include <time.h> 		//работа со временем
// библиотеки для работы с изображениями в формате jpg
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Структура для передачи данных потокам
typedef struct 
{
unsigned char *image;
unsigned char *resultImage;
int width; 
int height;
int startLine; 
int endLine;
} ThreadData;

// Функция фильтра Собела для потока
void *Sobel_Filter(void *arg)
{
	ThreadData *data = (ThreadData *)arg;
	for (int y = data->startLine; y < data->endLine; y++)
	{
		for (int x = 1; x < data->width - 1; x++)
		{
			int gx = (data->image[(y - 1) * data->width + (x + 1)] 
			+ 2 * data->image[y * data->width + (x + 1)] 
			+ data->image[(y + 1) * data->width + (x + 1)])
			- (data->image[(y - 1) * data->width + (x - 1)] 		
			+ 2 * data->image[y * data->width + (x - 1)] 
			+ data->image[(y + 1) * data->width + (x - 1)]);

			int gy = (data->image[(y + 1) * data->width + (x - 1)] 
			+ 2 * data->image[(y + 1) * data->width + x] 
			+ data->image[(y + 1) * data->width + (x + 1)])
			- (data->image[(y - 1) * data->width + (x - 1)]
			+ 2 * data->image[(y - 1) * data->width + x] 
			+ data->image[(y - 1) * data->width + (x + 1)]);

			int sum = sqrt(gx * gx + gy * gy); // вычисление модуля градиента
			if (sum > 255) sum = 255;
			if (sum < 0) sum = 0;
			data->resultImage[y* data->width + x] = sum;
		}//for x
	}//for y
	pthread_exit(NULL);
}//void

int main()
{
	int threadNumber = 1; // количество потоков
	FILE *fout = fopen("res.txt", "a"); // файл для записи результатов
	const char *path = "/home/diana/Документы/labs/l4/kitty.jpg"; // путь к файлу
	int width, height, channels;
	unsigned char *image = stbi_load(path, &width, &height, &channels, 1);
	if (image == NULL)
	{
		fprintf(stderr, "Error loading image '%s': %sn", path, stbi_failure_reason());
		exit(1);
	}
	unsigned char *res_im = (unsigned char *)malloc(width * height * sizeof(unsigned char));
	// Разбиение на отрезки
	int size = (height - 1) / threadNumber;
	int top = 1;
	int bottom = size;

	// начало отсчета времени
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	// Создание потоков
	pthread_t threads[threadNumber];
	ThreadData threadData[threadNumber];
	for (int i = 0; i < threadNumber; i++) {
		threadData[i].image = image;
		threadData[i].resultImage = res_im;
		threadData[i].width = width;
		threadData[i].height = height;
		threadData[i].startLine = top;
		threadData[i].endLine = bottom;
		pthread_create(&threads[i], NULL, Sobel_Filter, (void *)&threadData[i]);
		top = bottom;
		bottom += size;
	}//for
	//ожидание завершения потоков
	for(int i = 0; i < threadNumber; i++) {
		pthread_join(threads[i], NULL);
	}//for
	// завершение посчета времени
	clock_gettime(CLOCK_MONOTONIC, &end);
	double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	// печать результата выполнения программы в консоль и в файл
	printf("%d threads -- %.3f ms\n", threadNumber, seconds * 1000);
	fprintf(fout, "%d threads -- %.3f ms\n", threadNumber, seconds * 1000);
	stbi_write_jpg("result.jpg", width, height, 1, res_im, width); // вывод результата изображения
	stbi_image_free(image);
	free(res_im);
	fclose(fout);
	return 0;
}//main
