#pragma once
#include "framework.h"

using namespace std;
using namespace cv;

struct ImageData {
	LONG Width, Height;
	vector<BYTE> Data;
	PDIRECT3DTEXTURE9 Texture;
};


// Helper functions opencv 
Mat getMat( HWND hWND ) {

	RECT windowRect;
	GetClientRect( hWND, &windowRect );
	HDC deviceContext = GetDC( hWND );
	HDC memoryDeviceContext = CreateCompatibleDC( deviceContext );


	int height = windowRect.bottom;
	int width = windowRect.right;

	HBITMAP bitmap = CreateCompatibleBitmap( deviceContext, width, height );

	SelectObject( memoryDeviceContext, bitmap );

	/*   int x           = x1;
	   int y           = y2;
	   int width2      = cx-x;
	   int height2       = cy-y;*/
	   //copy data into bitmap
	BitBlt( memoryDeviceContext, 0, 0, width, height, deviceContext, 0, 0, SRCCOPY );


	//specify format by using bitmapinfoheader!
	BITMAPINFOHEADER bi;
	bi.biSize = sizeof( BITMAPINFOHEADER );
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0; //because no compression
	bi.biXPelsPerMeter = 1; //we
	bi.biYPelsPerMeter = 2; //we
	bi.biClrUsed = 3; //we ^^
	bi.biClrImportant = 4; //still we

	cv::Mat mat = cv::Mat( height, width, CV_8UC4 ); // 8 bit unsigned ints 4 Channels -> RGBA

	//transform data and store into mat.data
	GetDIBits( memoryDeviceContext, bitmap, 0, height, mat.data, ( BITMAPINFO* )&bi, DIB_RGB_COLORS );

	//clean up!
	DeleteObject( bitmap );
	DeleteDC( memoryDeviceContext ); //delete not release!
	ReleaseDC( hWND, deviceContext );

	return mat;
}