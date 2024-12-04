#include "cv.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
cv::Mat SDL_SurfaceToMat(SDL_Surface* surface) {
    if (!surface) {
        throw std::invalid_argument("SDL_Surface pointer is null.");
    }

    // Convert the surface to a known pixel format (e.g., RGBA32)
   // SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  //  if (!convertedSurface) {
//        throw std::runtime_error("Failed to convert SDL_Surface to RGBA32 format.");
//    }

    // Create a cv::Mat with the same size and type
    cv::Mat mat(surface->h, surface->w, CV_8UC4, surface->pixels, surface->pitch);

    // Convert RGBA to BGR
    cv::Mat matBGR;
    cv::cvtColor(mat, matBGR, cv::COLOR_RGBA2BGR);

    // Free the converted surface
   // SDL_DestroySurface(convertedSurface);

    return matBGR;
}

cv::Mat SDL_SurfaceToMat_YUY2(SDL_Surface* surface) {
    if (!surface) {
        throw std::invalid_argument("SDL_Surface pointer is null.");
    }

    // Ensure the surface format is YUY2
    if (surface->format!= SDL_PIXELFORMAT_YUY2) {
        throw std::runtime_error("SDL_Surface is not in YUY2 format.");
    }

    // Create a cv::Mat with the raw data from the SDL_Surface
    // YUY2 format has 2 bytes per pixel
    cv::Mat yuy2Mat(surface->h, surface->w, CV_8UC2, surface->pixels, surface->pitch);

    // Convert YUY2 to BGR format
    cv::Mat bgrMat;
    cv::cvtColor(yuy2Mat, bgrMat, cv::COLOR_YUV2BGR_YUY2);

    return bgrMat;
}



// Function to perform feature matching

extern "C" {bool detectImageInWebcam(SDL_Surface* webcamSurface, SDL_Surface* targetSurface, double matchThreshold = 10.0) {
    // Convert SDL_Surface to cv::Mat
    cv::Mat webcamMat = SDL_SurfaceToMat(webcamSurface);
    cv::Mat targetMat = SDL_SurfaceToMat(targetSurface);
//   cv::imshow("Webcam Image", webcamMat); // uncomment when testing image appearance
//cv::waitKey(0);


    // Initialize ORB detector
cv::Ptr<cv::ORB> orb = cv::ORB::create();
 /*   1500,    // nfeatures
    1.2f,    // scaleFactor
    8,       // nlevels
    31,      // edgeThreshold
    0,       // firstLevel
    2,       // WTA_K
    cv::ORB::HARRIS_SCORE, // scoreType
    31,      // patchSize
    20       // fastThreshold
);*/


    // Detect keypoints and compute descriptors
    std::vector<cv::KeyPoint> keypointsWebcam, keypointsTarget;
    cv::Mat descriptorsWebcam, descriptorsTarget;

    orb->detectAndCompute(webcamMat, cv::noArray(), keypointsWebcam, descriptorsWebcam);
    orb->detectAndCompute(targetMat, cv::noArray(), keypointsTarget, descriptorsTarget);

    if (descriptorsWebcam.empty() || descriptorsTarget.empty()) {
        fprintf(stderr, "No descriptors found. On webcam? %d. On input image? %d.\n", !descriptorsWebcam.empty(), !descriptorsTarget.empty());
        return false;
    }

    // Match descriptors using Brute-Force matcher with Hamming distance
 /*   cv::BFMatcher matcher(cv::NORM_HAMMING, true); // crossCheck=true
    std::vector<cv::DMatch> matches;
    matcher.match(descriptorsTarget, descriptorsWebcam, matches);
*/
// Initialize BFMatcher without crossCheck
cv::BFMatcher matcher(cv::NORM_HAMMING);

// Perform k-NN matching with k=2
std::vector<std::vector<cv::DMatch>> matches;
matcher.knnMatch(descriptorsTarget, descriptorsWebcam, matches, 2);

    if (matches.empty()) {
        return false;
    }
/*
    // Filter good matches based on distance
    double max_dist = 0; double min_dist = 100;

    // Find min and max distances
    for (const auto& match : matches) {
        double dist = match.distance;
        if (dist < min_dist) min_dist = dist;
        if (dist > max_dist) max_dist = dist;
    }

    // Define a threshold to identify good matches
    std::vector<cv::DMatch> goodMatches;
    for (const auto& match : matches) {
        if (match.distance <= std::max(2 * min_dist, 30.0)) {
            goodMatches.push_back(match);
        }
    }
*/
// Apply Lowe's ratio test
const float ratioThresh = 0.75f;
std::vector<cv::DMatch> goodMatches;
for (size_t i = 0; i < matches.size(); i++) {
    if (matches[i].size() < 2)
        continue; // Not enough matches
    const cv::DMatch& bestMatch = matches[i][0];
    const cv::DMatch& betterMatch = matches[i][1];
    
    float ratio = bestMatch.distance / betterMatch.distance;
    if (ratio < ratioThresh) {
        goodMatches.push_back(bestMatch);
    }
}

    // Debug: Print number of good matches
//    std::cout << "Good Matches: " << goodMatches.size() << std::endl;

    // Optionally, visualize matches
    /*
    cv::Mat imgMatches;
    cv::drawMatches(targetMat, keypointsTarget, webcamMat, keypointsWebcam, goodMatches, imgMatches);
    cv::imshow("Good Matches", imgMatches);
    cv::waitKey(30);
    */

    // Determine if enough good matches are found
    return (goodMatches.size() >= matchThreshold);
}
}
