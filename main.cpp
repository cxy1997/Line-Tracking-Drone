#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>  
#include "opencv.hpp"
#include <iostream>	
#include <cstring>
#include <cmath>
#include "ImageClass.h"
#include "car.h"
using namespace cv;
using namespace std;

const int height=400,width=400;
const double PI = 3.14159265358979323846264;
car OptimusPrime;  //小车变量
CvPoint2D32f originalPoints[5];
int poicnt=0;  //记录取到第几个点
CvCapture *cam;
IplImage *transimg=0,*img;
IplImage *TrackImage=cvCreateImage(cvSize(400,400),IPL_DEPTH_8U,3);
IplImage* dstimg = cvCreateImage(cvSize(400,400),IPL_DEPTH_8U,1);
IplImage* dstimg2 = cvCreateImage(cvSize(400,400),IPL_DEPTH_8U,1);
CvMat* transmat;
CvPoint2D32f newPoints[4];  //透视变换的四个点
CvPoint2D32f corners[50];  //路径拐角
CvPoint2D32f route[50];  //按顺序排列的路径点
bool visited[50] = { 0 };  //排序辅助
int rotcnt = 1, cornerCounts = 30;
HsvFloatImage HSVimg;
bool isRed(const int i, const int j)  //识别色块与小车红色的契合度
{
	return (HSVimg[i][j].h > 320 && HSVimg[i][j].h<338 && HSVimg[i][j].s<0.9 && HSVimg[i][j].s>0.48 && HSVimg[i][j].v > 100 && HSVimg[i][j].v < 200);
}
bool isBlue(const int i, const int j)  //识别色块与小车蓝色的契合度
{
	return (HSVimg[i][j].h > 184 && HSVimg[i][j].h<200 && HSVimg[i][j].s>0.8 && HSVimg[i][j].v > 220);
}
bool isMargin(CvPoint2D32f p)  //判断路径点是否无效
{
	if (p.x > 100 && p.x < 140 && p.y>260 && p.y < 390) return false;
	return (p.x < 40 || p.x>360 || p.y < 40 || p.y>360 || (p.x>267 && p.y>300));
}
void cvThin (IplImage* src, IplImage* dst, int iterations = 1)   //细化函数
{
    cvCopy(src, dst);
    BwImage dstdat(dst);
    IplImage* t_image = cvCloneImage(src);
    BwImage t_dat(t_image);
    for (int n = 0; n < iterations; n++)
        for (int s = 0; s <= 1; s++) {
            cvCopy(dst, t_image);
            for (int i = 0; i < src->height; i++)
                for (int j = 0; j < src->width; j++)
                    if (t_dat[i][j]) {
                        int a = 0, b = 0;
                        int d[8][2] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1},
                            {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                        int p[8];
                        p[0] = (i == 0) ? 0 : t_dat[i-1][j];
                        for (int k = 1; k <= 8; k++) {
                            if (i+d[k%8][0] < 0 || i+d[k%8][0] >= src->height ||
                                j+d[k%8][1] < 0 || j+d[k%8][1] >= src->width)
                                p[k%8] = 0;
                            else p[k%8] = t_dat[ i+d[k%8][0] ][ j+d[k%8][1] ];
                            if (p[k%8]) {
                                b++;
                                if (!p[k-1]) a++;
                            }
                        }
                        if (b >= 2 && b <= 6 && a == 1)
                            if (!s && !(p[2] && p[4] && (p[0] || p[6])))
                                dstdat[i][j] = 0;
                            else if (s && !(p[0] && p[6] && (p[2] || p[4])))
                                dstdat[i][j] = 0;
                    }
        }
    cvReleaseImage(&t_image);
}
void mouse(int mouseevent, int x, int y, int flags, void* param)  //Camera按键处理
{
    if (mouseevent == CV_EVENT_LBUTTONDOWN)    //按下左键
    {
        originalPoints[poicnt]=cvPoint2D32f(x,y);
        if (poicnt<4)
            ++poicnt;
        std::cout<<x<<" "<<y<<std::endl;
    }
}
void mouse2(int mouseevent, int x, int y, int flags, void* param)  //识别HSV色彩，调试用
{
	if (mouseevent == CV_EVENT_LBUTTONDOWN)    //按下左键
	{
		cout << "(" << x << ", " << y << "): ";
		cout << "h: " << HSVimg[y][x].h << " , s: " << HSVimg[y][x].s << " , v: " << HSVimg[y][x].v << '\n';
	}
}
double distance(CvPoint2D32f a, CvPoint2D32f b)  //计算两点之间距离
{
	return sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}
CvPoint2D32f nextPoint(CvPoint2D32f center)  //取下一个目标点
{
    double min=100000,tmp;
    int minN=29;
    CvPoint tmpPoint;
    for(int i=0;i<cornerCounts;++i) if (!visited[i])
    {
        if(corners[i].x!=0 || corners[i].y!=0)
        {
            tmp=distance(corners[i],center);
            if(tmp<min) 
			{ 
				min=tmp;
				minN = i;
			}
        }
    }
	visited[minN] = true;
    return corners[minN];
}
void init(int index=0)  //系统初始化
{
    cam = cvCreateCameraCapture(index);
    if (!cam) return;
    transmat = cvCreateMat(3,3,CV_32FC1);
    newPoints[0] = cvPoint2D32f(20, 20);
    newPoints[1] = cvPoint2D32f(width -20, 20);
    newPoints[2] = cvPoint2D32f(20, height-20);
    newPoints[3] = cvPoint2D32f(width-20, height-20);
    poicnt=0;
}
int main()
{
    init(1);
    cvNamedWindow("Camera");
    cvSetMouseCallback("Camera", mouse);
    while (poicnt<4)      //等待取四个点
    {
        img = cvQueryFrame(cam);    //获取一帧图像，并且用img指向它
        cvShowImage("Camera",img);
        cvWaitKey(1);
    }
    transimg = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3);
    newPoints[0] = cvPoint2D32f(20, 20);
    newPoints[1] = cvPoint2D32f(380, 20);
    newPoints[2] = cvPoint2D32f(20, 380);
    newPoints[3] = cvPoint2D32f(380, 380);
	cvGetPerspectiveTransform(originalPoints, newPoints, transmat);   //根据四个点计算变换矩阵
    while (1)  //矩阵处理
    {
        img = cvQueryFrame(cam);
        cvWarpPerspective(img, transimg, transmat);  //透视变换
        cvShowImage("Camera",transimg);
        if (cvWaitKey(1)==13) break; //等待回车键以开始
    }
	cvDestroyWindow("Camera");
    cvNamedWindow("Realtime Tracking");
	cvSetMouseCallback("Realtime Tracking", mouse2);
    //赛道处理
    img = cvQueryFrame(cam);
    cvWarpPerspective(img, transimg, transmat);  //根据变换矩阵计算图像的透视变换
    cvInRangeS(transimg,CV_RGB(0,0,0),CV_RGB(100,100,100),dstimg);  //二值化后存入dstimg
    
    cvSmooth(dstimg,dstimg2,CV_GAUSSIAN);  //高斯平滑处理
    cvThin(dstimg2,dstimg,6);  //细化
    //寻找角点
    for(int i=0;i<50;i++) //初始化角点数组
    {
        corners[i].x=0;
        corners[i].y=0;
    }
    IplImage* tmp1 = cvCreateImage(cvSize(400,400),IPL_DEPTH_32F,1);
    IplImage* tmp2 = cvCreateImage(cvSize(400,400),IPL_DEPTH_32F,1);
    cvGoodFeaturesToTrack(dstimg,tmp1,tmp2,corners,&cornerCounts,0.05,40);  //最后一个参数为角点最小距离
    cvMerge(dstimg,dstimg,dstimg,0,TrackImage);
	cvReleaseImage(&tmp1); cvReleaseImage(&tmp2);
    for (int i=0; i < cornerCounts;++i)
	{
        if(isMargin(corners[i]))
		{
			while (i < cornerCounts && isMargin(corners[cornerCounts])) --cornerCounts;
			if (i < cornerCounts)
			{
				corners[i] = corners[cornerCounts--];
			}
		}
		if (i < cornerCounts) cvCircle(TrackImage, cvPoint((int)(corners[i].x), (int)(corners[i].y)), 6, CV_RGB(255, 0, 0), 2);
	}
    cvShowImage("Route",TrackImage);
    int numRed,numBlue; //中心点，numRed为车身中心，numBlue为车头中心
    int sumx,sumy;
    CvPoint2D32f centerRed,centerBlue;
    img = cvQueryFrame(cam);    //获取一帧图像，并且用img指向它
    cvWarpPerspective(img, transimg, transmat);  //根据变换矩阵计算图像的透视变换
    IplImage *dst_image = cvCreateImage(cvGetSize(transimg),32,transimg->nChannels);
    IplImage *src_image_32 = cvCreateImage(cvGetSize(transimg),32,transimg->nChannels);
    HSVimg = HsvFloatImage(dst_image);
    while(1)
    {
        img = cvQueryFrame(cam);    //获取一帧图像，并且用img指向它
        cvWarpPerspective(img, transimg, transmat);  //根据变换矩阵计算图像的变换
        cvConvertScale(transimg,src_image_32);//将原图转换为32f类型
        cvCvtColor(src_image_32,dst_image,CV_BGR2HSV);//得到HSV图，保存在dst_image中  参数范围H(0,360) S(0,1) V(0,255)
        sumx=0;
		sumy=0;
        numRed=0;
		numBlue=0;
        for (int i=0;i<400;++i) for (int j=0;j<400;++j) if (isRed(i,j)) 
		{
			sumx+=j;
			sumy+=i;
			++numRed;
		}
        if(numRed==0) numRed=1;
        centerRed.x=sumx/numRed;
		centerRed.y=sumy/numRed;
        sumx=0;
		sumy=0;
        for (int i=0;i<400;++i) for (int j=0;j<400;++j) if (isBlue(i,j))
		{
			sumx+=j;
			sumy+=i;
			++numBlue;
        }
        if(numBlue==0) 	numBlue=1;
        centerBlue.x=sumx/numBlue;centerBlue.y=sumy/numBlue;
        //画出两个中心
        cvCircle(transimg, cvPoint((int)(centerRed.x), (int)(centerRed.y)), 10, CV_RGB(255,0,0),3);
        cvCircle(transimg, cvPoint((int)(centerBlue.x), (int)(centerBlue.y)), 10, CV_RGB(0,0,255),3);
        cvShowImage("Realtime Tracking",transimg);
        
        if (cvWaitKey(1)==13) break;
    }
    //开始走黑线
    //heading为小车方向角，direction为目标的方向角，tmpDistance为到下一个目标点的距离
    double heading,direction,tmpDistance=0,cal;
    CvPoint2D32f next;
	route[0] = centerBlue;
	for (int i = 1; i <= cornerCounts; ++i) route[i] = nextPoint(route[i - 1]);
	cout << "cornerCounts = " << cornerCounts << '\n';
	for (int i = 0; i<=cornerCounts; ++i)
	{
		cout << int(route[i].x) << ' ' << int(route[i].y) << endl;
	}
    while(1)
    {
        img = cvQueryFrame(cam);
        cvWarpPerspective(img, transimg, transmat);  //根据变换矩阵计算图像的透视变换
        cvConvertScale(transimg,src_image_32);//将原图转换为32f类型
        cvCvtColor(src_image_32,dst_image,CV_BGR2HSV);//得到HSV图，保存在dst_image中  H(0,360) S(0,1) V(0,255)
        sumx=0;sumy=0;
        numRed=0;numBlue=0;
		for (int i = 0; i<400; ++i) for (int j = 0; j<400; ++j) if (isRed(i, j))
		{
			sumx += j;
			sumy += i;
			++numRed;
		}
		if (numRed == 0) numRed = 1;
		centerRed.x = sumx / numRed;
		centerRed.y = sumy / numRed;
		sumx = 0;
		sumy = 0;
		for (int i = 0; i<400; ++i) for (int j = 0; j<400; ++j) if (isBlue(i, j))
		{
			sumx += j;
			sumy += i;
			++numBlue;
		}
        if(numBlue==0) numBlue=1;
        centerBlue.x=sumx/numBlue;centerBlue.y=sumy/numBlue;
        cvCircle(transimg, cvPoint((int)(centerRed.x), (int)(centerRed.y)), 10, CV_RGB(255,0,0),3);
        cvCircle(transimg, cvPoint((int)(centerBlue.x), (int)(centerBlue.y)), 10, CV_RGB(0,0,255),3);
        //当距离当前目标点小于该阈值时认为这个点已经走到，取下一个目标点
		tmpDistance = distance(route[rotcnt],centerBlue);
		if (tmpDistance < 36)
		{
			cvCircle(TrackImage, cvPoint((int)(route[rotcnt].x), (int)(route[rotcnt].y)), 6, CV_RGB(0, 255, 0), 2);
			cvShowImage("Route", TrackImage); //显示路线图	
			++rotcnt;
		}
        if (rotcnt>cornerCounts) break;   //走完全程退出
        heading=atan2((centerBlue.y-centerRed.y),(centerBlue.x-centerRed.x))/PI*180;  
        direction=atan2((route[rotcnt].y-centerRed.y),(route[rotcnt].x-centerRed.x))/PI*180;
		cal = direction - heading;
		if (cal < -300) cal += 360;
		else if (cal > 300) cal -= 360;
		cout << cal << ' ';
		OptimusPrime.head(cal);
        cvCircle(transimg, cvPoint((int)(route[rotcnt].x), (int)(route[rotcnt].y)), 6, CV_RGB(255,255,255),2);	 
        cvShowImage("Realtime Tracking",transimg);//显示小车实时状况
        if (cvWaitKey(1)==13) break; 
    }
    OptimusPrime.stop();
    cout<<"走完全程！"<<endl;
    system("pause");
    return 0;
}
