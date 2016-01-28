#ifndef DEBUG_SVC_H
#define DEBUG_SVC_H
/**	\file debug_svc.h
		\brief ���� � ����������� ����������� �������

		�������� �������:
		-# ���������/���������� ���������� ���������
		-# ���������/���������� ����������� ���������� � ���� ������������� �����������
		-# ���������/���������� ���������� Dalsa ��� �������������
*/


/**	\def PRINT_DEBUG
		\brief ������ ������ ���������/���������� ������ ���������� ��������� � �������
*/
#define PRINT_DEBUG 1

/**	\def USE_DUMP_CV_IMAGE
		\brief ������ ������ ���������/���������� ����������� ���������� � ���� ������������� �����������

		��� ����������� ����������  � ����� �������� IplImage ���������� ������ USE_DUMP_CV_IMAGE 1. 
		� ������ ��������� ��� ���������� ����. src1 �������� DUMP_CV_IMAGE("src1.jpg",src1).
		��� ���������� ����������� ���������� ���������� USE_DUMP_CV_IMAGE 0. 
		� ������� ������ DUMP_CV_IMAGE �������������� ����� ���������� �� ����� - ��������������� �� �����.
*/
#define USE_DUMP_CV_IMAGE 0

#if (USE_DUMP_CV_IMAGE != 0)

/**	\def DUMP_CV_IMAGE(file_name,image)
		\brief ������, ����������� ���������� ����������� � ����, ���� USE_DUMP_CV_IMAGE ����� 1
*/
	#define DUMP_CV_IMAGE(file_name,image)	cvSaveImage((file_name),(image));
#else

/**	\def DUMP_CV_IMAGE(file_name,image)
		\brief ������ ��� ���������� ����������� �����������
*/
	#define DUMP_CV_IMAGE(file_name,image)
#endif


/**	\def EMUL
		\brief ������, ������ ������ � ������ �������� � ������� ����� ������������ ������
*/
#define EMUL 0

#if (EMUL == 1)

//������� �������� ����� �1 - ���������� � �������� �� �����
#include "emul.h"

#endif


#if (EMUL == 2)

//�������� ����� �2 - ��������� ������� ��������
#include "emul2.h"

#endif


#endif