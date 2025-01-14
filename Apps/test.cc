// This software is for internal use in the EndoMapper project.
// Not to be re-distributed.

#include <opencv2/core/core.hpp>
#include <System.h>

int main(int argc, char **argv)
{
    std::cout << argc << std::endl;
    if ((argc != 4))
    {
        cerr << endl
             << "Usage: ./DefSLAM ORBvocabulary calibrationFile video" << endl
             << " or    ./DefSLAM ORBvocabulary calibrationFile  " << endl
             << endl;
        return 1;
    }

    cv::VideoCapture cap;
    if (argc == 4)
        cap.open(argv[3]);
    else
        cap.open(argv[3]);

    if (!cap.isOpened()) // check if we succeeded
        return -1;

    string arg = argv[2];
    string arg2 = argv[1];

    // Create SLAM system. It initializes all system threads and gets ready to
    // process frames.
    defSLAM::System SLAM(argv[1], argv[2], true);

    uint i(-1);
    //cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    //cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    uint initial_point(0);
    while (cap.isOpened())
    {
        cv::Mat imLeft;
        std::cout << "Loading : " << (double(i) / initial_point) * 100 << "% " << '\r' << std::flush;
        cap >> imLeft;
        i++;
        if (i < initial_point)
            continue;
        if (imLeft.empty())
        {
            cerr << endl
                 << "Failed to load image at: " << to_string(i) << endl;
            return 1;
        }

        SLAM.TrackMonocular(imLeft, i);
    }

    SLAM.Shutdown();

    return 0;
}
