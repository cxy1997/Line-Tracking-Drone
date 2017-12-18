//这个基本没用。。可以不写
#ifndef QUEUE_H
#define QUEUE
#endif

template <typename T>
class queue
{
private:
	queue<T> *nxt;
public:
	T dat;
	queue(T x){nxt=NULL;dat=x;};
	queue<T>* next()	{return nxt;};
	void link(queue<T> *nx)
	{
		nxt=nx;
	}
	queue<T>* connect(queue<T> *nx)
	{
		nx->link(nxt);
		link(nx);
		return nx;
	};
};

class point
{
public:
	int x,y;
	point(int x,int y){this->x=x;this->y=y;}
	point(){x=y=-1;}
};

typedef queue<point> MQ;