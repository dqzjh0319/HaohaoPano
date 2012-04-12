#include "robustmatcher.h"

RobustMatcher::~RobustMatcher()
{

}
// Set the feature detector
void RobustMatcher::setFeatureDetector(cv::Ptr<cv::FeatureDetector>& detect)
{
    detector= detect;
}
// Set the descriptor extractor
void RobustMatcher::setDescriptorExtractor(cv::Ptr<cv::DescriptorExtractor>& desc)
{
    extractor= desc;
}

void RobustMatcher::featherDectect(cv::Mat& image,
                                   std::vector<cv::KeyPoint>& thisKeyPoints,
                                   cv::Mat& thisDescriptor)
{
    cv::Mat grayImage;
    cv::cvtColor(image,grayImage,CV_RGB2GRAY);
    detector->detect(grayImage,thisKeyPoints);
    extractor->compute(image,thisKeyPoints,thisDescriptor);
}

void RobustMatcher::match(//input keypoint descriptor
                             cv::Mat& descriptors1,
                             cv::Mat& descriptors2,
                             // output matches
                             std::vector<cv::DMatch>& symMatches)
{
    // Construction of the matcher
    cv::BruteForceMatcher<cv::L2<float> > matcher;
    // from image 1 to image 2
    // based on k nearest neighbours (with k=2)
    std::vector<std::vector<cv::DMatch> > matches1;
    matcher.knnMatch(descriptors1,descriptors2,
                    matches1, // vector of matches (up to 2 per entry)
                    2);
                    // return 2 nearest neighbours
                    // from image 2 to image 1
                    // based on k nearest neighbours (with k=2)
    std::vector<std::vector<cv::DMatch> > matches2;
    matcher.knnMatch(descriptors2,descriptors1,
                    matches2, // vector of matches (up to 2 per entry)
                    2);
                    // return 2 nearest neighbours
    // 3. Remove matches for which NN ratio is
    // > than threshold
    // clean image 1 -> image 2 matches
    int removed = ratioTest(matches1);
    // clean image 2 -> image 1 matches
    removed = ratioTest(matches2);
    // 4. Remove non-symmetrical matches
    symmetryTest(matches1,matches2,symMatches);
    // 5. Validate matches using RANSAC

    //cv::Mat Homography = ransacTestFindH(symMatches,
    //                keypoints1, keypoints2, matches);
    /*cv::Mat fundemental= ransacTest(symMatches,
                    keypoints1, keypoints2, matches);*/
    // return the found fundemental matrix
    //return Homography;
}

int RobustMatcher::ratioTest(std::vector<std::vector<cv::DMatch> >& matches)
{
    int removed=0;
    // for all matches
    for (std::vector<std::vector<cv::DMatch> >::iterator
            matchIterator= matches.begin();
            matchIterator!= matches.end(); ++matchIterator)
    {
        // if 2 NN has been identified
        if (matchIterator->size() > 1) {
        // check distance ratio
        if ((*matchIterator)[0].distance/
            (*matchIterator)[1].distance > ratio) {
                matchIterator->clear(); // remove match
                removed++;
            }
        } else { // does not have 2 neighbours
        matchIterator->clear(); // remove match
        removed++;
        }
    }
    return removed;
}

void RobustMatcher::symmetryTest(
        const std::vector<std::vector<cv::DMatch> >& matches1,
        const std::vector<std::vector<cv::DMatch> >& matches2,
        std::vector<cv::DMatch>& symMatches)
{
// for all matches image 1 -> image 2
    for (std::vector<std::vector<cv::DMatch> >::
            const_iterator matchIterator1= matches1.begin();
            matchIterator1!= matches1.end(); ++matchIterator1)
    {
        // ignore deleted matches
        if (matchIterator1->size() < 2)
            continue;
        // for all matches image 2 -> image 1
        for (std::vector<std::vector<cv::DMatch> >::
                const_iterator matchIterator2= matches2.begin();
                matchIterator2!= matches2.end();
                ++matchIterator2)
        {
            // ignore deleted matches
            if (matchIterator2->size() < 2)
                continue;
            // Match symmetry test
            if ((*matchIterator1)[0].queryIdx ==
                (*matchIterator2)[0].trainIdx &&
                (*matchIterator2)[0].queryIdx ==
                (*matchIterator1)[0].trainIdx)
            {
                // add symmetrical match
                symMatches.push_back(
                    cv::DMatch((*matchIterator1)[0].queryIdx,
                    (*matchIterator1)[0].trainIdx,
                    (*matchIterator1)[0].distance));
                break; // next match in image 1 -> image 2
            }
        }
    }
}

/*cv::Mat RobustMatcher::ransacTestFindF(
            const std::vector<cv::DMatch>& matches,
            const std::vector<cv::KeyPoint>& keypoints1,
            const std::vector<cv::KeyPoint>& keypoints2,
            std::vector<cv::DMatch>& outMatches)
{
    // Convert keypoints into Point2f
    std::vector<cv::Point2f> points1, points2;
    for (std::vector<cv::DMatch>::
            const_iterator it= matches.begin();
            it!= matches.end(); ++it)
    {
        // Get the position of left keypoints
        float x= keypoints1[it->queryIdx].pt.x;
        float y= keypoints1[it->queryIdx].pt.y;
        points1.push_back(cv::Point2f(x,y));
        // Get the position of right keypoints
        x= keypoints2[it->trainIdx].pt.x;
        y= keypoints2[it->trainIdx].pt.y;
        points2.push_back(cv::Point2f(x,y));
    }
    // Compute F matrix using RANSAC
    std::vector<uchar> inliers(points1.size(),0);
    cv::Mat fundemental= cv::findFundamentalMat(
                            cv::Mat(points1),cv::Mat(points2), // matching points
                            inliers,
                            // match status (inlier or outlier)
                            cv::FM_RANSAC, // RANSAC method
                            distance,
                            // distance to epipolar line
                            confidence); // confidence probability
    // extract the surviving (inliers) matches
    std::vector<uchar>::const_iterator
        itIn= inliers.begin();
    std::vector<cv::DMatch>::const_iterator
        itM= matches.begin();
    // for all matches
    for ( ;itIn!= inliers.end(); ++itIn, ++itM) {
        if (*itIn)
        { // it is a valid match
            outMatches.push_back(*itM);
        }
    }
    if (refineF)
    {
        // The F matrix will be recomputed with
        // all accepted matches
        // Convert keypoints into Point2f
        // for final F computation
        points1.clear();
        points2.clear();
        for (std::vector<cv::DMatch>::
                const_iterator it= outMatches.begin();
                it!= outMatches.end(); ++it)
        {
            // Get the position of left keypoints

            float x= keypoints1[it->queryIdx].pt.x;
            float y= keypoints1[it->queryIdx].pt.y;
            points1.push_back(cv::Point2f(x,y));
            // Get the position of right keypoints
            x= keypoints2[it->trainIdx].pt.x;
            y= keypoints2[it->trainIdx].pt.y;
            points2.push_back(cv::Point2f(x,y));
        }
        // Compute 8-point F from all accepted matches
        fundemental= cv::findFundamentalMat(
                        cv::Mat(points1),
                        cv::Mat(points2), // matches
                        cv::FM_8POINT); // 8-point method
    }
    return fundemental;
}*/

cv::Mat RobustMatcher::ransacTestFindH(
            const std::vector<cv::DMatch>& matches,
            const std::vector<cv::KeyPoint>& keypoints1,
            const std::vector<cv::KeyPoint>& keypoints2,
            std::vector<cv::DMatch>& outMatches)
{
    // Convert keypoints into Point2f
    std::vector<cv::Point2f> points1, points2;
    for (std::vector<cv::DMatch>::
            const_iterator it= matches.begin();
            it!= matches.end(); ++it)
    {
        // Get the position of left keypoints
        float x= keypoints1[it->queryIdx].pt.x;
        float y= keypoints1[it->queryIdx].pt.y;
        points1.push_back(cv::Point2f(x,y));
        // Get the position of right keypoints
        x= keypoints2[it->trainIdx].pt.x;
        y= keypoints2[it->trainIdx].pt.y;
        points2.push_back(cv::Point2f(x,y));
    }
    // Compute H matrix using RANSAC
    std::vector<uchar> inliers(points1.size(),0);
    cv::Mat homography= cv::findHomography(
                            cv::Mat(points2), // points
                            cv::Mat(points1), // corresponding
                            inliers,
                            // outputted inliers matches
                            cv::RANSAC,
                            // RANSAC method
                            1.);
    // max distance to reprojection point

    // extract the surviving (inliers) matches
    std::vector<uchar>::const_iterator
        itIn = inliers.begin();
    std::vector<cv::DMatch>::const_iterator
        itM = matches.begin();
    // for all matches
    for ( ;itIn!= inliers.end(); ++itIn, ++itM) {
        if (*itIn)
        { // it is a valid match
            outMatches.push_back(*itM);
        }
    }

    /*// Draw the inlier points
    std::vector<cv::Point2f>::const_iterator itPts=
        points1.begin();
    std::vector<uchar>::const_iterator itIn= inliers.begin();
    while (itPts!=points1.end()) {
        // draw a circle at each inlier location
        if (*itIn)
        cv::circle(image1,*itPts,3,
            cv::Scalar(255,255,255),2);
        ++itPts;
        ++itIn;
    }
    itPts= points2.begin();
    itIn= inliers.begin();
    while (itPts!=points2.end()) {
        // draw a circle at each inlier location
        if (*itIn)
        cv::circle(image2,*itPts,3,
            cv::Scalar(255,255,255),2);
        ++itPts;
        ++itIn;
    }*/

    if (refineH)
    {
        // The F matrix will be recomputed with
        // all accepted matches
        // Convert keypoints into Point2f
        // for final F computation
        points1.clear();
        points2.clear();
        for (std::vector<cv::DMatch>::
                const_iterator it= outMatches.begin();
                it!= outMatches.end(); ++it)
        {
            // Get the position of left keypoints

            float x= keypoints1[it->queryIdx].pt.x;
            float y= keypoints1[it->queryIdx].pt.y;
            points1.push_back(cv::Point2f(x,y));
            // Get the position of right keypoints
            x= keypoints2[it->trainIdx].pt.x;
            y= keypoints2[it->trainIdx].pt.y;
            points2.push_back(cv::Point2f(x,y));
        }
        // Compute 8-point F from all accepted matches
        homography= cv::findHomography(
                        cv::Mat(points2),
                        cv::Mat(points1), // matches
                        cv::RANSAC);
    }

    return homography;
}
