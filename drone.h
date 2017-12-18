//小车类封装
#include"PORT.h"

class car
{
	ComPort com;
	char status;
public:
	car() :com(L"COM5")
	{
		status = 'Q';
	}
	void turnLeft()
	{
		status = 'A';
		com.Send('A');
	}
	void turnRight()
	{
		status = 'D';
		com.Send('D');
	}
	void advance()
	{
		status = 'W';
		com.Send('W');
	}
	void stop()
	{
		status = 'Q';
		com.Send('Q');
	}
	void back()
	{
		status = 'S';
		com.Send('S');
	}
	void head(double degree)
	{
		if (status == 'Q') advance();
		else if (status == 'A' && degree > 0) advance();
		else if (status == 'D' && degree < 0) advance();
		else if (status == 'W')
		{
			if (degree > 10) turnRight();
			else if (degree < -10) turnLeft();
		}
		cout << status << endl;
	}
};