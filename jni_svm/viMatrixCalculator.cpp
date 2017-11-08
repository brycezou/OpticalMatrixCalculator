#include <iostream>
#include "highgui.h"
#include "cv.h"
#include "BlobResult.h"
#include "blob.h"
#include <list>
#include "svm.h"
#include "viMatrixCalculator.h"
#include <android/log.h>

using namespace std;
using namespace cv;

#define FIXED_WIDTH	600
#define LOG_TAG    "Native_MatrixRecognition"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef struct BlobIndex
{
	int area;		//area
	int lx;			//left x
	int ty;			//top y
	int rx;			//right x;
	int by;			//bottom y;
	int cx;			//center x;
	int cy;			//center y;
	int width;		//width;
	int height;		//height;
	int yIndex;		//row
	int xIndex;		//column
	int label;
	int value;
}BlobIndex;

//�Զ��� list �������(���blob���)
bool BlobAreaSortRule(const BlobIndex &leftData, const BlobIndex &rightData)
{
	return leftData.area > rightData.area;
}

//���������������ͼ��
IplImage* ResizeInputImage(IplImage *src)
{
	//������С���Ŀ��ͼ��
	IplImage *newImg = cvCreateImage(cvSize(FIXED_WIDTH, src->height*FIXED_WIDTH/1.0/src->width), src->depth, src->nChannels);
	cvResize(src, newImg, CV_INTER_LINEAR);
	return newImg;
}

//��������Ҷ�ͼ��srcGray������Ӧ����ֵͼ��
//�����ӳ�����ͼ��dstBinMat��
void getBinaryImageAdaptive(IplImage* srcGray, Mat &dstBinMat)
{
	Mat srcGrayMat(srcGray);
	adaptiveThreshold(srcGrayMat, dstBinMat, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 9, 8);
}

IplImage* getNumberBlobs(IplImage *srcBinary, list<BlobIndex> &blobList)
{
	blobList.clear();
	BlobIndex biTemp;
	CBlobResult blobs;
	CBlob *currentBlob;
	IplImage *blobRGB = cvCreateImage(cvGetSize(srcBinary), IPL_DEPTH_8U, 3);
	blobs = CBlobResult( srcBinary, NULL, 0 );												//Ѱ�Ҷ�ֵͼ�е�blobs
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 10 );
	cvMerge(  srcBinary,  srcBinary,  srcBinary, NULL, blobRGB );		
	for (int j = 0; j < blobs.GetNumBlobs(); j++ )
	{
		currentBlob = blobs.GetBlob(j);
		CvRect rct = currentBlob->GetBoundingBox();										//�õ���ǰ��ͨ���������
		int wi = rct.width;
		int he = rct.height;
		int Height = srcBinary->height;
		int Width = srcBinary->width;
		if (	/*wi*1.0/he > 0.2 && wi*1.0/he < 5*/											//��߱��˲�
				/*&&*/ wi*he > Width*Height/1.0/10000									//�ַ�����˲�
			)			
		{
			currentBlob->FillBlob( blobRGB, CV_RGB(0, 0, 255));
			//cvRectangle(blobRGB, cvPoint(rct.x, rct.y), cvPoint(rct.x+rct.width, rct.y+rct.height), cvScalar(0, 255, 0), 1);
			biTemp.area = wi*he;
			biTemp.lx = rct.x;
			biTemp.ty = rct.y;
			biTemp.rx = rct.x+rct.width;
			biTemp.by = rct.y+rct.height;
			biTemp.cx = rct.x+rct.width/2;
			biTemp.cy = rct.y+rct.height/2;
			biTemp.width = rct.width;
			biTemp.height = rct.height;
			biTemp.yIndex = -1;
			biTemp.xIndex = -1;
			biTemp.label = -1;
			blobList.push_back(biTemp);
		}
	}

	return blobRGB;
}

//ȥ���������, ɾ���������������ͨ��
//blobListΪ������ͨ����Ϣ�ṹ��ɵ��б�
//����ȥ�����ź����Ч�����
int deleteBracket(list<BlobIndex> &blobList, IplImage *blobRGB)
{
	//�ȸ�ݿ��������
	blobList.sort(BlobAreaSortRule);
	//��ȥ���������������
	list<BlobIndex>::iterator itor = blobList.begin();
	list<BlobIndex>::iterator itorTemp = itor++;
	blobList.erase(itorTemp);
	itorTemp = itor++;
	blobList.erase(itorTemp);
	int nBlobNum = 0;	//ȥ�����ź����Ч�����
	for (itor = blobList.begin(); itor !=blobList.end(); itor++)		
	{
		cvRectangle(blobRGB, cvPoint((*itor).lx, (*itor).ty), cvPoint((*itor).rx, (*itor).by), cvScalar(0, 255, 0), 2);
		nBlobNum++;
	}
	return nBlobNum;
}

//ʹ����ͨ��Ŀ�Ⱥͳ��Ⱦ�ֵ���Ʒ�����ֵ
int getThreshold(list<BlobIndex> &blobList, BlobIndex *blobArray, int nBlobNum)
{
	int k = 0, clThresh = 0;
	for (list<BlobIndex>::iterator itor = blobList.begin(); itor !=blobList.end(); itor++)		
	{
		clThresh += (*itor).width;
		clThresh += (*itor).height;
		blobArray[k++] = (BlobIndex)(*itor);
	}
	clThresh = clThresh/nBlobNum;

	return clThresh;
}

//��ݷ�����ֵ�����ֿ���ǩ, ������־���ĸ���Ԫ��
//nBlobNumΪ��Ч�����, clThreshΪ���ڷ������ֵ
//blobArrayΪ������Ч����Ϣ������, ���ؾ����е�Ԫ�ظ���
int addLabel4Blob(BlobIndex *blobArray, int nBlobNum, int clThresh)
{
	int nLabel = 0;
	for (int i = 0; i < nBlobNum; i++)
	{
		for (int j = 0; j < nBlobNum; j++)
		{
			int dx = abs(blobArray[i].cx-blobArray[j].cx);
			int dy = abs(blobArray[i].cy-blobArray[j].cy);
			if (dx < clThresh && dy < clThresh)
			{
				if (-1==blobArray[i].label && -1==blobArray[j].label)
				{
					blobArray[i].label = nLabel;
					blobArray[j].label = nLabel;
					nLabel++;
				}
				else if (-1==blobArray[i].label)
				{
					blobArray[i].label = blobArray[j].label;
				}
				else if (-1==blobArray[j].label)
				{
					blobArray[j].label = blobArray[i].label;
				}
			}
		}
	}
	return nLabel;
}

//��ݱ�ǩֵ��������Ԫ��ֵ, ������Ԫ��ֵ����pblobArrayRefined��
void reCombineElements(BlobIndex *blobArray, BlobIndex *pblobArrayRefined, IplImage *blobRGB, int nLabel, int nBlobNum)
{
	for (int i = 0; i < nLabel; i++)
	{
		int minx = 10000, maxx = -1, miny = 10000, maxy = -1;
		//Ѱ�Ҿ����i��Ԫ�ص�������ɲ���
		for (int j = 0; j < nBlobNum; j++)
		{
			if (i == blobArray[j].label)
			{
				if (blobArray[j].lx < minx)		minx = blobArray[j].lx;
				if (blobArray[j].ty < miny)		miny = blobArray[j].ty;
				if (blobArray[j].rx > maxx)	maxx = blobArray[j].rx;
				if (blobArray[j].by > maxy)	maxy = blobArray[j].by;
			}
		}
		//cvRectangle(blobRGB, cvPoint(minx, miny), cvPoint(maxx, maxy), cvScalar(0, 0, 255), 1);
		pblobArrayRefined[i].lx = minx;
		pblobArrayRefined[i].ty = miny;
		pblobArrayRefined[i].rx = maxx;
		pblobArrayRefined[i].by = maxy;
		pblobArrayRefined[i].cx = (minx+maxx)/2;
		pblobArrayRefined[i].cy = (miny+maxy)/2;
		pblobArrayRefined[i].width = maxx-minx;
		pblobArrayRefined[i].height = maxy-miny;
		pblobArrayRefined[i].xIndex = -1;
		pblobArrayRefined[i].yIndex = -1;
		pblobArrayRefined[i].label = -1;
	}
	for (int i = 0; i < nLabel; i++)
	{
		cvRectangle(blobRGB, cvPoint(pblobArrayRefined[i].lx, pblobArrayRefined[i].ty), cvPoint(pblobArrayRefined[i].rx, pblobArrayRefined[i].by), cvScalar(0, 0, 255), 1);
	}
}

//pblobArrayRefinedΪ�ϲ��������, nLabelΪ������ĳ���
//clThreshΪ���ڷ������ֵ
//���¾������Ԫ�ص��к�XX.ylabel, ����ֵΪ���������
int getRowIndex(BlobIndex *pblobArrayRefined, int nLabel, int clThresh)
{
	BlobIndex biTemp;
	//���Ԫ�ص�����y���ֵ������������(ð������)
	for(int j = 0; j < nLabel-1; j++)
	{
		for(int i = 0; i < nLabel-1-j; i++)
		{
			if(pblobArrayRefined[i].cy > pblobArrayRefined[i+1].cy)
			{
				biTemp = pblobArrayRefined[i];
				pblobArrayRefined[i] = pblobArrayRefined[i+1];
				pblobArrayRefined[i+1] = biTemp;
			}
		}
	}
	//�����ֵ���з���
	int nRows = 0;
	for (int i = 0; i < nLabel; i++)
	{
		for (int j = 0; j < nLabel; j++)
		{
			int dy = abs(pblobArrayRefined[i].cy-pblobArrayRefined[j].cy);
			if (dy < clThresh)	//���С����ֵ, ����Ϊ��ͬһ��
			{
				if (-1==pblobArrayRefined[i].yIndex && -1==pblobArrayRefined[j].yIndex)
				{
					pblobArrayRefined[i].yIndex = nRows;
					pblobArrayRefined[j].yIndex = nRows;
					nRows++;
				}
				else if (-1==pblobArrayRefined[i].yIndex)
				{
					pblobArrayRefined[i].yIndex = pblobArrayRefined[j].yIndex;
				}
				else if (-1==pblobArrayRefined[j].yIndex)
				{
					pblobArrayRefined[j].yIndex = pblobArrayRefined[i].yIndex;
				}
			}
		}
	}
	return nRows;
}

//pblobArrayRefinedΪ�ϲ��������, nLabelΪ������ĳ���
//clThreshΪ���ڷ������ֵ
//���¾������Ԫ�ص��к�XX.xlabel, ����ֵΪ���������
int getColumnIndex(BlobIndex *pblobArrayRefined, int nLabel, int clThresh)
{
	BlobIndex biTemp;
	//���Ԫ�ص�����x���ֵ������������(ð������)
	for(int j = 0; j < nLabel-1; j++)
	{
		for(int i = 0; i < nLabel-1-j; i++)
		{
			if(pblobArrayRefined[i].cx > pblobArrayRefined[i+1].cx)
			{
				biTemp = pblobArrayRefined[i];
				pblobArrayRefined[i] = pblobArrayRefined[i+1];
				pblobArrayRefined[i+1] = biTemp;
			}
		}
	}
	//�����ֵ���з���
	int nCols = 0;
	for (int i = 0; i < nLabel; i++)
	{
		for (int j = 0; j < nLabel; j++)
		{
			int dx = abs(pblobArrayRefined[i].cx-pblobArrayRefined[j].cx);
			if (dx < clThresh)	//���С����ֵ, ����Ϊ��ͬһ��
			{
				if (-1==pblobArrayRefined[i].xIndex && -1==pblobArrayRefined[j].xIndex)
				{
					pblobArrayRefined[i].xIndex = nCols;
					pblobArrayRefined[j].xIndex = nCols;
					nCols++;
				}
				else if (-1==pblobArrayRefined[i].xIndex)
				{
					pblobArrayRefined[i].xIndex = pblobArrayRefined[j].xIndex;
				}
				else if (-1==pblobArrayRefined[j].xIndex)
				{
					pblobArrayRefined[j].xIndex = pblobArrayRefined[i].xIndex;
				}
			}
		}
	}
	return nCols;
}

//��˳�����о���Ԫ��, ����nCol*nRow==nLabel
void getSortedMatrix(BlobIndex *pblobArraySorted, BlobIndex *pblobArrayRefined, int nCol, int nRow, int nLabel)
{
	for (int i = 0; i < nRow; i++)
	{
		for (int j = 0; j < nCol; j++)
		{
			//Ϊÿһ��λ���ҵ���Ӧ��Ԫ��
			for (int k = 0; k < nLabel; k++)
			{
				if (i==pblobArrayRefined[k].yIndex && j==pblobArrayRefined[k].xIndex)
				{
					pblobArraySorted[i*nCol+j] = pblobArrayRefined[k];
				}
			}
		}
	}
}

typedef struct NumberGroup
{
	IplImage *img;
	char *chArray;
	int chLength;
	double value;
}NumberGroup;
typedef struct SingleNumber 
{
	int lx;
	int ty;
	int width;
	int height;
	int cx;
	int cy;
	char value;
}SingleNumber;
bool BlobCenterXSortRule(const SingleNumber &leftData, const SingleNumber &rightData)
{
	return leftData.cx < rightData.cx;
}
struct svm_model* svModel;

//ʶ��srcBinary�е�ROI���������, ROI��sn�ṹָ��
char recognizeNumChar(SingleNumber sn, IplImage *srcBinary)
{
	CvRect rt;
	rt.x = sn.lx;
	rt.y = sn.ty;
	rt.width = sn.width;
	rt.height = sn.height;
	cvSetImageROI(srcBinary, rt);
	IplImage *tmp = cvCreateImage(cvSize(32, 32), IPL_DEPTH_8U, 1);
	cvResize(srcBinary, tmp, CV_INTER_LINEAR);
	cvThreshold(tmp, tmp, 25, 255, CV_THRESH_OTSU);

	struct svm_node *node = (struct svm_node *) malloc(1025*sizeof(struct svm_node));	//svm�ڵ�(index:value)
	for(int y = 0; y < tmp->height; y++)
	{
		for(int x = 0; x < tmp->width; x++)
		{
			int index = y*tmp->width+x;
			node[index].index = index+1;
			node[index].value = (cvGet2D(tmp, y, x).val[0] > 128 ? 1 : 0);                                
		}      
	}
	node[1024].index = -1;
	int intPredict = (int)svm_predict(svModel, node);

	cvReleaseImage(&tmp);
	cvResetImageROI(srcBinary);
	return '0' + (intPredict == 10 ? 1 : intPredict);
}

//�õ�һ���������ʶ����, ��������nubGroup.value��
void getNumGroupValue(NumberGroup &nubGroup)
{
	cvDilate(nubGroup.img, nubGroup.img, NULL, 1);		//����
	cvErode(nubGroup.img, nubGroup.img, NULL, 1);		//��ʴ
	list<SingleNumber> snList;
	snList.clear();

	//��ȡһ���������е���ͨ��
	CBlobResult blobs;
	CBlob *currentBlob;
	blobs = CBlobResult( nubGroup.img, NULL, 0 );			//Ѱ�Ҷ�ֵͼ�е�blobs
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 10 );	
	for (int j = 0; j < blobs.GetNumBlobs(); j++ )
	{
		currentBlob = blobs.GetBlob(j);
		CvRect rct = currentBlob->GetBoundingBox();										//�õ���ǰ��ͨ���������
		int wi = rct.width;
		int he = rct.height;
		int Height = nubGroup.img->height;
		int Width = nubGroup.img->width;
		if (	/*wi*1.0/he > 0.2 && wi*1.0/he < 5*/											//��߱��˲�
			/*&&*/ wi*he > Width*Height/1.0/500											//�ַ�����˲�
			)			
		{
			SingleNumber sn;
			sn.lx = rct.x;
			sn.ty = rct.y;
			sn.width = rct.width;
			sn.height = rct.height;
			sn.cx = rct.x+rct.width/2;
			sn.cy = rct.y+rct.height/2;
			sn.value = '@';
			snList.push_back(sn);
		}
	}
	//���X����������, ���ڷ���
	snList.sort(BlobCenterXSortRule);
	//���������ж�С������ֵ
	int num = 0, meanlen = 0;
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		meanlen += (*itor).width;
		meanlen += (*itor).height;
		num++;
	}
	meanlen /= num;
	//��ݳ����ж�С���"."������"-"��"1"
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		if ( ((*itor).width+(*itor).height)*3 < meanlen )
		{
			(*itor).value = '.';
		}
		if ( (*itor).width > 4* (*itor).height )
		{
			(*itor).value = '-';
		}
		if ( (*itor).height >= 5* (*itor).width )
		{
			(*itor).value = '1';
		}
	}
	//ʶ�����������ַ�
	int chLength = 0;
	LOGI("before using svm model");
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		if ((*itor).value == '@')
		{
			(*itor).value = recognizeNumChar(*itor, nubGroup.img);
		}
		chLength++;
	}
	nubGroup.chLength = chLength+1;
	nubGroup.chArray = new char[nubGroup.chLength+1];
	int i = 0;
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++, i++)
	{
		*(nubGroup.chArray+i) = (*itor).value;
	}
	*(nubGroup.chArray+i) = '\0';
	nubGroup.value = atof(nubGroup.chArray);
	//cout<<nubGroup.value<<"\t\t"<<nubGroup.chLength<<endl;

	delete []nubGroup.chArray;
	cvReleaseImage(&nubGroup.img);
	snList.clear();
}

//ʶ����󲢽������Json��ʽ���ַ���
char* getJsonMatrix(IplImage *srcRGB)
{
	//ͼ��Ԥ����, �õ���ֵ��ͼ��
	LOGI("get binary image");
	IplImage *srcGray = cvCreateImage(cvGetSize(srcRGB), IPL_DEPTH_8U, 1);
	cvCvtColor(srcRGB, srcGray, CV_BGR2GRAY); 
	IplImage *smallGray = ResizeInputImage(srcGray);
	cvReleaseImage(&srcGray);
	Mat binaryMat;
	getBinaryImageAdaptive(smallGray, binaryMat);
	cvReleaseImage(&smallGray);
	IplImage binaryImg = binaryMat;	

	//��ͨ����ȡ, ����˲�, ���������ֵ
	LOGI("extract blobs");	
	list<BlobIndex> blobList;
	IplImage *blobRGB = getNumberBlobs(&binaryImg, blobList);
	//cvShowImage("blobRGB1", blobRGB);
	int nBlobNum = deleteBracket(blobList, blobRGB);
	//cvShowImage("blobRGB2", blobRGB);
	BlobIndex *blobArray = new BlobIndex[nBlobNum];
	int clThresh = getThreshold(blobList, blobArray, nBlobNum);
	blobList.clear();

	//��ݷ�����ֵ����ͨ����ǩ, ��ݱ�ǩ�����ط���
	LOGI("re-combine blobs");
	int nLabel = addLabel4Blob(blobArray, nBlobNum, clThresh);
	BlobIndex *pblobArrayRefined = new BlobIndex[nLabel];
	reCombineElements(blobArray, pblobArrayRefined, blobRGB, nLabel, nBlobNum);
	//cvShowImage("blobRGB3", blobRGB);
	cvReleaseImage(&blobRGB);
	delete []blobArray;

	//�õ���������������, �Լ�ÿһ��Ԫ�ص��������
	int nCol = getColumnIndex(pblobArrayRefined, nLabel, clThresh);
	int nRow = getRowIndex(pblobArrayRefined, nLabel, clThresh);
	if (nCol*nRow != nLabel)
	{
		char strError[] = "{\"end\":\"error\"}\0";
		return strError;
	}

	//��˳�����о���Ԫ��
	BlobIndex *pblobArraySorted = new BlobIndex[nLabel];
	getSortedMatrix(pblobArraySorted, pblobArrayRefined, nCol, nRow, nLabel);
	delete []pblobArrayRefined;

	LOGI("create number group");	
	NumberGroup *nubGroup = new NumberGroup[nLabel];
	for (int i = 0; i < nLabel; i++)
	{
		//cout<<pblobArraySorted[i].cx<<'\t'<<pblobArraySorted[i].cy<<'\t'<<pblobArraySorted[i].xIndex<<'\t'<<pblobArraySorted[i].yIndex<<endl;
		CvRect rt;
		rt.x = pblobArraySorted[i].lx-1;
		rt.y = pblobArraySorted[i].ty-1;
		rt.width = pblobArraySorted[i].width+2;
		rt.height = pblobArraySorted[i].height+2;
		cvSetImageROI(&binaryImg, rt);
		IplImage *tmp = cvCreateImage(cvGetSize(&binaryImg), IPL_DEPTH_8U, 1);
		cvCopyImage(&binaryImg, tmp);
		nubGroup[i].img = tmp;
		cvResetImageROI(&binaryImg);
	}
	binaryMat.release();		//�ͷ�binaryMat, Ҳ���ͷ�binaryImg
	delete []pblobArraySorted;

	//char strName[100];
	//for (int i = 0; i < nLabel; i++)
	//{
	//	sprintf(strName, "00%d", i);
	//	cvShowImage(strName, nubGroup[i].img);
	//	//cvWaitKey(0);
	//}

	//svModelΪѵ���õ���svmģ��, �þ����svmģ��
	LOGI("load svm model");
	svModel = svm_load_model("/sdcard/MatrixCalculator/svm_classify.model");
	LOGI("just after svm model");
	//��ʶ������֯��ΪJson��ʽ���ַ���
	char stResult[1024];
	memset(stResult, '\0', sizeof(char)*1024);
	sprintf(stResult, "{\"row\":%d,\"col\":%d,", nRow, nCol);
	for (int i = 0; i < nLabel; i++)
	{
		getNumGroupValue(nubGroup[i]);
		char strTemp[32];
		sprintf(strTemp, "\"%d\":%f,", i, nubGroup[i].value);
		strcat(stResult, strTemp);
	}
	strcat(stResult, "\"end\":\"ok\"}\0");
	delete []nubGroup;

	svm_free_and_destroy_model(&svModel);
	LOGI("release svm model");
	return stResult;
}

