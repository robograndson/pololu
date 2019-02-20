/*
   The MIT License (MIT) (http://opensource.org/licenses/MIT)
   
   Copyright (c) 2015 Jacques Menuet
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include <stdio.h>
#include <cmath>
#include "std_msgs/Int64.h"
#include "ros/ros.h"

#if _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>
	#include <sys/timeb.h>
	#include <time.h>
	#include <sys/time.h>
#endif

#include "beginner_tutorials/RPMSerialInterface.h"

#define DEVICENUMBER 12
#define PINSTEER 4
#define PINMOTOR 2 

ros::Subscriber steering_sub, motor_sub; 
RPM::SerialInterface* serialInterface;
// A utility class to provide cross-platform sleep and simple time methods
class Utils
{
public:
	static void sleep( unsigned int _Milliseconds );
	static unsigned long long int getTickFrequency();
	static unsigned long long int getTimeAsTicks();
	static unsigned int getTimeAsMilliseconds();

private:
	static unsigned long long int mInitialTickCount;
};

void steering_callback(const std_msgs::Int64 &msg)
{
	serialInterface->setTargetCP(PINSTEER, msg.data);
}

void motor_callback(const std_msgs::Int64 &msg)
{
	if (msg.data >= 6000)
	{
		serialInterface->setTargetCP(PINMOTOR, msg.data);
	}
	else
	{
		serialInterface->setTargetCP(PINMOTOR, msg.data);
		Utils::sleep(50);
		serialInterface->setTargetCP(PINMOTOR, 6000);
		Utils::sleep(50);
		serialInterface->setTargetCP(PINMOTOR, msg.data);
	}
	
	ROS_INFO("%d",msg.data);
}


int main(int argc, char** argv)
{
	ros::init(argc, argv, "pololuControl");
    ros::NodeHandle n;
	steering_sub = n.subscribe("steering",1000, steering_callback);
	motor_sub = n.subscribe("motor",1000, motor_callback);

#ifdef _WIN32
	std::string portName = "COM4";
#else
	std::string portName = "/dev/ttyACM0";
	//std::string portName = "/dev/cu.usbmodem00031501"; // Example for Mac OS, the Maestro creates two devices, use the one with the lowest number (the command port)
#endif
	unsigned int baudRate = 9600;
	printf("Creating serial interface '%s' at %d bauds\n", portName.c_str(), baudRate);
	std::string errorMessage;
	serialInterface = RPM::SerialInterface::createSerialInterface( portName, baudRate, &errorMessage );
	if ( !serialInterface )
	{
		printf("Failed to create serial interface. %s\n", errorMessage.c_str());
		return -1;
	}
	ros::spin();
	return 0;
}

// Utils class implementation
void Utils::sleep( unsigned int _Milliseconds )
{
#if _WIN32
	::Sleep( _Milliseconds );
#else
	struct timespec l_TimeSpec;
	l_TimeSpec.tv_sec = _Milliseconds / 1000;
	l_TimeSpec.tv_nsec = (_Milliseconds % 1000) * 1000000;
	struct timespec l_Ret;
	nanosleep(&l_TimeSpec,&l_Ret);
#endif
}

unsigned long long int Utils::getTickFrequency()
{
#if _WIN32
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
#else
	// The gettimeofday function returns the time in microseconds. So it's frequency is 1,000,000.
	return 1000000;
#endif
}

unsigned long long int Utils::getTimeAsTicks()
{
	unsigned long long int tickCount;
#if _WIN32
	LARGE_INTEGER l;
	QueryPerformanceCounter(&l);
	tickCount = l.QuadPart;
#else
	struct timeval p;
	gettimeofday(&p, NULL);	// Gets the time since the Epoch (00:00:00 UTC, January 1, 1970) in sec, and microsec
	tickCount = (p.tv_sec * 1000LL * 1000LL) + p.tv_usec;
#endif
	if ( mInitialTickCount==0xffffffffffffffffUL )
		mInitialTickCount = tickCount;
	tickCount -= mInitialTickCount;
	return tickCount;
}

unsigned int Utils::getTimeAsMilliseconds()
{
	unsigned int millecondsTime = static_cast<unsigned int>( (getTimeAsTicks() * 1000) / getTickFrequency() );
	return millecondsTime;
}

unsigned long long int Utils::mInitialTickCount = 0xffffffffffffffffUL;

