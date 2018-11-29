/**	\file ReciverTask.h
		\brief Description of class for frames receiving
*/
#ifndef RECIVER_TASK
#define RECIVER_TASK

#include "Poco/Runnable.h"
#include "Poco/Thread.h"

using std::string;
using Poco::Runnable;



/** \struct MessageFromServo	
     \brief Message from robot 
 */
struct MessageFromServo	
{

	int id;			///<id of request
	int x;			///<x-coord.
	int y;			///<y-coord.
};

/**
	\struct cadr_receive
	\brief IPC parameters
*/
typedef struct
{

	string servo_application_ip;	///<IP-adress of servo module
	int servo_application_socket;	///<Port number

} cadr_receive;


/**
	\class ReciverTask
	\brief Class for receiving data from servo app
*/
class ReciverTask : public Runnable
{
	cadr_receive &c_r_;
public:
	ReciverTask(cadr_receive &cre):c_r_(cre)
	{
	}

private:

	MessageFromServo m_servo;  ///<Msg to recognition

	
	virtual void run();	///<Start thread
};

#endif