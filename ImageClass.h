#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>


template<class T>    //定义不同图像类型，方便访问图像元素
class Image
{
private:
	IplImage* imgptr;
public:
	Image(IplImage* img=0) {imgptr=img;}
	~Image() {imgptr = nullptr;}
	inline T* operator[](const int rowIndex) 
	{
		return ((T *)(imgptr->imageData + rowIndex*imgptr->widthStep));
	}
};
struct RgbPixel {
	unsigned char b,g,r;
};
struct RgbPixelFloat {
  float b,g,r;
};
struct HsvPixel {
	unsigned char h,s,v;
};
struct HsvPixelFloat {
  float h,s,v;
};
typedef Image<RgbPixelFloat>  RgbFloatImage;
typedef Image<RgbPixel>  RgbImage;
typedef Image<HsvPixelFloat>  HsvFloatImage;
typedef Image<HsvPixel>  HsvImage;
typedef Image<unsigned char>  BwImage;
