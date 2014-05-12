#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "engine.h"
#include <fcntl.h>
#include <io.h>

#define SHIMMER_DATA_OSC_ADDR "/shimmerdata"
#define BUS_OSC_ADDR "/c_set"
#define SHIMMER_NUM_ARGS 18
#define SHIMMER_PORT 57120
#define SC_SERVER_PORT 57110

#define SHIMMER_IPADDR "192.168.2.100"

#include "OscSender.h"
#include "ShimmerWrapper.h"



using namespace ci;
using namespace ci::app;
using namespace std;

class ShimmerInterfaceApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown(KeyEvent key); 
	void update();
	void draw();
	void shutdown(); 
	void showWin32Console();
	virtual void prepareSettings(Settings * settings);



	Engine *ep; //matlab engine
	ShimmerWrapper *shimmer; 
	osc::Sender sender; 
	osc::Sender serverSender;
};

void ShimmerInterfaceApp::setup()
{
	showWin32Console();

	/*
	* Start the MATLAB engine
	*/
	if (!(ep = engOpen(NULL))) {
//		MessageBox((HWND)NULL, L"Can't start MATLAB engine",
//			L"Engwindemo.c", MB_OK);
//		exit(-1);
		cout << "Unable to start MATLAB engine... \n";
	}
	else cout << "MATLAB engine started! WUT.\n";

	//create a Shimmer wrapper to interface with the sensor
	shimmer = new ShimmerWrapper(ep, 0);

	sender.setup(SHIMMER_IPADDR, SHIMMER_PORT);
	serverSender.setup(SHIMMER_IPADDR, SC_SERVER_PORT);
}

void ShimmerInterfaceApp::prepareSettings(Settings * settings)
{
	settings->setFrameRate(75); 
}

void ShimmerInterfaceApp::keyDown(KeyEvent key)
{
	if (key.getChar() == ' ')
	{
		shimmer->connect();
	}
	else if (key.getChar() == 's')
	{
		shimmer->start();
	}
	else if (key.getChar() == 'q')
	{
		shimmer->stop();
	}
}

void ShimmerInterfaceApp::showWin32Console(){
	static const WORD MAX_CONSOLE_LINES = 500;
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;
	// allocate a console for this app
	AllocConsole();
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
		coninfo.dwSize);
	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);
	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);
	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio();
}

void ShimmerInterfaceApp::mouseDown(MouseEvent event)
{
}

void ShimmerInterfaceApp::update()
{
	if (shimmer->isStarted())
	{
		shimmer->run(); 
		if (shimmer->hasData())
		{
		
			//should really have written some kind of function for this... 
			std::vector<ci::osc::Message> msgVec = shimmer->getOSC(); 
			for (int i = 0; i < msgVec.size(); i++)
				sender.sendMessage(msgVec[i]);

			msgVec.clear();
			msgVec = shimmer->getOSCAccelToSCBusX();
			cout << "sooo x-values? wut?: " << msgVec.size() << std::endl;
			for (int i = 0; i < msgVec.size(); i++)
				serverSender.sendMessage(msgVec[i]);

			msgVec.clear();
			msgVec = shimmer->getOSCAccelToSCBusY();
			for (int i = 0; i < msgVec.size(); i++)
				serverSender.sendMessage(msgVec[i]);

			msgVec.clear();
			msgVec = shimmer->getOSCAccelToSCBusZ();
			for (int i = 0; i < msgVec.size(); i++)
				serverSender.sendMessage(msgVec[i]);
		}
	}
}

void ShimmerInterfaceApp::shutdown()
{
	FreeConsole(); 
	shimmer->stop(); 
	delete shimmer; 
}

void ShimmerInterfaceApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( ShimmerInterfaceApp, RendererGl )
