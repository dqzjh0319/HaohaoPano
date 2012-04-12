#-------------------------------------------------
#
# Project created by QtCreator 2012-03-11T13:38:19
#
#-------------------------------------------------

QT       += core gui

TARGET = MosaicsDemo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    robustmatcher.cpp

HEADERS  += mainwindow.h \
    robustmatcher.h

FORMS    += mainwindow.ui

INCLUDEPATH += /usr/local/include/opencv
LIBS += /usr/local/lib/libopencv_core.so \
/usr/local/lib/libopencv_highgui.so \
/usr/local/lib/libopencv_ml.so \
/usr/local/lib/libopencv_calib3d.so \
/usr/local/lib/libopencv_features2d.so \
/usr/local/lib/libopencv_flann.so \
/usr/local/lib/libopencv_gpu.so \
/usr/local/lib/libopencv_imgproc.so \
/usr/local/lib/libopencv_legacy.so \
/usr/local/lib/libopencv_objdetect.so \
/usr/local/lib/libopencv_ts.so \
/usr/local/lib/libopencv_video.so
