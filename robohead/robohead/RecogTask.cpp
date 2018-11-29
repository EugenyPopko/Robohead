/**	\file RecogTask.cpp
	\brief Файл с реализацией класса потока распознавания
*/
#include "RecogTask.h"
#include "ReciverTask.h"


#include <deque>
#include "Poco/Mutex.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include "Poco/AutoPtr.h"
#include "Poco/Logger.h" 
#include "Poco/FileChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/FormattingChannel.h"
#include "Poco/Message.h"
#include "Poco/Format.h" 



#include <io.h>
#include <time.h>

#include <sys\stat.h>
//#include <fstream>
//#include <iostream>


using Poco::Mutex;
using std::deque;



using Poco::AutoPtr;
using Poco::Logger;
using Poco::Channel;
using Poco::FileChannel;
using Poco::FormattingChannel;
using Poco::Formatter;
using Poco::PatternFormatter;

using namespace std;


///Array of images
faces_main facesDB[MAX_IMG_COUNT]; 

extern Mutex mut_new_data;
extern deque<MessageFromServo> deq;

/*****************************************************************************/
//Load DB SQLite
int RecogTask::loadBase()
{
    int rc;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *sql = "select * from faces_main;";
    const char *tail;
	int i=0;

    rc = sqlite3_open("db/faces.db", &db);

    if(rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    rc = sqlite3_prepare(db, sql, (int)strlen(sql), &stmt, &tail);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    }
    
    rc = sqlite3_step(stmt);

    while(rc == SQLITE_ROW) {
        
		facesDB[i].img_id = sqlite3_column_int(stmt, 0);
		sprintf_s(facesDB[i].name,64, "%s", sqlite3_column_text(stmt, 1));

		printf("%d\t%s\n",facesDB[i].img_id,facesDB[i].name);

        rc = sqlite3_step(stmt);
		i++;
    }

    sqlite3_finalize(stmt);

    sqlite3_close(db);

	return i; //возвращаем число записей
}

/*****************************************************************************/
//Add face and name
int RecogTask::addFace(char *name)
{
    sqlite3 *db;
    int rc;
	sqlite3_stmt *stmt;
	const char* tail;
    char *zErr;
	char* data;

    rc = sqlite3_open("db/faces.db", &db);

    if(rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    
    const char* sql = "insert into faces_main (img_id, name) values (NULL, ?100);";

    rc = sqlite3_prepare(db, sql, (int)strlen(sql), &stmt, &tail);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare() : Error: %s\n", tail);
        return rc;
    }

    sqlite3_bind_text(stmt, 100, name, (int)strlen(name), SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_close(db);

	facesDB[TrainFaces_count].img_id=TrainFaces_count+1;
	sprintf_s(facesDB[TrainFaces_count].name,64,"%s",name);

	TrainFaces_count++;
	printf("New face,TrainFaces_count=%d\n",TrainFaces_count);

	return 0;
}

/*****************************************************************************/
//Load data from facedata.xml
int RecogTask::loadTrainingData()
{
    CvFileStorage * fileStorage;

    //open file
    fileStorage = cvOpenFileStorage( "facedata.xml", 0, CV_STORAGE_READ );
    if( !fileStorage )
    {
        fprintf(stderr, "Can't open facedata.xml\n");
        return 0;
    }

    Eigens_count = cvReadIntByName(fileStorage, 0, "nEigens", 0);
    TrainFaces_count = cvReadIntByName(fileStorage, 0, "nTrainFaces", 0);
	trainPersonNumMat = (CvMat *)cvReadByName(fileStorage, 0, "trainPersonNumMat", 0);
    eigenValMat  = (CvMat *)cvReadByName(fileStorage, 0, "eigenValMat", 0);
    projectedTrainFaceMat = (CvMat *)cvReadByName(fileStorage, 0, "projectedTrainFaceMat", 0);
    pAvgTrainImg = (IplImage *)cvReadByName(fileStorage, 0, "avgTrainImg", 0);
    eigenVectArr = (IplImage **)cvAlloc(TrainFaces_count*sizeof(IplImage *));
    for(int i=0; i<Eigens_count; i++)
    {
        char varname[256];
        sprintf( varname, "eigenVect_%d", i );
        eigenVectArr[i] = (IplImage *)cvReadByName(fileStorage, 0, varname, 0);
    }

    cvReleaseFileStorage( &fileStorage );

    return 1;
}

/*****************************************************************************/
//Load images of faces
int RecogTask::loadFaces()
{
    char imgFilename[512];
    int iFace, nFaces=0;

	//load base
	nFaces=loadBase();

    faceImgArr        = (IplImage **)cvAlloc( nFaces*sizeof(IplImage *) );
    personNumTruthMat = cvCreateMat( 1, nFaces, CV_32SC1 );

	char buf[128];

    for(iFace=0; iFace<nFaces; iFace++)
    {
		
		memcpy(personNumTruthMat->data.i+iFace,&facesDB[iFace].img_id,sizeof(int));

		sprintf_s(imgFilename,"img/%s%d.bmp",facesDB[iFace].name,facesDB[iFace].img_id);

        // загружаем изображение
		IplImage * img=cvLoadImage(imgFilename, CV_LOAD_IMAGE_GRAYSCALE);

		faceImgArr[iFace]=cvCreateImage(cvGetSize(img),IPL_DEPTH_8U,1);

		cvEqualizeHist(img,faceImgArr[iFace]);

		cvReleaseImage(&img);

        if( !faceImgArr[iFace] )
        {
            fprintf(stderr, "Can\'t load image from %s\n", imgFilename);
            return 0;
        }
    }

    return nFaces;
}

/*****************************************************************************/
//Поиск ближайшего соседа
int RecogTask::findNearestNeighbor(float* projectedTestFace)
{
    float leastDistSq = DBL_MAX;
    int iTrain, iNearest = 0;

    for(iTrain=0; iTrain<TrainFaces_count; iTrain++) //по всем лицам
    {
        float distSq=0;
        for(int i=0; i<Eigens_count; i++){
            float d_i = projectedTestFace[i] - projectedTrainFaceMat->data.fl[iTrain*Eigens_count + i];
            distSq += d_i*d_i; //расстояние
        }

        if(distSq < leastDistSq){
            leastDistSq = distSq;
            iNearest = iTrain;
        }
    }

    if(rec_p_.face_thresh < leastDistSq) //должно быть больше порога
    {
        iNearest = -1;
    }

    return iNearest;
}
/*****************************************************************************/
//save data in facedata.xml
void RecogTask::storeTrainingData(void)
{
    CvFileStorage * fileStorage;

    fileStorage = cvOpenFileStorage( "facedata.xml", 0, CV_STORAGE_WRITE );

    cvWriteInt( fileStorage, "nEigens", Eigens_count);
    cvWriteInt( fileStorage, "nTrainFaces", TrainFaces_count );
    cvWrite(fileStorage, "trainPersonNumMat", personNumTruthMat, cvAttrList(0,0));
    cvWrite(fileStorage, "eigenValMat", eigenValMat, cvAttrList(0,0));
    cvWrite(fileStorage, "projectedTrainFaceMat", projectedTrainFaceMat, cvAttrList(0,0));
    cvWrite(fileStorage, "avgTrainImg", pAvgTrainImg, cvAttrList(0,0));

    for(int i=0; i<Eigens_count; i++){
        char varname[200];
        sprintf( varname, "eigenVect_%d", i );
        cvWrite(fileStorage, varname, eigenVectArr[i], cvAttrList(0,0));
    }

    cvReleaseFileStorage( &fileStorage );
}

/*****************************************************************************/
//Метод главных компонент
void RecogTask::PCA(void)
{
    CvTermCriteria term_crit;
    CvSize faceImgSize;

    Eigens_count = TrainFaces_count-1; //кол-во собственных значений

    faceImgSize.width  = faceImgArr[0]->width;
    faceImgSize.height = faceImgArr[0]->height;
    eigenVectArr = (IplImage**)cvAlloc(sizeof(IplImage*) * Eigens_count);

    for(int i=0; i<Eigens_count; i++) eigenVectArr[i] = cvCreateImage(faceImgSize, IPL_DEPTH_32F, 1);

    eigenValMat = cvCreateMat( 1, Eigens_count, CV_32FC1 );
    pAvgTrainImg = cvCreateImage(faceImgSize, IPL_DEPTH_32F, 1);

    //критерий остановки
	term_crit = cvTermCriteria(CV_TERMCRIT_ITER, Eigens_count, 1);

	//Метод главных компонент (Principal Component Analysis, PCA) 
	//применяется для сжатия информации без существенных потерь информативности.

	//nTrainFaces - число эталонов, 
	//faceImgArr - указатель на массив изображений-эталонов,
	//eigenVectArr – (выход функции) указатель на массив собственных объектов (изображения глубиной 32 бит)
	//CV_EIGOBJ_NO_CALLBACK – флаги ввода/вывода. Для работы с памятью.
	//0-размер буфера. Для работы с памятью.
	//0-указатель на структуру для работы с памятью.
	//calcLimit-критерий прекращения вычислений. Два варианта: по количеству итераций и по ко точности
	//pAvgTrainImg-(выход функции) усредненное изображение эталонов
	//eigenValMat->data.fl-указатель на собственные числа (может быть NULL)
    cvCalcEigenObjects(
        TrainFaces_count,
        (void*)faceImgArr,
        (void*)eigenVectArr,
        CV_EIGOBJ_NO_CALLBACK,
        0,
        0,
		&term_crit,
        pAvgTrainImg,
        eigenValMat->data.fl);

    cvNormalize(eigenValMat, eigenValMat, 1, 0, CV_L1, 0);
}

/*****************************************************************************/
//Recognition
int RecogTask::recognize(void)
{
    
    float * projectedTestFace = 0;
    projectedTestFace = (float *)cvAlloc( Eigens_count*sizeof(float) );

    //функция вычисления компонентов разложения
    cvEigenDecomposite(testImg,Eigens_count,eigenVectArr,0, 0,pAvgTrainImg,projectedTestFace);

	int iNearest, nearest, truth;
    iNearest = findNearestNeighbor(projectedTestFace);
    truth    = personNumTruthMat->data.i[0];
    if(iNearest!= -1) nearest  = trainPersonNumMat->data.i[iNearest];
    else nearest = -1;

    return nearest;
}

/*****************************************************************************/
//Training
void RecogTask::training(void)
{

    TrainFaces_count = loadFaces(); //загружаем массив
	printf("nTrainFaces=%d\n", TrainFaces_count);

    if( TrainFaces_count < 2 ){
		printf("Need 2 or more training faces. Input file contains only %d\n", TrainFaces_count);
        return;
    }

    PCA(); //метод главных компонент
	
    projectedTrainFaceMat = cvCreateMat( TrainFaces_count, Eigens_count, CV_32FC1 );
    int offset = projectedTrainFaceMat->step / sizeof(float);
    for(int i=0; i<TrainFaces_count; i++)
    {
        cvEigenDecomposite(
            faceImgArr[i],
            Eigens_count,
            eigenVectArr,
            0, 0,
            pAvgTrainImg,
            projectedTrainFaceMat->data.fl + i*offset);
    }

    storeTrainingData(); //сохраняем данные

	printf("Training complete\n");
}

/*****************************************************************************/
//Send new face to servo
void RecogTask::SendNewFace(int id, int x, int y)
{
 MessageFromServo msg;

 DatagramSocket send_s;
 SocketAddress sa("localhost", 11002);
 send_s.connect(sa);
 msg.id=id;
 msg.x=x;
 msg.y=y;
 send_s.sendBytes(&msg, sizeof(msg));
 send_s.close();
}

/*****************************************************************************/
//Detect face
void RecogTask::detectFaces(IplImage* img, int ident)
{
    int i,j;
	CvSeq *faces = cvHaarDetectObjects(img, cascade_f, storage, 1.1, 3, CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_DO_ROUGH_SEARCH, cvSize(130, 130)); //классификатор (каскад Хаара)

    if (faces->total == 0) return; //при отсутствии лиц не продолжаем

    CvRect *r = (CvRect*)cvGetSeqElem(faces, 0);
    cvRectangle(img, cvPoint(r->x, r->y),
                cvPoint(r->x + r->width, r->y + r->height),
                CV_RGB(255, 0, 0), 1, 8, 0);

    IplImage* img1 = cvCreateImage(cvSize(r->height, r->width), img->depth, rec_p_.channels);

    int a, b, k;

	//read rect of face
    for (i = r->y, a = 0; i < r->y + r->height, a < r->height; ++i, ++a){
        for (j = r->x, b = 0; j < r->x + r->width, b < r->width; ++j, ++b){
            for ( k = 0; k < rec_p_.channels; ++k){
                CvScalar pix = cvGet2D(img, i, j);
                img1->imageData[a * img1->widthStep + b * rec_p_.channels + k] = (char)pix.val[k];
            }
        }
    }

	char bufstr[64];

    if (ident>0)     // if query for adding new face
    {
		loadBase(); //load base

        IplImage* tmpsize = cvCreateImage(cvSize(IMG_WIDTH,IMG_HEIGHT),img->depth, rec_p_.channels);
        cvResize(img1,tmpsize,CV_INTER_LINEAR);

		sprintf_s(bufstr,64,"img/%s%d.bmp",facesDB[ident-1].name,facesDB[ident-1].img_id);
		printf("save new img %s\n",bufstr);

        cvSaveImage(bufstr, tmpsize);
        training(); //дообучение
		loadTrainingData();

		cvReleaseImage(&tmpsize);
    }

	
	int xf=cvRound((double)r->width*0.5)+r->x;
	int yf=cvRound((double)r->height*0.5)+r->y;
	xf=(100.0*(double)xf)/img->width;
	yf=(100.0*(double)yf)/img->height;

	IplImage *sizedImg=cvCreateImage(cvSize(IMG_WIDTH, IMG_HEIGHT),img->depth, rec_p_.channels);
	cvResize(img1, sizedImg, CV_INTER_LINEAR); //подгоняем лицо под размер
				
	IplImage *grayImg=cvCreateImage(cvSize(IMG_WIDTH, IMG_HEIGHT), IPL_DEPTH_8U,1);

	cvCvtColor(sizedImg,grayImg,CV_BGR2GRAY); //перегоняем в серый

	cvEqualizeHist(grayImg, testImg);

	cvReleaseImage(&sizedImg);
	cvReleaseImage(&grayImg);

	cvReleaseImage(&img1);

	int response = recognize(); //try recognize

    sprintf_s(bufstr, 64, "Unknown");

	if (response>0) sprintf_s(bufstr, 64, "%s", facesDB[response-1].name);

    CvFont font;
    cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0, 1, CV_AA);
    cvPutText(img, bufstr, cvPoint(r->x, r->y+r->height/2), &font, cvScalar(255, 255, 255, 0));

	SendNewFace(response,xf,yf); //send result to servo

}

/*****************************************************************************/
//основной поток
void  RecogTask::run()
{ 
	printf("Start recog\n");

	MessageFromServo fromDeq; //сообщение для распознавания, получаемое из дека
	fromDeq.id=0; //сброс

    faceImgArr= 0; 
    personNumTruthMat= 0; 
    TrainFaces_count= 0; 
    Eigens_count= 0; 
    pAvgTrainImg= 0; 
    eigenVectArr= 0;
    eigenValMat= 0; 
    projectedTrainFaceMat=0;
	trainPersonNumMat=0;

    cascade_f = (CvHaarClassifierCascade*)cvLoad("haarcascade_frontalface_alt.xml", 0, 0, 0);
    storage = cvCreateMemStorage(0);

	CvCapture* capture = cvCaptureFromCAM(0);

//	cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH,320);
//	cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT,240);

    IplImage* frame;
    cvNamedWindow( "CaptureWindow", CV_WINDOW_NORMAL );   // Основное Окно захвата
    char key;

    cout << "  'ESC'  exit"                                        << endl;

	training(); //дообучение
	loadTrainingData();

	testImg = cvCreateImage(cvSize(IMG_WIDTH,IMG_HEIGHT), IPL_DEPTH_8U,1);

//	writer = cvCreateVideoWriter("faces.avi", -1, 20, cvSize(640, 480), 1);
//    if (writer!=0) printf("writer OK\n");

	if( capture )
    {
		printf("Capture ready\n");
		while (true)
		{

			if( !cvGrabFrame( capture )) break;		
            frame = cvRetrieveFrame( capture );				//кадр с камеры
			if( !frame ) {printf("Error frame\n");break;} //при отутствии кадра - выход
			
			//cvFlip(frame, frame, 0);					//переворот

			mut_new_data.lock();	
			if(!deq.empty()){  //дек не пуст
				fromDeq=deq.front(); //берем первое из дека
				deq.pop_front(); 
			}
			mut_new_data.unlock();

			key = cvWaitKey(1);   //ожидаем нажатия кнопки
	//		if( key == 'l' ) fromDeq.id=10;

#if (PRINT_DEBUG!= 0)
			float t = (float)cvGetTickCount();	
#endif
			//printf("id=%d\n",fromDeq.id);
			detectFaces(frame, fromDeq.id);   //поиск лиц

			fromDeq.id=0; //сброс

//			cvWriteFrame(writer, frame);

#if (PRINT_DEBUG!= 0)
			float t1 = (float)cvGetTickCount() - t;
			printf("< %.1f >\n", t1/(cvGetTickFrequency()*1000.));
#endif

			cvShowImage("CaptureWindow", frame);

			if( key == 27 )  break;              //выход
            
		}//while (true)
	}

//	cvReleaseVideoWriter(&writer);

    cvReleaseCapture( &capture );
    cvDestroyWindow( "CaptureWindow" );

}