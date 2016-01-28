#ifndef DEBUG_SVC_H
#define DEBUG_SVC_H
/**	\file debug_svc.h
		\brief Файл с настройками отладочного сервиса

		Основные функции:
		-# Включение/Выключение отладочных сообщений
		-# Включение/Выключение отладочного сохранения в файл промежуточных изображений
		-# Включение/отключение библиотеки Dalsa для распознавания
*/


/**	\def PRINT_DEBUG
		\brief Макрос задает включение/отключение вывода отладочных сообщений в консоль
*/
#define PRINT_DEBUG 1

/**	\def USE_DUMP_CV_IMAGE
		\brief Макрос задает включение/отключение отладочного сохранения в файл промежуточных изображений

		ДЛЯ отладочного сохранения  в файле объектов IplImage установить макрос USE_DUMP_CV_IMAGE 1. 
		В тексте программы для сохранения напр. src1 написать DUMP_CV_IMAGE("src1.jpg",src1).
		Для отключения отладочного сохранения установить USE_DUMP_CV_IMAGE 0. 
		В текстах макрос DUMP_CV_IMAGE комментировать после отключения не нужно - компилироваться не будет.
*/
#define USE_DUMP_CV_IMAGE 0

#if (USE_DUMP_CV_IMAGE != 0)

/**	\def DUMP_CV_IMAGE(file_name,image)
		\brief Макрос, выполняющий сохранение изображения в файл, если USE_DUMP_CV_IMAGE равен 1
*/
	#define DUMP_CV_IMAGE(file_name,image)	cvSaveImage((file_name),(image));
#else

/**	\def DUMP_CV_IMAGE(file_name,image)
		\brief Макрос для сохранения отладочного изображения
*/
	#define DUMP_CV_IMAGE(file_name,image)
#endif


/**	\def EMUL
		\brief Макрос, задает работу в режиме эмуляции и подсчет верно распознанных кадров
*/
#define EMUL 0

#if (EMUL == 1)

//Большой тестовый набор №1 - полувагоны и цистерны из Рефти
#include "emul.h"

#endif


#if (EMUL == 2)

//Тестовый набор №2 - различные сложные цистерны
#include "emul2.h"

#endif


#endif