/**	\file RecogTask.h
	\brief Заголовочный файл с описанием класса потока распознавания
*/

#ifndef RECOG_TASK
#define RECOG_TASK

#include <fstream>
#include <stdio.h>
#include <iostream>

#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/Net/DatagramSocket.h"

#include "sqlite3.h"

#include "opencv/cv.h"
#include "opencv/cvaux.h"
#include "opencv/highgui.h"


#include "debug_svc.h"

using std::string;
using Poco::Runnable;

using namespace Poco::Net;
using Poco::Net::DatagramSocket;


#define MAX_IMG_COUNT 512	///<Максимальное кол-во изображений

#define IMG_WIDTH 120
#define IMG_HEIGHT 120

//CvVideoWriter *writer = 0;



///Структура, содержащая шаблон с определенным номером
struct faces_main
{
	int img_id;
	char name[64];
};



/** \struct recog_pars	
       \brief Параметры конфигурации 
 */
struct recog_pars
{

	double face_thresh;
	int channels;
} ;

/**
	\class RecogTask
	\brief Класс потока для распознавания
*/
class  RecogTask : public Runnable
{
	recog_pars & rec_p_;

public:
	RecogTask:: RecogTask(recog_pars &cpr):rec_p_(cpr)
	{
	}


private:

	virtual void run();		///<Запуск потока распознавания кадров

    void training(void); ///<Обучение
	int recognize(void); //распознавание
    void PCA(void); //метод главных компонент
    void storeTrainingData(void);
    int loadTrainingData();
    int findNearestNeighbor(float* projectedTestFace);
    int loadFaces(); //загрузка изображений
    void detectFaces(IplImage* img, int ident);//обнаружение лиц
	int	loadBase(); //загрузка базы изображений
	void SendNewFace(int id, int x, int y);
	int addFace(char *name); //добавить новое лицо

    IplImage ** faceImgArr; // массив изображений лиц
    CvMat    *  personNumTruthMat;
    int TrainFaces_count; // кол-во лиц для тренировки
    int Eigens_count; // кол-во собственных векторов
    IplImage * pAvgTrainImg; // усредненное изображение
    IplImage ** eigenVectArr; // собственные вектора
    CvMat * eigenValMat; // собственные значения
    CvMat * projectedTrainFaceMat; // спроецированные лица
    CvHaarClassifierCascade *cascade_f; //классификатор
    CvMemStorage *storage; //структура storage
	IplImage* testImg;
	CvMat * trainPersonNumMat;
	

};

#endif