

#include <iostream>
#include <stdio.h>

#include "ReciverTask.h"
#include "RecogTask.h"


#include <Poco/AutoPtr.h>

#include <Poco/Util/IniFileConfiguration.h>

#include "Poco/Mutex.h"
#include "Poco/NamedMutex.h"
#include <deque>

#include "Poco/Runnable.h"
#include "Poco/Thread.h"



using std::string;
using Poco::AutoPtr;
using Poco::Util::IniFileConfiguration;
using Poco::Mutex;
using Poco::NamedMutex;
using Poco::Runnable;
using std::deque;

Mutex mut_new_data;				///<ћутекс, используетс€ дл€ последовательного доступа к деку потоков приема файлов и распознавани€ 
deque<MessageFromServo> deq;	///<ќсновной дек данных, передаваемых из потока приема файлов в распознавание


///Max thread count
#define MAX_THREADS 8



int main( int argc, char** argv )
{

 NamedMutex StartSingleCopy("Face_Application");	
 if (!StartSingleCopy.tryLock()) return 0;					//ѕроверка на запуск единичного экземпл€ра приложени€

 recog_pars rec_par;  //параметры, используемые потоками распознавани€
 cadr_receive cadr_r; //параметры, используемые потоком приема кадров


//Read configuration
 AutoPtr<IniFileConfiguration> cfg(new IniFileConfiguration);
 try
 {
	cfg->load("conf.ini");

	//threads=cfg->getInt("DEBUG.threads",1);  //считываем кол-во потоков распознавани€

	rec_par.face_thresh=cfg->getDouble("RECOG.FaceThresh",100000000.0);	//threshold for recognition

	rec_par.channels=cfg->getInt("IMAGE.Channels",3);	//число каналов

	printf("Loading conf ok\n");

 }
 catch(...)
 {
	printf("Error loading config!\n");
	return -1;
 }


 Poco::Thread thread[MAX_THREADS];

 ReciverTask *synch_task = new ReciverTask(cadr_r);  //ѕоток приема команд
 thread[0].start(*synch_task);

 RecogTask *input_task = new RecogTask(rec_par); //1й поток распознавани€. ѕрисутствует всегда
 thread[1].start(*input_task);


 thread[0].join(); 
 thread[1].join(); 

    return 0;
}
