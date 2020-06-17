#include "glog/logging.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/core/core.hpp>

#include "ceres/ceres.h"

#include <System.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

void LoadImages(const string &strPathLeft, const string &strPathRight,
                const string &strPathTimes, vector<string> &vstrImageLeft,
                vector<string> &vstrImageRight, vector<double> &vTimeStamps);

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        cerr << endl
             << "Usage: ./stereo_Morlana_groundtruth path_to_vocabulary "
                "path_to_settings path_to_left_folder path_to_right_folder "
                "path_to_times_file"
             << endl;
        return 1;
    }

    google::InitGoogleLogging("Hello");
    // double n = 6;
    // Eigen::setNbThreads(n);
    // Eigen::initParallel();
    // Retrieve paths to images
    vector<string> vstrImageLeft;
    vector<string> vstrImageRight;
    vector<double> vTimeStamp;

    std::cout << "start load images  " << std::endl;
    LoadImages(string(argv[3]), string(argv[4]), string(argv[5]), vstrImageLeft,
               vstrImageRight, vTimeStamp);
    std::cout << "end load images  " << std::endl;

    if (vstrImageLeft.empty() || vstrImageRight.empty())
    {
        cerr << "ERROR: No images in provided path." << endl;
        return 1;
    }

    if (vstrImageLeft.size() != vstrImageRight.size())
    {
        cerr << "ERROR: Different number of left and right images." << endl;
        return 1;
    }

    // Read rectification parameters
    cv::FileStorage fsSettings(argv[2], cv::FileStorage::READ);
    if (!fsSettings.isOpened())
    {
        cerr << "ERROR: Wrong path to settings" << endl;
        return -1;
    }

    cv::Mat K_l, K_r, P_l, P_r, R_l, R_r, D_l, D_r;
    fsSettings["LEFT.K"] >> K_l;
    fsSettings["RIGHT.K"] >> K_r;

    fsSettings["LEFT.P"] >> P_l;
    fsSettings["RIGHT.P"] >> P_r;

    fsSettings["LEFT.R"] >> R_l;
    fsSettings["RIGHT.R"] >> R_r;

    fsSettings["LEFT.D"] >> D_l;
    fsSettings["RIGHT.D"] >> D_r;

    int rows_l = fsSettings["LEFT.height"];
    int cols_l = fsSettings["LEFT.width"];
    int rows_r = fsSettings["RIGHT.height"];
    int cols_r = fsSettings["RIGHT.width"];

    if (K_l.empty() || K_r.empty() || P_l.empty() || P_r.empty() || R_l.empty() ||
        R_r.empty() || D_l.empty() || D_r.empty() || rows_l == 0 || rows_r == 0 ||
        cols_l == 0 || cols_r == 0)
    {
        cerr << "ERROR: Calibration parameters to rectify stereo are missing!"
             << endl;
        return -1;
    }

    cv::Mat M1l, M2l, M1r, M2r;
    cv::initUndistortRectifyMap(K_l, D_l, R_l, P_l.rowRange(0, 3).colRange(0, 3),
                                cv::Size(cols_l, rows_l), CV_32F, M1l, M2l);
    cv::initUndistortRectifyMap(K_r, D_r, R_r, P_r.rowRange(0, 3).colRange(0, 3),
                                cv::Size(cols_r, rows_r), CV_32F, M1r, M2r);

    const size_t nImages = vstrImageLeft.size();

    // Create SLAM system. It initializes all system threads and gets ready to
    // process frames.
    ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::MONOCULAR, true);

    // Vector for tracking time statistics
    vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    cout << endl
         << "-------" << endl;
    cout << "Start processing sequence ..." << endl;
    cout << "Images in the sequence: " << nImages << endl
         << endl;

    sleep(3);
    // Main loop
    size_t start = 200;
    cv::Mat imLeft, imRight, imLeftRect, imRightRect;
    for (size_t ni = start; ni < nImages; ni++)
    {
        std::cout << vstrImageLeft[ni] << " i:  " << ni << std::endl;
        // Read left and right images from file
        imLeft = cv::imread(vstrImageLeft[ni], cv::IMREAD_UNCHANGED);
        imRight = cv::imread(vstrImageRight[ni], cv::IMREAD_UNCHANGED);

        if (imLeft.empty())
        {
            cerr << endl
                 << "Failed to load image at: " << string(vstrImageLeft[ni]) << endl;
            return 1;
        }

        if (imRight.empty())
        {
            cerr << endl
                 << "Failed to load image at: " << string(vstrImageRight[ni]) << endl;
            return 1;
        }

        std::chrono::steady_clock::time_point trect1 =
            std::chrono::steady_clock::now();

        cv::remap(imLeft, imLeftRect, M1l, M2l, cv::INTER_LINEAR);
        cv::remap(imRight, imRightRect, M1r, M2r, cv::INTER_LINEAR);

        std::chrono::steady_clock::time_point trect2 =
            std::chrono::steady_clock::now();
        // ORB_SLAM2::vTimeStereoRect.push_back(std::chrono::duration_cast<std::chrono::duration<float>
        // >(trect2 - trect1).count());

        double tframe = vTimeStamp[ni];

        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        if (imLeftRect.channels() == 1)
        {
            cv::cvtColor(imLeftRect, imLeftRect, cv::COLOR_GRAY2RGB);
            cv::cvtColor(imRightRect, imRightRect, cv::COLOR_GRAY2RGB);
        }
        cv::Mat _mask(imLeftRect.rows, imLeftRect.cols, CV_8UC1, cv::Scalar(255));

        SLAM.TrackMonocularGT(imLeftRect, imRightRect, ni, _mask);

        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        double ttrack =
            std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1)
                .count();

        // ORB_SLAM2::vTimeTotalTrack.push_back(ttrack);

        vTimesTrack[ni] = ttrack;

        // Wait to load the next frame
        double T = 0;
        if (ni < nImages - 1)
            T = vTimeStamp[ni + 1] - tframe;
        else if (ni > 0)
            T = tframe - vTimeStamp[ni - 1];

        //  if(ttrack<T)
        //      usleep((T-ttrack)*1e6);
    }

    // Stop all threads
    SLAM.Shutdown();

    // Tracking time statistics
    sort(vTimesTrack.begin(), vTimesTrack.end());
    float totaltime = 0;
    for (int ni = 0; ni < nImages; ni++)
    {
        totaltime += vTimesTrack[ni];
    }
    cout << "-------" << endl
         << endl;
    cout << "median tracking time: " << vTimesTrack[nImages / 2] << endl;
    cout << "mean tracking time: " << totaltime / nImages << endl;

    // Save camera trajectory
    SLAM.SaveTrajectoryTUM("CameraTrajectory.txt");

    return 0;
}

void LoadImages(const string &strPathLeft, const string &strPathRight,
                const string &strPathTimes, vector<string> &vstrImageLeft,
                vector<string> &vstrImageRight, vector<double> &vTimeStamps)
{
    ifstream fTimes;

    fTimes.open(strPathTimes);
    vTimeStamps.reserve(5000);
    vstrImageLeft.reserve(5000);
    vstrImageRight.reserve(5000);
    std::cout << strPathTimes << std::endl;
    while (!fTimes.eof())
    {
        string s;
        getline(fTimes, s);
        if (!s.empty())
        {
            stringstream ss;
            ss << s;
            string name = ss.str();
            for (uint i(0); i < 6; i++)
                name.pop_back();
            vstrImageLeft.push_back(strPathLeft + "/stereo_im_l_" + name + ".png");
            vstrImageRight.push_back(strPathRight + "/stereo_im_r_" + name + ".png");

            //  std::cout << name<< std::endl;

            double t;
            ss >> t;
            vTimeStamps.push_back(t / 1e6);
        }
    }
}