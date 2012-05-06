#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QPersistentModelIndex>
#include <robustmatcher.h>
#include <QtDebug>
#include <fstream>
#include <QQueue>
#include <QStack>
#include <QSize>
#include <QPixmap>
#include <QMessageBox>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
#define MIN_MATCHNO 10

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionImport_triggered();

    void on_pb_import_clicked();

    void on_pb_remove_clicked();

    void on_lv_files_doubleClicked(const QModelIndex &index);

    void on_lv_files_clicked(const QModelIndex &index);

    void on_pb_fDectect_clicked();

    void on_pb_fMatch_clicked();

    void on_tv_imgGroup_doubleClicked(const QModelIndex &index);

    void on_pb_computeH_clicked();

    void on_pb_autoMosaic_clicked();

    void on_pb_fushion_clicked();

    void on_pb_QuitProcess_clicked();

    void on_actionExit_triggered();

    void on_pb_AllinOne_clicked();

private:
    void statusRefresh();
    void pfmFeatherDetect();
    void pfmMatch();
    void pfmcomputeH();
    void pfmAutoMosaic();
    void pfmFusion();

public:
    std::vector<std::vector<cv::KeyPoint> > keyPointVec;
    std::vector<std::vector<std::vector<cv::DMatch> > > matchPairVec;
    std::vector<std::vector<int> > matchMap;
    std::vector<std::vector<cv::Mat> > HomographyVec;
    std::vector<QSize> inImageSizeVec;
    std::vector<cv::Mat> finalH;
    std::vector<cv::Mat> descriptorVec;

private:
    Ui::MainWindow *ui;
    QStandardItemModel listmodel; //mode of the input file listview
    QStringList inFileNames;
    QStandardItemModel processInfoModel;
    QStandardItemModel imageGroupModel;
    QPersistentModelIndex *curIndex; // usage of saving current index for removing

    //prcess status
    bool isImported;
    bool isFeaDetected;
    bool isMatched;
    bool isComputeHed;
    bool isMosaiced;
    bool isFusioned;

    int rstw, rsth;
};

#endif // MAINWINDOW_H


