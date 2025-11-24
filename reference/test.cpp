#include </usr/include/opencv4/opencv2/highgui/highgui.hpp>
#define PHOTOPATH /home/dxf/Desktop/1.png
int main(){
	image=cv2::imread(PHOTOPATH);
	cv2::imshow("test",image);
	return 0;
	}
