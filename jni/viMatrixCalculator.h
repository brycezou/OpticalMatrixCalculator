#include <iostream>
#include "highgui.h"
#include "cv.h"
#include "BlobResult.h"
#include "blob.h"
#include <list>
#include "ittFann.h"

using namespace std;

char* getJsonMatrix(IplImage *srcRGB);