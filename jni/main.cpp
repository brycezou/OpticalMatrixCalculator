#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <opencv/highgui.h>
#include <opencv/cv.h>
#include "viMatrixCalculator.h"

using namespace std;
using namespace cv;


#define  LOG_TAG    "Native_MatrixRecognition"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


typedef struct 
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} rgba;


IplImage* MyBmp2IplImage(AndroidBitmapInfo infoOrg, void *pixOrg)
{
	IplImage *srcCaptured = cvCreateImage(cvSize(infoOrg.width, infoOrg.height), IPL_DEPTH_8U, 3);
	CvScalar stemp;
	for( int y = 0; y < infoOrg.height; y ++ ) 
    {
    	rgba * Orgline = (rgba *) pixOrg;
    	for ( int x = 0 ; x < infoOrg.width; x++ ) 
    	{
			stemp.val[2] = Orgline[x].red;
			stemp.val[1] = Orgline[x].green;
			stemp.val[0] = Orgline[x].blue;
			cvSet2D(srcCaptured, y, x, stemp);
    	}
    	
    	pixOrg = (char *) pixOrg + infoOrg.stride;
    }
	
	return srcCaptured;
}


void MyIplImage2Bmp(AndroidBitmapInfo infoDst, IplImage *idCardCutImg, void *pixDst)
{
	CvScalar stemp;
    for( int y = 0; y < idCardCutImg->height; y ++ ) 
    {
    	rgba * Dstline = (rgba *) pixDst;
    	for ( int x = 0 ; x < idCardCutImg->width; x++ ) 
    	{
			stemp=cvGet2D(idCardCutImg, y, x);
    		Dstline[x].red = stemp.val[2];
			Dstline[x].green = stemp.val[1];
			Dstline[x].blue = stemp.val[0];
    	}
    	
    	pixDst = (char *) pixDst + infoDst.stride;
    }
}


extern "C"
{

JNIEXPORT jobject JNICALL Java_njust_pr_opticalmatrixcalculator_MainActivity_RecognizeMatrix(	JNIEnv *env,
																								jobject  obj, 
																								jobject bmpOrg )
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	jclass myobj = env->FindClass("njust/pr/opticalmatrixcalculator/MatrixInfo");
	LOGI("Get into class");
	jmethodID construcMID = env->GetMethodID(myobj, "<init>", "(IILjava/lang/String;)V");  
	LOGI("Call construct method");
	jobject objReturn;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LOGI("Go into native method");
    AndroidBitmapInfo  infoOrg;
    void*              pixOrg; 
    int                retValue;

    if ((retValue = AndroidBitmap_getInfo(env, bmpOrg, &infoOrg)) < 0)
    { 
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", retValue); 
		objReturn = env->NewObject( myobj, construcMID, 0, 0, env->NewStringUTF("{\"end\":\"error\"}\0"));
		return objReturn;
	}
    if (infoOrg.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    { 
		LOGE("Bitmap format is not RGBA_8888 !");     	
		objReturn = env->NewObject( myobj, construcMID, 0, 0, env->NewStringUTF("{\"end\":\"error\"}\0"));
		return objReturn;
	}
    if ((retValue = AndroidBitmap_lockPixels(env, bmpOrg, &pixOrg)) < 0)
    { 
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", retValue); 
		objReturn = env->NewObject( myobj, construcMID, 0, 0, env->NewStringUTF("{\"end\":\"error\"}\0"));
		return objReturn;
	}

	LOGI("Bmp2IplImage");
	IplImage *srcCaptured = MyBmp2IplImage(infoOrg, pixOrg);
	LOGI("Begin to process image");
	char *strResult = getJsonMatrix(srcCaptured);
	cvReleaseImage(&srcCaptured);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	jstring strReturn = env->NewStringUTF(strResult);
    objReturn = env->NewObject( myobj, construcMID, 0, 0, strReturn);
	LOGI("Java object created");
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LOGI("successful");
	LOGI("Unlocking pixels");
	AndroidBitmap_unlockPixels(env, bmpOrg);

	return objReturn;
}

}//end extern "C"


