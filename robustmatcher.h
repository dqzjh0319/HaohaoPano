#ifndef ROBUSTMATCHER_H
#define ROBUSTMATCHER_H

#include "mainwindow.h"

class RobustMatcher
{

private:
    // pointer to the feature point detector object
    cv::Ptr<cv::FeatureDetector> detector;
    // pointer to the feature descriptor extractor object
    cv::Ptr<cv::DescriptorExtractor> extractor;
    float ratio; // max ratio between 1st and 2nd NN
    bool refineF; // if true will refine the F matrix
    bool refineH;
    double distance; // min distance to epipolar
    double confidence; // confidence level (probability)
public:
    RobustMatcher():ratio(0.65f), refineF(true), refineH(false),
    confidence(0.99), distance(3.0) {
    // SURF is the default feature
        detector = new cv::SiftFeatureDetector();
        extractor = new cv::SiftDescriptorExtractor();
    }

    ~RobustMatcher();

    void setFeatureDetector(cv::Ptr<cv::FeatureDetector>& detect);

    void setDescriptorExtractor(cv::Ptr<cv::DescriptorExtractor>& desc);

    void featherDectect(cv::Mat& image,
                        std::vector<cv::KeyPoint>& thisKeyPoints,
                        cv::Mat& thisDescriptor);

    void match(cv::Mat& descriptors1,
               cv::Mat& descriptors2,
               std::vector<cv::DMatch>& symMatches);

    //void computeH()
    cv::Mat ransacTestFindH(
                const std::vector<cv::DMatch>& matches,
                const std::vector<cv::KeyPoint>& keypoints1,
                const std::vector<cv::KeyPoint>& keypoints2,
                std::vector<cv::DMatch>& outMatches);

private:
    int ratioTest(std::vector<std::vector<cv::DMatch> >& matches);
    void symmetryTest(const std::vector<std::vector<cv::DMatch> >& matches1,
                    const std::vector<std::vector<cv::DMatch> >& matches2,
                    std::vector<cv::DMatch>& symMatches);

    /*cv::Mat ransacTestFindF(
                const std::vector<cv::DMatch>& matches,
                const std::vector<cv::KeyPoint>& keypoints1,
                const std::vector<cv::KeyPoint>& keypoints2,
                std::vector<cv::DMatch>& outMatches);*/



};

#endif // ROBUSTMATCHER_H
