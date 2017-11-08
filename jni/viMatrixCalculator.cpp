#include "viMatrixCalculator.h"
#include <algorithm>
#include <android/log.h>

using namespace cv;

#define FIXED_WIDTH	400/*600*/
#define  LOG_TAG    "Native_MatrixRecognition"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

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

//�Զ��� list �������(����blob���)
bool BlobAreaSortRule(const BlobIndex &leftData, const BlobIndex &rightData)
{
	return leftData.area > rightData.area;
}

//�������������������ͼ��
IplImage* ResizeInputImage(IplImage *src)
{
	//������С���Ŀ��ͼ��
	IplImage *newImg = cvCreateImage(cvSize(FIXED_WIDTH, src->height*FIXED_WIDTH/1.0/src->width), src->depth, src->nChannels);
	cvResize(src, newImg, CV_INTER_LINEAR);
	return newImg;
}

//��������Ҷ�ͼ��srcGray������Ӧ����ֵͼ��
//�������ӳ�����ͼ��dstBinMat��
void getBinaryImageAdaptive(IplImage* srcGray, Mat &dstBinMat)
{
	Mat srcGrayMat(srcGray);
	adaptiveThreshold(srcGrayMat, dstBinMat, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 11, 9);
}

void getNumberBlobs(IplImage *srcBinary, list<BlobIndex> &blobList)
{
	blobList.clear();
	BlobIndex biTemp;
	CBlobResult blobs;
	CBlob *currentBlob;
	//IplImage *blobRGB = cvCreateImage(cvGetSize(srcBinary), IPL_DEPTH_8U, 3);
	blobs = CBlobResult( srcBinary, NULL, 0 );												//Ѱ�Ҷ�ֵͼ�е�blobs
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 24);
	//cvMerge(  srcBinary,  srcBinary,  srcBinary, NULL, blobRGB );		
	for (int j = 0; j < blobs.GetNumBlobs(); j++ )
	{
		currentBlob = blobs.GetBlob(j);
		CvRect rct = currentBlob->GetBoundingBox();										//�õ���ǰ��ͨ����������
		int wi = rct.width;
		int he = rct.height;
		int Height = srcBinary->height;
		int Width = srcBinary->width;
		if (	/*wi*1.0/he > 0.2 && wi*1.0/he < 5*/											//��߱��˲�
				/*&&*/ wi*he > Width*Height/1.0/8100										//�ַ�����˲�
			)			
		{
			//currentBlob->FillBlob( blobRGB, CV_RGB(0, 0, 255));
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

	//return blobRGB;
}

//ȥ����������, ɾ���������������ͨ��
//blobListΪ������ͨ����Ϣ�ṹ��ɵ��б�
//����ȥ�������ź����Ч�����
int deleteBracket(list<BlobIndex> &blobList/*, IplImage *blobRGB*/)
{
	//�ȸ��ݿ��������
	blobList.sort(BlobAreaSortRule);
	if (blobList.size() <= 2)
		return -1;
	//��ȥ���������������
	list<BlobIndex>::iterator itor = blobList.begin();
	list<BlobIndex>::iterator itorTemp = itor++;
	blobList.erase(itorTemp);
	itorTemp = itor++;
	blobList.erase(itorTemp);
	int nBlobNum = 0;	//ȥ�������ź����Ч�����
	for (itor = blobList.begin(); itor !=blobList.end(); itor++)		
	{
		//cvRectangle(blobRGB, cvPoint((*itor).lx, (*itor).ty), cvPoint((*itor).rx, (*itor).by), cvScalar(0, 255, 0), 2);
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

//ʶ��srcBinary�е�ROI���������, ROI��sn�ṹָ��
char recognizeNumChar(SingleNumber sn, IplImage *srcBinary, ittFann &ittfann2)
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

	fann_type fannInput[1024];
	for (int i = 0; i < tmp->height; i++)
	{
		for (int j = 0; j < tmp->width; j++)
		{
			char chTmp = ((uchar *)(tmp->imageData+i*tmp->widthStep))[j];
			fannInput[i*32+j] = -(int)(chTmp);
		}
	}
	fann_type *fannOutput = ittfann2.fannPredict(fannInput, 1024);
	int maxv = -10, maxi = -1;
	for (unsigned int k = 0; k < ittfann2.m_ann->num_output; k++)
	{
		if (fannOutput[k] > maxv)
		{
			maxv = fannOutput[k];
			maxi = k;
		}
	}
	
	cvReleaseImage(&tmp);
	cvResetImageROI(srcBinary);
	return '0' + maxi;
}

//�õ�һ���������ʶ����, ��������nubGroup.value��
void getNumGroupValue(NumberGroup &nubGroup, ittFann &ittfann2)
{
	//cvDilate(nubGroup.img, nubGroup.img, NULL, 1);		//����
	//cvErode(nubGroup.img, nubGroup.img, NULL, 1);		//��ʴ
	list<SingleNumber> snList;
	snList.clear();

	//��ȡһ���������е���ͨ��
	CBlobResult blobs;
	CBlob *currentBlob;
	blobs = CBlobResult( nubGroup.img, NULL, 0 );			//Ѱ�Ҷ�ֵͼ�е�blobs
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 10);	
	for (int j = 0; j < blobs.GetNumBlobs(); j++ )
	{
		currentBlob = blobs.GetBlob(j);
		CvRect rct = currentBlob->GetBoundingBox();										//�õ���ǰ��ͨ����������
		int wi = rct.width;
		int he = rct.height;
		int Height = nubGroup.img->height;
		int Width = nubGroup.img->width;
		if (	/*wi*1.0/he > 0.2 && wi*1.0/he < 5*/											//��߱��˲�
			/*&&*/ wi*he > Width*Height/1.0/1000											//�ַ�����˲�
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
	//����X�����������, ���ڷ���
	snList.sort(BlobCenterXSortRule);
	//���������ж�С�������ֵ
	int num = 0, meanlen = 0;
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		meanlen += (*itor).width;
		meanlen += (*itor).height;
		num++;
	}
	meanlen /= num;
	//���ݳ����ж�С����"."������"-"��"1"
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		if ( ((*itor).width+(*itor).height)*2 < meanlen )
		{
			(*itor).value = '.';
		}
		if ( (*itor).width > 3* (*itor).height )
		{
			(*itor).value = '-';
		}
		if ( (*itor).height >= 5*(*itor).width && 5*(*itor).width < meanlen)
		{
			(*itor).value = '1';
		}
	}
	//ʶ�����������ַ�
	int chLength = 0;
	for (list<SingleNumber>::iterator itor = snList.begin(); itor != snList.end(); itor++)
	{
		if ((*itor).value == '@')
		{
			(*itor).value = recognizeNumChar(*itor, nubGroup.img, ittfann2);
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////141111
//�Զ��� list �������(����blobY����)
bool BlobCenterYSortRule(const BlobIndex &leftData, const BlobIndex &rightData)
{
	return leftData.cy < rightData.cy;
}
//�Զ��� list �������(����blobX����)
bool BlobCenterXSortRule2(const BlobIndex &leftData, const BlobIndex &rightData)
{
	return leftData.cx < rightData.cx;
}
int getRowIndex(int nBlobNum, BlobIndex *blobArray, int clThresh)
{
	int yclThresh = clThresh/2;
	int yLabel = 0;
	blobArray[0].yIndex = yLabel;
	for (int i = 0; i < nBlobNum-1; i++)
	{
		if (blobArray[i+1].cy-blobArray[i].cy < yclThresh)
		{
			blobArray[i+1].yIndex = blobArray[i].yIndex;
		}
		else
		{
			yLabel++;
			blobArray[i+1].yIndex = yLabel;
		}
	}
	return yLabel+1;
}
int getColumnIndex(int nBlobNum, BlobIndex *blobArray, int clThresh, int nRow)
{
	vector<BlobIndex> *blobListArray = new vector<BlobIndex>[nRow];
	for (int i = 0; i < nRow; i++)
	{
		blobListArray[i].clear();
		for (int j = 0; j < nBlobNum; j++)
		{
			if (blobArray[j].yIndex == i)
			{
				blobListArray[i].push_back(blobArray[j]);
			}
		}
		std::sort(blobListArray[i].begin(), blobListArray[i].end(), BlobCenterXSortRule2);
	}

	int *xLabel = new int[nRow];
	memset(xLabel, 0, sizeof(int)*nRow);
	for (int i = 0; i < nRow; i++)
	{
		blobListArray[i][0].xIndex = xLabel[i];
		for (int j = 0; j < blobListArray[i].size()-1; j++)		
		{
			if (blobListArray[i][j+1].cx-blobListArray[i][j].cx < 3*clThresh/4)
			{
				blobListArray[i][j+1].xIndex = blobListArray[i][j].xIndex;
			}
			else
			{
				xLabel[i]++;
				blobListArray[i][j+1].xIndex = xLabel[i];
			}
		}
	}

	int k, nCol = 0;
	for (k = 1; k < nRow; k++)
	{
		if (xLabel[k] != xLabel[0])
			break;
	}
	if (k != nRow )
		nCol = -2;
	else
	{
		nCol = xLabel[0];
		k = 0;
		for (int i = 0; i < nRow; i++)
		{
			for (int j = 0; j < blobListArray[i].size(); j++)	
			{
				blobArray[k] = blobListArray[i][j];
				blobArray[k].label = blobArray[k].yIndex*(nCol+1)+blobArray[k].xIndex;
				k++;
			}
		}
	}

	delete []xLabel;
	for (int i = 0; i < nRow; i++)
	{
		blobListArray[i].clear();
	}
	delete []blobListArray;

	return nCol+1;
}
//���ݱ�ǩֵ��������Ԫ��ֵ, ������Ԫ��ֵ����pblobArrayRefined��
void reCombineElements(BlobIndex *blobArray, BlobIndex *pblobArrayRefined, /*IplImage *blobRGB, */int nBlobNum, int nRow, int nCol)
{
	for (int row = 0; row < nRow; row++)
	{
		for (int col = 0; col < nCol; col++)
		{
			int i = row*nCol+col;
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
			pblobArrayRefined[i].xIndex = col;
			pblobArrayRefined[i].yIndex = row;
			pblobArrayRefined[i].label = i;
		}
	}
	//int nLabel = nRow*nCol;
	//for (int i = 0; i < nLabel; i++)
	//{
	//	cvRectangle(blobRGB, cvPoint(pblobArrayRefined[i].lx, pblobArrayRefined[i].ty), cvPoint(pblobArrayRefined[i].rx, pblobArrayRefined[i].by), cvScalar(0, 0, 255), 1);
	//}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//ʶ����󲢽������Json��ʽ���ַ�������
char* getJsonMatrix(IplImage *srcRGB)
{
	//ͼ��Ԥ����, �õ���ֵ��ͼ��
	IplImage *srcGray = cvCreateImage(cvGetSize(srcRGB), IPL_DEPTH_8U, 1);
	cvCvtColor(srcRGB, srcGray, CV_BGR2GRAY); 
	IplImage *smallGray = ResizeInputImage(srcGray);
	//cvSaveImage("gray.jpg", smallGray);
	cvReleaseImage(&srcGray);
	Mat binaryMat;
	getBinaryImageAdaptive(smallGray, binaryMat);
	cvReleaseImage(&smallGray);
	IplImage binaryImg = binaryMat;	
	//cvSaveImage("binary.jpg", &binaryImg);

	//��ͨ����ȡ, ����˲�, ���������ֵ
	list<BlobIndex> blobList;
	/*IplImage *blobRGB = */getNumberBlobs(&binaryImg, blobList);
	//cvSaveImage("blobRGB1.jpg", blobRGB);
	int nBlobNum = deleteBracket(blobList/*, blobRGB*/);
	if (nBlobNum < 0)
	{
		char strmp[] = "{\"end\":\"error\"}\0";
		char *strError = new char[strlen(strmp)+1];
		memset(strError, '\0', sizeof(char)*(strlen(strmp)+1));
		strcat(strError, strmp);
		return strError;
	}
	//cvSaveImage("blobRGB2.jpg", blobRGB);

	blobList.sort(BlobCenterYSortRule);
	BlobIndex *blobArray = new BlobIndex[nBlobNum];
	int clThresh = getThreshold(blobList, blobArray, nBlobNum);
	blobList.clear();
	int nRow = getRowIndex(nBlobNum, blobArray, clThresh);
	int nCol = getColumnIndex(nBlobNum, blobArray, clThresh, nRow);
	if (nCol < 0)
	{
		char strmp[] = "{\"end\":\"error\"}\0";
		char *strError = new char[strlen(strmp)+1];
		memset(strError, '\0', sizeof(char)*(strlen(strmp)+1));
		strcat(strError, strmp);
		return strError;
	}

	int nLabel = nRow*nCol;
	BlobIndex *pblobArraySorted = new BlobIndex[nLabel];
	reCombineElements(blobArray, pblobArraySorted, /*blobRGB,*/ nBlobNum, nRow, nCol);
	//cvSaveImage("blobRGB3.jpg", blobRGB);
	//cvShowImage("blobRGB3", blobRGB);
	//cvReleaseImage(&blobRGB);
	delete []blobArray;

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

	//modelΪѵ���õ���fannģ��, �þ����fannģ��
	LOGI("before fannCreateFromFile");
	ittFann ittfann2;
	ittfann2.fannCreateFromFile((char*)"/sdcard/MatrixCalculator/fannModel", false);
	LOGI("after fannCreateFromFile");

	//��ʶ������֯��ΪJson��ʽ���ַ�������
	char *stResult = new char[1024];
	memset(stResult, '\0', sizeof(char)*1024);
	sprintf(stResult, "{\"row\":%d,\"col\":%d,", nRow, nCol);
	for (int i = 0; i < nLabel; i++)
	{
		getNumGroupValue(nubGroup[i], ittfann2);
		char strTemp[32];
		sprintf(strTemp, "\"%d\":%f,", i, nubGroup[i].value);
		strcat(stResult, strTemp);
	}
	strcat(stResult, "\"end\":\"ok\"}\0");
	delete []nubGroup;

	ittfann2.fannRelease();
	return stResult;
}

