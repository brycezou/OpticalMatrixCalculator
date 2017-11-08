LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include ../includeOpenCV.mk
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
	#try to load OpenCV.mk from default install location
	include $(TOOLCHAIN_PREBUILT_ROOT)/user/share/OpenCV/OpenCV.mk
else
	include $(OPENCV_MK_PATH)
endif

LOCAL_MODULE    := JniMatrixRecognizer

LOCAL_SRC_FILES := BlobContour.cpp BlobResult.cpp blob.cpp ComponentLabeling.cpp viMatrixCalculator.cpp BlobOperators.cpp main.cpp fann.c fann_error.c fann_train.c floatfann.c fann_cascade.c fann_io.c fann_train_data.c ittFann.cpp

LOCAL_LDLIBS +=  -llog -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
