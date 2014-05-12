#include <sstream>
#include "mex.h"

#define MAX_MATLAB_OUTPUT_LEN 8192



/* get_characteristics figures out the size, and category
of the input array_ptr, and then displays all this information. */
void
get_characteristics(const mxArray *array_ptr)
{
	const char    *class_name;
	const mwSize  *dims;
	char          *shape_string;
	char          *temp_string;
	mwSize        c;
	mwSize        number_of_dimensions;
	size_t        length_of_shape_string;

	/* Display the mxArray's Dimensions; for example, 5x7x3.
	If the mxArray's Dimensions are too long to fit, then just
	display the number of dimensions; for example, 12-D. */
	number_of_dimensions = mxGetNumberOfDimensions(array_ptr);
	dims = mxGetDimensions(array_ptr);

	/* alloc memory for shape_string w.r.t thrice the number of dimensions */
	/* (so that we can also add the 'x')                                   */
	shape_string = (char *)mxCalloc(number_of_dimensions * 3, sizeof(char));
	shape_string[0] = '\0';
	temp_string = (char *)mxCalloc(64, sizeof(char));

	for (c = 0; c<number_of_dimensions; c++) {
		sprintf(temp_string, "%"FMT_SIZE_T"dx", dims[c]);
		strcat(shape_string, temp_string);
	}

	length_of_shape_string = strlen(shape_string);
	/* replace the last 'x' with a space */
	shape_string[length_of_shape_string - 1] = '\0';
	if (length_of_shape_string > 16) {
		sprintf(shape_string, "%"FMT_SIZE_T"u-D", number_of_dimensions);
	}
	std::cout << "Dimensions: " << shape_string << std::endl;

	/* Display the mxArray's class (category). */
	class_name = mxGetClassName(array_ptr);
	std::cout << "Class Name: " << class_name << std::endl;


	/* Display a bottom banner. */
	std::cout << "------------------------------------------------\n";

	/* free up memory for shape_string */
	mxFree(shape_string);
}

/* Display the subscript associated with the given index. */
void
display_subscript(const mxArray *array_ptr, mwSize index)
{
	mwSize     inner, subindex, total, d, q, number_of_dimensions;
	mwSize       *subscript;
	const mwSize *dims;

	number_of_dimensions = mxGetNumberOfDimensions(array_ptr);
	subscript = (mwSize *) mxCalloc(number_of_dimensions, sizeof(mwSize));
	dims = mxGetDimensions(array_ptr);

	std::cout << "(";
	subindex = index;
	for (d = number_of_dimensions - 1;; d--) { /* loop termination is at the end */

		for (total = 1, inner = 0; inner<d; inner++)
			total *= dims[inner];

		subscript[d] = subindex / total;
		subindex = subindex % total;
		if (d == 0) {
			break;
		}
	}

	for (q = 0; q<number_of_dimensions - 1; q++) {
		std::cout << subscript[q] << ",";
	}
	std::cout << subscript[number_of_dimensions - 1] << " )  ";

	mxFree(subscript);
}

static void
analyze_double(const mxArray *array_ptr)
{
	double *pr, *pi;
	mwSize total_num_of_elements, index;

	pr = mxGetPr(array_ptr);
	pi = mxGetPi(array_ptr);
	total_num_of_elements = mxGetNumberOfElements(array_ptr);

	for (index = 0; index<total_num_of_elements; index++)  {
		std::cout << "\t";
		display_subscript(array_ptr, index);
		std::cout << " = " << *pr++ << std::endl;
	}
}


/* Pass analyze_string a pointer to a char mxArray.  Each element
in a char mxArray holds one 2-byte character (an mxChar);
analyze_string displays the contents of the input char mxArray
one row at a time.  Since adjoining row elements are NOT stored in
successive indices, analyze_string has to do a bit of math to
figure out where the next letter in a string is stored. */
static void
analyze_string(const mxArray *string_array_ptr)
{
	char *buf;
	mwSize number_of_dimensions, buflen;
	const mwSize *dims;
	mwSize d, page, total_number_of_pages, elements_per_page;

	/* Allocate enough memory to hold the converted string. */
	buflen = mxGetNumberOfElements(string_array_ptr) + 1;
	buf = (char *) mxCalloc(buflen, sizeof(char));

	/* Copy the string data from string_array_ptr and place it into buf. */
	if (mxGetString(string_array_ptr, buf, buflen) != 0)
		std::cout << "MATLAB:explore:invalidStringArray",
		"Could not convert string data\n.";
	else std::cout << "what is inside so far: " << buf << std::endl; 
}

class ShimmerData
{
private: 
	double data[SHIMMER_NUM_ARGS];
public: 
	inline double getTimeStamp(){ return data[0];  };
	inline ci::Vec3d getAccelData()
	{
		ci::Vec3d accelData(data[1], data[2], data[3]); 
		return accelData; 
	};
	inline ci::Vec3d getAccelDataLowNoise()
	{
		ci::Vec3d accelData(data[4], data[5], data[6]);
		return accelData;
	};
	inline ci::Vec3d getAccelDataWideRange()
	{
		ci::Vec3d accelData(data[7], data[8], data[9]);
		return accelData;
	};
	inline ci::Vec3d getGyro()
	{
		ci::Vec3d gyro(data[10], data[11], data[12]);
		return gyro;
	};
	inline ci::Vec3d getMagnet()	
	{
		ci::Vec3d mag(data[13], data[14], data[15]);
		return mag;
	};
	inline double getBatt1(){ return data[16]; };
	inline double getBatt2(){ return data[17]; };

	inline void setData(int index, double d)
	{
		data[index] = d;
	}
	inline double getData(int index)
	{
		return data[index];
	}

	ShimmerData(){}; 
};

class ShimmerWrapper
{
private:
	Engine *ep; //MATLAB engine driving this code
	int _ID; 
	std::string port;
	std::string varName;
	std::string deviceID; 
	mxArray *shimmerData; 
	mxArray *shimmerNames;
	std::vector<ShimmerData *> curData; 
	bool init; 
	bool started; 
	char buffer[MAX_MATLAB_OUTPUT_LEN + 1];
	void handleData(); 
	void printNames();
	void eraseOldData();
	ci::Vec2i getSubscript(const mxArray *array_ptr, mwSize index);
	std::vector<ci::osc::Message> getOSCScalarData(int whichData);
public: 
	ShimmerWrapper(Engine *e, int id, std::string port_num = "3");
	void connect();
	void start();
	void logOutput();
	void run();
	void stop(); 
	std::string getDeviceID();
	inline bool isStarted(){ return started; };
	inline std::vector<ShimmerData *> getCurData(){ return curData; };
	std::vector<ci::osc::Message> getOSC();
	std::vector<ci::osc::Message> getOSCAccelToSCBusX();
	std::vector<ci::osc::Message> getOSCAccelToSCBusY();
	std::vector<ci::osc::Message> getOSCAccelToSCBusZ();

	bool hasData(){ return curData.size() != 0;  };
};

ShimmerWrapper::ShimmerWrapper(Engine *e, int id, std::string port_num)
{
	ep = e; 
	_ID = id;
	port = port_num;

	std::stringstream ss; ss << "shimmer" << id;
	varName = ss.str(); 
	init = false; 
	started = false; 
	deviceID = "";

	//set up buffer
	buffer[MAX_MATLAB_OUTPUT_LEN] = '\0';
	int res = engOutputBuffer(ep, buffer, MAX_MATLAB_OUTPUT_LEN);
	logOutput();
}

ci::Vec2i ShimmerWrapper::getSubscript(const mxArray *array_ptr, mwSize index)
{
	mwSize     inner, subindex, total, d, q, number_of_dimensions;
	mwSize       *subscript;
	const mwSize *dims;

	number_of_dimensions = mxGetNumberOfDimensions(array_ptr);
	subscript = (mwSize *)mxCalloc(number_of_dimensions, sizeof(mwSize));
	dims = mxGetDimensions(array_ptr);
	ci::Vec2i result; 

	subindex = index;
	for (d = number_of_dimensions - 1;; d--) { /* loop termination is at the end */

		for (total = 1, inner = 0; inner<d; inner++)
			total *= dims[inner];

		subscript[d] = subindex / total;
		subindex = subindex % total;
		if (d == 0) {
			break;
		}
	}

	for (q = 0; q<number_of_dimensions - 1; q++) {
		result.x = subscript[q];
	}
	result.y = subscript[number_of_dimensions - 1];

	mxFree(subscript);

	return result;
}


void ShimmerWrapper::connect()
{												//TODO: pass as param!!!
	std::stringstream ss; 
	ss << varName << " = ShimmerHandleClass('" << port << "');";
	engEvalString(ep, ss.str().c_str());              //Define shimmer as a ShimmerHandle Class instance with comPort1; 
	std::cout << ss.str() << std::endl;
	logOutput();
	
	std::stringstream s2; s2 << "started = connectShimmer(" << varName << ");";
	std::cout << s2.str() << std::endl;
	engEvalString(ep, s2.str().c_str()); 
	logOutput();
}

void ShimmerWrapper::eraseOldData()
{
	for (int i = 0; i < curData.size(); i++)
	{
		if (curData[i] != NULL)
			delete curData[i];
		curData[i] = NULL;
	}
	curData.clear(); 
}

std::vector<ci::osc::Message> ShimmerWrapper::getOSC()
{
	std::vector<ci::osc::Message> msgVector;
	const int MAX_MSG = 100; 

	int  i = 0; 
	int  howManyTimes = curData.size() / MAX_MSG; 
	int  leftover = curData.size() % MAX_MSG;

	for (int k = 0; k < howManyTimes; k++)
	{
		ci::osc::Message msg;
		msg.setAddress(SHIMMER_DATA_OSC_ADDR);
		msg.addIntArg(MAX_MSG);
		for (i = k*MAX_MSG; i < MAX_MSG*(k+1); i++)
		for (int j = 0; j < SHIMMER_NUM_ARGS; j++)
		{
			msg.addFloatArg(curData[i]->getData(j));
		}
		msgVector.push_back(msg); 
	}

	if( leftover > 0)
	{
		ci::osc::Message msg;
		msg.setAddress(SHIMMER_DATA_OSC_ADDR);
		msg.addIntArg(leftover);
		for (i = howManyTimes*MAX_MSG; i < curData.size(); i++)
		for (int j = 0; j < SHIMMER_NUM_ARGS; j++)
		{
			msg.addFloatArg(curData[i]->getData(j));
		}
		msgVector.push_back(msg);
	}

//	std::cout << "Number of OSC messages which will be sent: " << msgVector.size() << std::endl;

	return msgVector;
}

std::vector<ci::osc::Message> ShimmerWrapper::getOSCScalarData(int whichData)
{
	//data needs to be bundled since it is overflowing the buffers, as is
	std::vector<ci::osc::Message> msgVector;
	const int MAX_MSG = 1024;

	int  i = 0;
	int  howManyTimes = curData.size() / MAX_MSG;
	int  leftover = curData.size() % MAX_MSG;

	for (int k = 0; k < howManyTimes; k++)
	{
		for (i = k*MAX_MSG; i < MAX_MSG*(k + 1); i++)
		{
			ci::osc::Message msg;
			msg.setAddress(BUS_OSC_ADDR);
			msg.addIntArg(whichData - 1);
			msg.addFloatArg(curData[i]->getData(whichData));
			msgVector.push_back(msg);
		}
	}

	if (leftover > 0)
	{
		for (i = howManyTimes*MAX_MSG; i < curData.size(); i++)
		{
			ci::osc::Message msg;
			msg.setAddress(BUS_OSC_ADDR);
			msg.addIntArg(whichData - 1);
			msg.addFloatArg(curData[i]->getData(whichData));
			msgVector.push_back(msg);
		}
	}

	return msgVector;
}

std::vector<ci::osc::Message> ShimmerWrapper::getOSCAccelToSCBusX()
{
	return getOSCScalarData(1);
}

std::vector<ci::osc::Message> ShimmerWrapper::getOSCAccelToSCBusY()
{
	return getOSCScalarData(2);
}

std::vector<ci::osc::Message> ShimmerWrapper::getOSCAccelToSCBusZ()
{
	return getOSCScalarData(3);
}

void ShimmerWrapper::handleData()
{
	mwSize s = mxGetNumberOfElements(shimmerData);
	eraseOldData(); //clear the current data -- REMEMBER I AM DOING THIS 
	if (s > 0){
		//get_characteristics(shimmerData);
		//analyze_double(shimmerData);

		double *pr, *pi;
		mwSize total_num_of_elements, index, numOfDataPackets, indexNum;

		pr = mxGetPr(shimmerData);
		numOfDataPackets = mxGetM(shimmerData);
		indexNum = mxGetN(shimmerData);

		total_num_of_elements = mxGetNumberOfElements(shimmerData);

		for (index = 0; index<total_num_of_elements; index++)  {
			ci::Vec2i indexVector = getSubscript(shimmerData, index);
			int i = indexVector.x;
			int j = indexVector.y;

			if (j == 0)
			{
				curData.push_back(new ShimmerData); 
			}
			curData[i]->setData(j, *pr++);
			//display_subscript(shimmerData, index);
			//std::cout << "  " << i<<","<<j <<" = " << *pr << std::endl;
		}
	}
}

void ShimmerWrapper::printNames()
{
	mwSize s = mxGetNumberOfElements(shimmerNames);
	//std::cout << "string array dims=" << s << std::endl;
	analyze_string(shimmerNames);
}

void ShimmerWrapper::run()
{
	// Read the latest data from shimmer data buffer, signalFormatArray defines the format of the data and signalUnitArray the unit
	std::stringstream ss;
	ss << "[newData, signalNameArray, signalFormatArray, signalUnitArray] = " << varName << ".getdata('Time Stamp', 'c', 'Accelerometer', 'c', 'Gyroscope', 'c', 'Magnetometer', 'u', 'Battery Voltage', 'a');";
	engEvalString(ep, ss.str().c_str());
	logOutput();

	shimmerData = engGetVariable(ep, "newData");
	if (shimmerData == NULL)
	{
		if (init){
			std::cout << "ERROR! No data received from " << varName << ":  " << deviceID << std::endl;
		}
	}
	else if (!init && mxGetNumberOfElements(shimmerData) > 0)
	{
		engEvalString(ep, "signalNamesString = char(signalNameArray(1,1)); ");
		engEvalString(ep, "for i = 2:length(signalNameArray)\ntabbedNextSignalName = [char(9), char(signalNameArray(1, i))];\n signalNamesString = strcat(signalNamesString, tabbedNextSignalName); end");

		shimmerNames = engGetVariable(ep, "signalNamesString");
		printNames();
		init = true;
	}
	else
	{
		handleData(); 
	}

}

void ShimmerWrapper::start()
{
	std::stringstream s2; s2 << "res = " << varName << ".start;";
	engEvalString(ep, s2.str().c_str());
	logOutput();
	started = true; 
}

void ShimmerWrapper::logOutput()
{
	std::string buf(buffer);
	if ( buf.length() > 0 )
		std::cout << "MATLAB output: " << buffer << std::endl; 
}

void ShimmerWrapper::stop()
{
	std::stringstream ss;
	ss << varName << ".stop;\n" << varName << ".disconnect;";
	logOutput();
	started = false; 
}

