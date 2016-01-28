/**	\file RecogTask.h
	\brief ������������ ���� � ��������� ������ ������ �������������
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


#define MAX_IMG_COUNT 512	///<������������ ���-�� �����������

#define IMG_WIDTH 120
#define IMG_HEIGHT 120

//CvVideoWriter *writer = 0;



///���������, ���������� ������ � ������������ �������
struct faces_main
{
	int img_id;
	char name[64];
};



/** \struct recog_pars	
       \brief ��������� ������������ 
 */
struct recog_pars
{

	double face_thresh;
	int channels;
} ;

/**
	\class RecogTask
	\brief ����� ������ ��� �������������
*/
class  RecogTask : public Runnable
{
	recog_pars & rec_p_;

public:
	RecogTask:: RecogTask(recog_pars &cpr):rec_p_(cpr)
	{
	}


private:

	virtual void run();		///<������ ������ ������������� ������

    void training(void); ///<��������
	int recognize(void); //�������������
    void PCA(void); //����� ������� ���������
    void storeTrainingData(void);
    int loadTrainingData();
    int findNearestNeighbor(float* projectedTestFace);
    int loadFaces(); //�������� �����������
    void detectFaces(IplImage* img, int ident);//����������� ���
	int	loadBase(); //�������� ���� �����������
	void SendNewFace(int id, int x, int y);
	int addFace(char *name); //�������� ����� ����

    IplImage ** faceImgArr; // ������ ����������� ���
    CvMat    *  personNumTruthMat;
    int TrainFaces_count; // ���-�� ��� ��� ����������
    int Eigens_count; // ���-�� ����������� ��������
    IplImage * pAvgTrainImg; // ����������� �����������
    IplImage ** eigenVectArr; // ����������� �������
    CvMat * eigenValMat; // ����������� ��������
    CvMat * projectedTrainFaceMat; // ��������������� ����
    CvHaarClassifierCascade *cascade_f; //�������������
    CvMemStorage *storage; //��������� storage
	IplImage* testImg;
	CvMat * trainPersonNumMat;
	

};

#endif