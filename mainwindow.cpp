#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    statusRefresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionImport_triggered()
{
    QPixmap tmp;
    bool test = tmp.load("FusionResult.png");
    qDebug() << test;
    ui->lbl_mainDisplay->setPixmap(tmp);
}

void MainWindow::statusRefresh()
{
    isImported = false;
    isFeaDetected = false;
    isMatched = false;
    isComputeHed = false;
    isMosaiced = false;
    isFusioned = false;
    inFileNames.clear();
    keyPointVec.clear();
    matchPairVec.clear();
    matchMap.clear();
    HomographyVec.clear();
    inImageSizeVec.clear();
    finalH.clear();
    descriptorVec.clear();
    listmodel.clear();
    processInfoModel.clear();
    imageGroupModel.clear();
    ui->pb_import->setEnabled(true);
    ui->pb_remove->setEnabled(true);
    ui->pb_fDectect->setEnabled(false);
    ui->pb_fMatch->setEnabled(false);
    ui->pb_computeH->setEnabled(false);
    ui->pb_autoMosaic->setEnabled(false);
    ui->pb_AllinOne->setEnabled(false);
    ui->pb_QuitProcess->setEnabled(false);
    ui->pb_fushion->setEnabled(false);
    ui->progressBar->setVisible(false);
    ui->lv_proinfo->setEnabled(false);
    ui->tv_imgGroup->setEnabled(false);
    ui->lbl_mainDisplay->setPixmap(NULL);
    ui->cb_fdetect->setChecked(false);
    ui->cb_computeH->setChecked(false);
    ui->cb_fmatch->setChecked(false);
    ui->cb_Mosaic->setChecked(false);
    ui->cb_fusion->setChecked(false);
}

void MainWindow::on_pb_import_clicked()
{
    QStandardItem *item = new QStandardItem("Name");
    listmodel.setHorizontalHeaderItem(0,item);
    item = new QStandardItem("Path");
    listmodel.setHorizontalHeaderItem(1,item);
    //item = new QStandardItem("haohao.jpg");
    QStringList tmpFileNames = QFileDialog::getOpenFileNames(this,
                                                tr("Choose Input Images"),
                                                ".",
                                                tr("Image Files (*.png *.jpg *.jpeg *.bmp)")
                                                );
    QStringList::iterator strliter = tmpFileNames.begin();
    bool isRepeated = false;
    int rcount = 0;
    for(;strliter != tmpFileNames.end(); ++strliter)
    {
        QStandardItem *item = new QStandardItem((*strliter).toLocal8Bit().constData());
        int tmpcheck = inFileNames.indexOf(*strliter);
        //QList<QStandardItem*> tmp = listmodel.findItems((*strliter).toLocal8Bit().constData());
        if(tmpcheck >= 0)
        {
            isRepeated = true;
            rcount++;
        }
        else
        {
            inFileNames.append(*strliter);
            //qDebug() << inFileNames.size();
            QList<QStandardItem *> rowitem;
            int addrId = (*strliter).lastIndexOf("/");
            QString::Iterator tmpstrit = strliter->begin();
            tmpstrit += addrId + 1;
            QString imageName;
            for(;tmpstrit != strliter->end(); ++ tmpstrit)
            {
                imageName.append(*tmpstrit);
            }
            item = new QStandardItem(imageName);
            rowitem.append(item);
            item = new QStandardItem(*strliter);
            rowitem.append(item);
            listmodel.appendRow(rowitem);
        }
    }
    if(isRepeated)
    {
        QMessageBox mesbox;
        mesbox.setWindowTitle("Warning");
        mesbox.setText(QString::number(rcount) + " repeated image(s) ignored!");
        mesbox.exec();
    }
    if(inFileNames.size() > 1)
    {
        ui->pb_fDectect->setEnabled(true);
        ui->pb_AllinOne->setEnabled(true);
        isImported = true;
    }
    ui->lv_files->setModel(&listmodel);
}

void MainWindow::on_pb_remove_clicked()
{
    //QModelIndexList indexlist = ui->tv_imgGroup->currentIndex();
    inFileNames.removeAt(curIndex->row());
    //qDebug() << inFileNames.size();
    listmodel.removeRow(curIndex->row());
    if(inFileNames.size() < 2)
    {
        ui->pb_fDectect->setEnabled(false);
        ui->pb_AllinOne->setEnabled(false);
        isImported = false;
    }
}

void MainWindow::on_lv_files_doubleClicked(const QModelIndex &index)
{
    QString filename = inFileNames[index.row()];
    QPixmap showimg;
    //qDebug() << index.row();
    cv::Mat tmpshowimg = cv::imread(filename.toAscii().data());
    cv::cvtColor(tmpshowimg,tmpshowimg,CV_BGR2RGB);
    if(isFeaDetected)
    {
        cv::drawKeypoints(tmpshowimg,
                          keyPointVec[index.row()],
                          tmpshowimg,
                          cv::Scalar(255,0,0)
                          //cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS
                          );
    }
    QImage tmpqimg = QImage((const unsigned char*)(tmpshowimg.data),
                        tmpshowimg.cols,tmpshowimg.rows,QImage::Format_RGB888);
    showimg = QPixmap::fromImage(tmpqimg);
    showimg = showimg.scaled(DEFAULT_WIDTH,DEFAULT_HEIGHT,
                                      Qt::KeepAspectRatio);
    ui->lbl_mainDisplay->setPixmap(showimg);
}

void MainWindow::on_lv_files_clicked(const QModelIndex &index)
{
    curIndex = new QPersistentModelIndex(index);
}

void MainWindow::pfmFeatherDetect()
{
    ui->pb_import->setEnabled(false);
    ui->pb_remove->setEnabled(false);
    ui->pb_computeH->setEnabled(false);
    ui->pb_autoMosaic->setEnabled(false);
    ui->pb_fMatch->setEnabled(false);
    ui->pb_fushion->setEnabled(false);
    ui->lv_proinfo->setEnabled(true);
    ui->pb_QuitProcess->setEnabled(false);
    std::ofstream proData;
    proData.open("ProcessInfo.dat");
    QStringList::iterator nameIter;
    nameIter = inFileNames.begin();
    RobustMatcher rMatcher;
    ui->progressBar->setEnabled(true);
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0,inFileNames.size());
    ui->lv_proinfo->setModel(&processInfoModel);
    for(int i = 1;nameIter != inFileNames.end(); ++nameIter, ++i)
    {
        cv::Mat tmpImage = cv::imread((*nameIter).toLocal8Bit().constData());
        cv::cvtColor(tmpImage,tmpImage,CV_BGR2RGB);
        //saving the size of input images
        QSize tmpsize(tmpImage.cols,tmpImage.rows);
        inImageSizeVec.push_back(tmpsize);
        std::vector<cv::KeyPoint> tmpkp;
        cv::Mat tmpdes;
        rMatcher.featherDectect(tmpImage, tmpkp, tmpdes);
        keyPointVec.push_back(tmpkp);
        descriptorVec.push_back(tmpdes);
        proData << "File Name: " << (*nameIter).toLocal8Bit().constData() << std::endl;
        proData << "KeyPoint No.: " << tmpkp.size() << std::endl;
        proData << std::endl;
        QStandardItem *tmpitem =
                new QStandardItem(*nameIter + "Feather Dectected Successfully");
        QStandardItem *tmpitemfno =
                new QStandardItem(QString::number(tmpkp.size()) + " Points Detected");
        processInfoModel.appendRow(tmpitem);
        processInfoModel.appendRow(tmpitemfno);
        ui->progressBar->setValue(i);
        ui->lv_proinfo->scrollToBottom();
    }
    proData.close();
    isFeaDetected = true;
    ui->progressBar->setVisible(false);
    ui->pb_fMatch->setEnabled(true);
    ui->pb_QuitProcess->setEnabled(true);
    ui->pb_fDectect->setEnabled(false);
    ui->cb_fdetect->setChecked(true);
}

void MainWindow::on_pb_fDectect_clicked()
{
    if(!isImported)
    {
        QMessageBox qmbox;
        qmbox.setText("Less than 2 Input Images!!");
        qmbox.exec();
        return;
    }

    pfmFeatherDetect();

}

void MainWindow::pfmMatch()
{
    ui->tv_imgGroup->setEnabled(true);
    ui->pb_QuitProcess->setEnabled(false);
    RobustMatcher rMatcher;
    std::ofstream matchinfo;
    matchinfo.open("matchinfo.dat");
    int pgv = 1;
    ui->progressBar->setRange(0,inFileNames.size() * inFileNames.size());
    ui->progressBar->setVisible(true);
    QStandardItem *item = imageGroupModel.invisibleRootItem();
    for(int i = 0;i < inFileNames.size(); ++i )
    {
        QStandardItem *itit = new QStandardItem("Image " + QString::number(i));
        item->appendRow(itit);
        std::vector<std::vector<cv::DMatch> > tmpdm;
        std::vector<int> tmpmmap;
        for(int j = 0; j < inFileNames.size(); ++j, ++pgv)
        {
            if(i == j)
                continue;
            std::vector<cv::DMatch> tmpsingledm;
            rMatcher.match(descriptorVec[i], descriptorVec[j], tmpsingledm);
            if(tmpsingledm.size() < MIN_MATCHNO)
                continue;
            tmpdm.push_back(tmpsingledm);
            tmpmmap.push_back(j);
            matchinfo << i << " to " <<  j<< std::endl
                      << tmpsingledm.size() << " matching pairs"
                      << std::endl << std::endl;
            QStandardItem *ititit =
                    new QStandardItem("Image " + QString::number(j) + ": "
                                      + QString::number(tmpsingledm.size())
                                      + " pairs");
            itit->appendRow(ititit);
            QStandardItem *tmpitem
                    = new QStandardItem(QString::number(i) +" to "
                                        + QString::number(j) + " "
                                        + QString::number(tmpsingledm.size()) +
                                        " pairs");
            processInfoModel.appendRow(tmpitem);
            ui->lv_proinfo->scrollToBottom();
            ui->progressBar->setValue(pgv);
        }
        matchPairVec.push_back(tmpdm);
        matchMap.push_back(tmpmmap);
    }
    matchinfo.close();
    ui->progressBar->setVisible(false);
    ui->tv_imgGroup->setModel(&imageGroupModel);
    ui->pb_computeH->setEnabled(true);
    ui->pb_fMatch->setEnabled(false);
    ui->pb_QuitProcess->setEnabled(true);
    isMatched = true;
    ui->cb_fmatch->setChecked(true);
}

void MainWindow::on_pb_fMatch_clicked()
{
    if(!isFeaDetected)
    {
        QMessageBox qmbox;
        qmbox.setText("exec Feather Detecting please!");
        qmbox.exec();
        return;
    }

    pfmMatch();
}

void MainWindow::on_tv_imgGroup_doubleClicked(const QModelIndex &index)
{
    if(!isMatched)
    {
        QMessageBox qmBox;
        qmBox.setText("Match Images First Please!");
        qmBox.exec();
        return;
    }
    QModelIndex parIndex = index.parent();
    //qDebug() << parIndex.row() << ", " << index.row();
    int ref = parIndex.row();
    int cmp = matchMap[ref][index.row()];
    cv::Mat refImage = cv::imread(inFileNames[ref].toLocal8Bit().constData());
    cv::cvtColor(refImage,refImage,CV_BGR2RGB);
    cv::Mat cmpImage = cv::imread(inFileNames[cmp].toLocal8Bit().constData());
    cv::cvtColor(cmpImage,cmpImage,CV_BGR2RGB);
    cv::Mat matchImage;
    cv::drawMatches(refImage, keyPointVec[ref], cmpImage,
                    keyPointVec[cmp], matchPairVec[ref][index.row()], matchImage);
    QImage tmpImage = QImage((const unsigned char*)(matchImage.data),
        matchImage.cols,matchImage.rows,QImage::Format_RGB888);
    QPixmap showimg = QPixmap::fromImage(tmpImage);
    showimg = showimg.scaled(DEFAULT_WIDTH, DEFAULT_HEIGHT, Qt::KeepAspectRatio);
    ui->lbl_mainDisplay->setPixmap(showimg);
}

void MainWindow::pfmcomputeH()
{
    ui->pb_QuitProcess->setEnabled(false);
    int rows = inFileNames.size();
    RobustMatcher rMatcher;
    std::ofstream proData;
    proData.open("inlierNo.dat");
    ui->progressBar->setRange(1, rows);
    ui->progressBar->setVisible(true);
    for(int i = 0;i < rows; ++i)
    {
        int cols = matchMap[i].size();
        std::vector<cv::Mat> singleHList;
        for(int j = 0;j < cols; ++j)
        {
            int refImgIndex = i;
            int cmpImgIndex = matchMap[i][j];
            std::vector<cv::DMatch> inlierMat;
            cv::Mat H = rMatcher.ransacTestFindH(
                        matchPairVec[i][j],
                        keyPointVec[refImgIndex],
                        keyPointVec[cmpImgIndex],
                        inlierMat);
            singleHList.push_back(H);
            proData << i << " to " << cmpImgIndex << " : "
                    << inlierMat.size() << "inner Points"
                    << std::endl;
            proData << "Homography is " << std::endl
                    << H << std::endl << std::endl;
            QStandardItem *tmpitem =
                    new QStandardItem(QString::number(i) + " to "
                                      + QString::number(cmpImgIndex) + " : "
                                      + QString::number(inlierMat.size()) + "innerPoints");
            processInfoModel.appendRow(tmpitem);
            ui->lv_proinfo->scrollToBottom();
            ui->progressBar->setValue(i + 1);
        }
        HomographyVec.push_back(singleHList);
    }
    proData.close();
    isComputeHed = true;
    ui->progressBar->setVisible(false);
    ui->pb_computeH->setEnabled(false);
    ui->pb_autoMosaic->setEnabled(true);
    ui->pb_QuitProcess->setEnabled(true);
    ui->cb_computeH->setChecked(true);
}

void MainWindow::on_pb_computeH_clicked()
{
    if(!isMatched)
    {
        QMessageBox qmBox;
        qmBox.setText("Match Images First Please!");
        qmBox.exec();
        return;
    }

    pfmcomputeH();

}

void MainWindow::pfmAutoMosaic()
{
    std::vector<bool> isComputeFinalH;
    for(int i = 1; i < inFileNames.size(); ++i)
    {
        isComputeFinalH.push_back(false);
    }
    //for each pair from reference image 0 to other image find a shortest path
    //using breadth-fisrt search
    std::ofstream finalHInfoFile;
    finalHInfoFile.open("finalHInfoFile.dat");
    ui->progressBar->setRange(1,inFileNames.size());
    ui->pb_QuitProcess->setVisible(true);
    cv::Mat unitMat(3,3,CV_64F);
    unitMat.at<double>(0,0) = 1.0;
    unitMat.at<double>(0,1) = 0.0;
    unitMat.at<double>(0,2) = 0.0;

    unitMat.at<double>(1,0) = 0.0;
    unitMat.at<double>(1,1) = 1.0;
    unitMat.at<double>(1,2) = 0.0;

    unitMat.at<double>(2,0) = 0.0;
    unitMat.at<double>(2,1) = 0.0;
    unitMat.at<double>(2,2) = 1.0;
    finalH.push_back(unitMat);
    for(int i = 1; i < inFileNames.size(); ++i)
    {
        //save each temperory pair transfer path
        //synchronization with var "srhBuf" simply by their index
        QQueue<QStack<int> > transPath;
        //BFS buffering queue
        QQueue<int> srhBuf;
        //initailize the buf queue with 0
        srhBuf.enqueue(0);
        //current searching central node
        int curNode = 0;
        //saving the transfer path from ref image to current node
        QStack<int> curNodeTranPath;
        //initailize transfer path
        curNodeTranPath.push(0);
        transPath.enqueue(curNodeTranPath);
        curNodeTranPath.clear();
        bool isFound = false;
        //saving BFS node status to protect from the re-search
        //and fake shutdown from infinite cycle path
        std::vector<bool> isSrhed;
        for(int j = 0; j < inFileNames.size(); ++j)
        {
            isSrhed.push_back(false);
        }
        //BFS start from 0 defaultly
        while(!srhBuf.empty())
        {
            curNode = srhBuf.dequeue();
            curNodeTranPath = transPath.dequeue();
            isSrhed[curNode] = true;
            std::vector<int>::iterator iter = matchMap[curNode].begin();
            for(;iter != matchMap[curNode].end(); ++iter)
            {
                //for each node not equal i, enqueue all its unsearched adjoint node
                if(*iter != i)
                {
                    if(isSrhed[*iter] == false)
                    {
                        srhBuf.enqueue(*iter);
                        QStack<int> tmpPath(curNodeTranPath);
                        tmpPath.push(*iter);
                        transPath.enqueue(tmpPath);
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    isFound = true;
                    break;
                }
            }
            if(isFound)
                break;
        }
        //if find the shortest path compute its finalH
        if(isFound)
        {
            cv::Mat tmpFinalH(3,3,CV_64F);
            tmpFinalH.at<double>(0,0) = 1.0;
            tmpFinalH.at<double>(0,1) = 0.0;
            tmpFinalH.at<double>(0,2) = 0.0;

            tmpFinalH.at<double>(1,0) = 0.0;
            tmpFinalH.at<double>(1,1) = 1.0;
            tmpFinalH.at<double>(1,2) = 0.0;

            tmpFinalH.at<double>(2,0) = 0.0;
            tmpFinalH.at<double>(2,1) = 0.0;
            tmpFinalH.at<double>(2,2) = 1.0;
            curNode = i;
            while(curNodeTranPath.size() != 0)
            {
                //for each transfer node find the tmp H
                int refNodeIndex = curNodeTranPath.pop();
                int cmpIndexofRefNode;
                for(int f = 0; f < matchMap[refNodeIndex].size(); ++f)
                {
                    if(matchMap[refNodeIndex][f] == curNode)
                    {
                        cmpIndexofRefNode = f;
                        break;
                    }
                }
                tmpFinalH = HomographyVec[refNodeIndex][cmpIndexofRefNode] * tmpFinalH;
                curNode = refNodeIndex;
            }
            finalHInfoFile << i << " to ref image finalH is:" << std::endl;
            finalHInfoFile << tmpFinalH << std::endl << std::endl;
            finalH.push_back(tmpFinalH);
            QStandardItem *tmpitem = new QStandardItem("ref to " + QString::number(i)
                                                       + " TransH computed...");
            processInfoModel.appendRow(tmpitem);
            ui->lv_proinfo->scrollToBottom();
        }
        else
        {
            cv::Mat tmpZeroH = cv::Mat::zeros(3,3,CV_64F);
            finalH.push_back(tmpZeroH);
        }
        ui->progressBar->setValue(i);
    }
    finalHInfoFile.close();

    //adjust boundray for display
    double minx, miny;
    minx = miny = 0.0;
    double maxx, maxy;
    maxx = inImageSizeVec[0].width();
    maxy = inImageSizeVec[0].height();
    std::vector<QSize>::iterator sizeit = inImageSizeVec.begin();
    for(int n = 0; sizeit != inImageSizeVec.end(); ++sizeit, ++n)
    {
        cv::Mat tmpbd(3,4,CV_64F);
        tmpbd.at<double>(0,0) = 0.0;
        tmpbd.at<double>(1,0) = 0.0;
        tmpbd.at<double>(2,0) = 1.0;

        tmpbd.at<double>(0,1) = (double)(*sizeit).width();
        tmpbd.at<double>(1,1) = 0.0;
        tmpbd.at<double>(2,1) = 1.0;

        tmpbd.at<double>(0,2) = 0.0;
        tmpbd.at<double>(1,2) = (double)(*sizeit).height();
        tmpbd.at<double>(2,2) = 1.0;

        tmpbd.at<double>(0,3) = (double)(*sizeit).width();
        tmpbd.at<double>(1,3) = (double)(*sizeit).height();
        tmpbd.at<double>(2,3) = 1.0;

        tmpbd = finalH[n] * tmpbd;

        for(int h = 0; h < 4; ++h)
        {
            double xx = tmpbd.at<double>(0,h);
            double yy = tmpbd.at<double>(1,h);
            double ww = tmpbd.at<double>(2,h);

            xx /= ww;
            yy /= ww;

            if(xx > maxx)
            {
                maxx = xx;
            }
            if(xx < minx)
            {
                minx = xx;
            }
            if(yy > maxy)
            {
                maxy = yy;
            }
            if(yy < miny)
            {
                miny = yy;
            }
        }
    }
    //qDebug() << finalH.size();
    std::vector<cv::Mat>::iterator hit = finalH.begin();
    for(; hit != finalH.end(); ++hit)
    {
        double w = (*hit).at<double>(2,2);
        double xf, yf;
        if(minx < 0.0)
        {
            xf = w * (-minx);
            (*hit).at<double>(0,2) += xf;
        }
        if(miny < 0.0)
        {
            yf = w * (-miny);
            (*hit).at<double>(1,2) += yf;
        }
    }
    ui->progressBar->setRange(1, finalH.size());
    ui->progressBar->setValue(1);
    std::ofstream of;
    of.open("bdAdjustInfo.dat");
    std::vector<cv::Mat>::iterator it = finalH.begin();
    for(;it != finalH.end(); ++it)
    {
        of << (*it) << std::endl << std::endl;
    }
    of.close();
    rstw = maxx - minx;
    rsth = maxy - miny;
    cv::Mat rImage;
    std::vector<cv::Mat>::iterator fHiter = finalH.begin();
    int id = 0;
    cv::Mat tmpImage = cv::imread(inFileNames[id].toLocal8Bit().constData());
    cv::cvtColor(tmpImage,tmpImage,CV_BGR2RGB);
    cv::warpPerspective(tmpImage,               //source image
                        rImage,                 //output image
                        (*fHiter),                 //transfer Matrix H
                        cv::Size(rstw,rsth)     //output image size
                        );
    ++fHiter;
    ++id;
    ui->progressBar->setValue(id + 1);
    for(; fHiter != finalH.end(); ++fHiter, ++id)
    {
        cv::Mat tmpImage = cv::imread(inFileNames[id].toLocal8Bit().constData());
        cv::cvtColor(tmpImage,tmpImage,CV_BGR2RGB);
        cv::warpPerspective(tmpImage,               //source image
                            rImage,                 //output image
                            (*fHiter),                 //transfer Matrix H
                            cv::Size(rstw,rsth),     //output image size
                            cv::INTER_LINEAR,
                            cv::BORDER_TRANSPARENT
                            );
        ui->progressBar->setValue(id + 1);
    }
    ui->progressBar->setVisible(false);
    isMosaiced = true;
    ui->pb_autoMosaic->setEnabled(false);
    ui->pb_fushion->setEnabled(true);
    ui->pb_QuitProcess->setEnabled(true);
    ui->cb_Mosaic->setChecked(true);
    cv::Mat oImage;
    cv::cvtColor(rImage,oImage, CV_RGB2BGR);
    cv::imwrite("mosaicResult.png",oImage);
}

void MainWindow::on_pb_autoMosaic_clicked()
{
    if(!isComputeHed)
    {
        QMessageBox qmBox;
        qmBox.setText("Compute Each Homography First Please!");
        qmBox.exec();
        return;
    }

    pfmAutoMosaic();

    cv::Mat tmpshowimg = cv::imread("mosaicResult.png");
    cv::cvtColor(tmpshowimg,tmpshowimg,CV_BGR2RGB);
    QImage tmpqimg = QImage((const unsigned char*)(tmpshowimg.data),
                        tmpshowimg.cols,tmpshowimg.rows,QImage::Format_RGB888);
    QPixmap showimg = QPixmap::fromImage(tmpqimg);
    showimg = showimg.scaled(DEFAULT_WIDTH,DEFAULT_HEIGHT,
                                      Qt::KeepAspectRatio);
    ui->lbl_mainDisplay->setPixmap(showimg);
}

void MainWindow::pfmFusion()
{
    ui->pb_QuitProcess->setEnabled(false);
    cv::Mat rImage = cv::Mat::zeros(rsth, rstw, 16);
    std::vector<cv::Mat>::iterator fHiter = finalH.begin();
    std::vector<QSize>::iterator sizeit = inImageSizeVec.begin();
    int id = 0;
    cv::Mat pixelWieghtMap = cv::Mat::zeros(rsth, rstw, CV_64F);
    QVector<QVector<bool> > isRenderedMap(rsth);
    QVector<bool> tmpinivec(rstw);
    tmpinivec.fill(false);
    isRenderedMap.fill(tmpinivec);
    ui->progressBar->setRange(1,finalH.size());
    ui->progressBar->setVisible(true);
    for(; fHiter != finalH.end(); ++fHiter, ++id, ++sizeit)
    {
        cv::Mat tmpImage = cv::imread(inFileNames[id].toLocal8Bit().constData());
        cv::cvtColor(tmpImage,tmpImage,CV_BGR2RGB);
        cv::Mat tmpbd(3,4,CV_64F);
        tmpbd.at<double>(0,0) = 0.0;
        tmpbd.at<double>(1,0) = 0.0;
        tmpbd.at<double>(2,0) = 1.0;

        tmpbd.at<double>(0,1) = (double)(*sizeit).width();
        tmpbd.at<double>(1,1) = 0.0;
        tmpbd.at<double>(2,1) = 1.0;

        tmpbd.at<double>(0,2) = 0.0;
        tmpbd.at<double>(1,2) = (double)(*sizeit).height();
        tmpbd.at<double>(2,2) = 1.0;

        tmpbd.at<double>(0,3) = (double)(*sizeit).width();
        tmpbd.at<double>(1,3) = (double)(*sizeit).height();
        tmpbd.at<double>(2,3) = 1.0;

        tmpbd = finalH[id] * tmpbd;

        double minx, miny;
        minx = rstw;
        miny = rsth;
        double maxx, maxy;
        maxx = 0.0;
        maxy = 0.0;
        for(int h = 0; h < 4; ++h)
        {
            double xx = tmpbd.at<double>(0,h);
            double yy = tmpbd.at<double>(1,h);
            double ww = tmpbd.at<double>(2,h);

            xx /= ww;
            yy /= ww;

            if(xx > maxx)
            {
                maxx = xx;
            }
            if(xx < minx)
            {
                minx = xx;
            }
            if(yy > maxy)
            {
                maxy = yy;
            }
            if(yy < miny)
            {
                miny = yy;
            }
        }
        cv::Mat tmpInvH = fHiter->inv();
        if(minx < 0.0)
            minx = 0.0;
        if(miny < 0.0)
            miny = 0.0;
        if(maxx > rstw)
            maxx = rstw;
        if(maxy > rsth)
            maxy = rsth;
        for(int row = (int)miny; row < (int)maxy; ++row)
        {
            for(int col = (int)minx; col < (int)maxx; ++col)
            {
                cv::Mat tmpPoint(3,1,CV_64F);
                //initialize tmpPoint as a position in result image
                tmpPoint.at<double>(0,0) = (double)col;
                tmpPoint.at<double>(1,0) = (double)row;
                tmpPoint.at<double>(2,0) = 1.0;

                tmpPoint = tmpInvH * tmpPoint;

                //now tmpPoint represents a position in source image
                double tmpx = tmpPoint.at<double>(0,0);
                double tmpy = tmpPoint.at<double>(1,0);
                double tmpw = tmpPoint.at<double>(2,0);

                tmpx /= tmpw;
                tmpy /= tmpw;

                if(tmpx < 0.0 || tmpx > (double)tmpImage.cols)
                    continue;
                if(tmpy < 0.0 || tmpy > (double)tmpImage.rows)
                    continue;

                //compute the tmpPoint pixel value with bilinear interpolation method
                int intpartx = (int)tmpx;
                int intparty = (int)tmpy;
                double decpartx = tmpx - (double)intpartx;
                double decparty = tmpy - (double)intparty;
                double RGB[3];
                for(int chani = 0; chani < 3; ++chani)
                {
                    double tmppv1 = tmpImage.at<cv::Vec3b>(intparty,intpartx)[chani] +
                            decpartx * (tmpImage.at<cv::Vec3b>(intparty,intpartx + 1)[chani]
                                        -tmpImage.at<cv::Vec3b>(intparty,intpartx)[chani]);

                    double tmppv2 = tmpImage.at<cv::Vec3b>(intparty + 1,intpartx)[chani] +
                            decpartx * (tmpImage.at<cv::Vec3b>(intparty + 1,intpartx + 1)[chani]
                                        -tmpImage.at<cv::Vec3b>(intparty + 1,intpartx)[chani]);

                    RGB[chani] = tmppv1 + decparty * (tmppv2 - tmppv1);
                }

                //compute the current pixel fusion weight
                double tmppixw = 1.0 - ((tmpx - (double)tmpImage.cols/2.0)
                        * (tmpx - (double)tmpImage.cols/2.0))
                        / (((double)tmpImage.cols/2.0) * ((double)tmpImage.cols/2.0));
                tmppixw *= 1.0 - ((tmpy - (double)tmpImage.rows/2.0)
                        * (tmpy - (double)tmpImage.rows/2.0))
                        / (((double)tmpImage.rows/2.0) * ((double)tmpImage.rows/2.0));

                //operate the fusion things
                if(!isRenderedMap[row][col])
                {   //if the pixel is null
                    rImage.at<cv::Vec3b>(row, col)[0] = RGB[0];
                    rImage.at<cv::Vec3b>(row, col)[1] = RGB[1];
                    rImage.at<cv::Vec3b>(row, col)[2] = RGB[2];

                    isRenderedMap[row][col] = true;

                    pixelWieghtMap.at<double>(row, col) = tmppixw;
                }
                else
                {
                    double tmppixwpre = pixelWieghtMap.at<double>(row, col);
                    for(int ci = 0; ci < 3; ++ci)
                    {
                        double curPixValue = (tmppixwpre/(tmppixwpre + tmppixw))
                                * rImage.at<cv::Vec3b>(row, col)[ci];
                        curPixValue += (tmppixw/(tmppixwpre + tmppixw))
                                * RGB[ci];

                        rImage.at<cv::Vec3b>(row, col)[ci] = curPixValue;
                    }
                    pixelWieghtMap.at<double>(row, col) += tmppixw;
                }
            }
        }
        ui->progressBar->setValue(id + 1);
        QStandardItem *tmpit = new QStandardItem("Image" + QString::number(id+1)
                                                 + " feathering fusion processed...");
        processInfoModel.appendRow(tmpit);
        ui->lv_proinfo->scrollToBottom();
    }
    //qDebug() << id;
    ui->progressBar->setVisible(false);
    cv::Mat oImage;
    cv::cvtColor(rImage,oImage, CV_RGB2BGR);
    cv::imwrite("FusionResult.png",oImage);
    ui->pb_QuitProcess->setEnabled(true);
    ui->cb_fusion->setChecked(true);
    ui->pb_fushion->setEnabled(false);
}

void MainWindow::on_pb_fushion_clicked()
{
    if(!isMosaiced)
    {
        QMessageBox qmBox;
        qmBox.setText("Mosaic the images first please!");
        qmBox.exec();
        return;
    }
    pfmFusion();
    cv::Mat tmpshowimg = cv::imread("FusionResult.png");
    cv::cvtColor(tmpshowimg,tmpshowimg,CV_BGR2RGB);
    QImage tmpqimg = QImage((const unsigned char*)(tmpshowimg.data),
                        tmpshowimg.cols,tmpshowimg.rows,QImage::Format_RGB888);
    QPixmap showimg = QPixmap::fromImage(tmpqimg);
    showimg = showimg.scaled(DEFAULT_WIDTH,DEFAULT_HEIGHT,
                                      Qt::KeepAspectRatio);
    ui->lbl_mainDisplay->setPixmap(showimg);
}

void MainWindow::on_pb_QuitProcess_clicked()
{
    statusRefresh();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_pb_AllinOne_clicked()
{
    ui->pb_QuitProcess->setEnabled(false);
    pfmFeatherDetect();
    pfmMatch();
    pfmcomputeH();
    pfmAutoMosaic();
    pfmFusion();
    ui->pb_QuitProcess->setEnabled(true);
}
