#pragma once
#include "framework.h"

using namespace std;
using namespace cv;

struct ImageData {
	LONG Width, Height;
	vector<BYTE> Data;
	PDIRECT3DTEXTURE9 Texture;
};