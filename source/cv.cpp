#include "cv.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <SDL3/SDL.h>
cv::Mat SDL_SurfaceToMat(SDL_Surface* surface) {
    if (!surface) {
        throw std::invalid_argument("SDL_Surface pointer is null.");
    }

    // Convert the surface to a known pixel format (e.g., RGBA32)
    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!convertedSurface) {
        throw std::runtime_error("Failed to convert SDL_Surface to RGBA32 format.");
    }

    // Create a cv::Mat with the same size and type
    cv::Mat mat(convertedSurface->h, convertedSurface->w, CV_8UC4, convertedSurface->pixels, convertedSurface->pitch);

    // Convert RGBA to BGR
    cv::Mat matBGR;
    cv::cvtColor(mat, matBGR, cv::COLOR_RGBA2BGR);

    // Free the converted surface
    SDL_DestroySurface(convertedSurface);

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

extern "C" {
    // Modified function to return a pointer to Rectangle instead of a bool
    SDL_FRect* detectImageInWebcam(SDL_Surface* webcamSurface, SDL_Surface* targetSurface, double matchThreshold = 10.0) {
        // Convert SDL_Surface to cv::Mat
        cv::Mat webcamMat = SDL_SurfaceToMat(webcamSurface);
        cv::Mat targetMat = SDL_SurfaceToMat(targetSurface);
    
        // Initialize ORB detector
        cv::Ptr<cv::ORB> orb = cv::ORB::create();
    
        // Detect keypoints and compute descriptors
        std::vector<cv::KeyPoint> keypointsWebcam, keypointsTarget;
        cv::Mat descriptorsWebcam, descriptorsTarget;
    
        orb->detectAndCompute(webcamMat, cv::noArray(), keypointsWebcam, descriptorsWebcam);
        orb->detectAndCompute(targetMat, cv::noArray(), keypointsTarget, descriptorsTarget);
    
        // Check if descriptors are found
        if (descriptorsWebcam.empty() || descriptorsTarget.empty()) {
            fprintf(stderr, "No descriptors found. On webcam? %d. On input image? %d.\n", 
                    !descriptorsWebcam.empty(), !descriptorsTarget.empty());
            return NULL;
        }
    
        // Initialize BFMatcher without crossCheck
        cv::BFMatcher matcher(cv::NORM_HAMMING);
    
        // Perform k-NN matching with k=2
        std::vector<std::vector<cv::DMatch>> matches;
        matcher.knnMatch(descriptorsTarget, descriptorsWebcam, matches, 2);
    
        // Check if any matches are found
        if (matches.empty()) {
            return NULL;
        }
    
        // Apply Lowe's ratio test to filter good matches
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
    
        // Determine if enough good matches are found
        if (static_cast<double>(goodMatches.size()) >= matchThreshold) {
            // Collect the locations of the matched keypoints in the webcam image
            std::vector<cv::Point> pointsWebcam;
            pointsWebcam.reserve(goodMatches.size());
            for (const auto& match : goodMatches) {
                pointsWebcam.emplace_back(keypointsWebcam[match.trainIdx].pt);
            }
    
            // Compute the bounding rectangle that encompasses all matched points
            cv::Rect boundingRect = cv::boundingRect(pointsWebcam);
    
            // Allocate memory for the Rectangle struct
            SDL_FRect* rect = (SDL_FRect*)malloc(sizeof(*rect));
            if (!rect) {
                // Allocation failed
                fprintf(stderr, "Memory allocation for Rectangle failed.\n");
                return NULL;
            }
    
            // Populate the Rectangle struct with bounding rectangle data
            
            rect->x = boundingRect.x;
            rect->y = boundingRect.y;
            rect->w = boundingRect.width;
            rect->h = boundingRect.height;
    
            return rect;
        } else {
            // Not enough matches; return NULL
            return NULL;
        }
    }
}
