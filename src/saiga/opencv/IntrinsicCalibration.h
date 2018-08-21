#pragma once

#include "CameraData.h"
#include "CalibrationPattern.h"

namespace Saiga
{
class SAIGA_GLOBAL IntrinsicsCalibration
{
public:

    IntrinsicsCalibration(CalibrationPattern& pattern)
        : pattern(pattern)
    {

    }

    void addImage(cv::Mat image)
    {
        currentIntrinsics.w = image.cols;
        currentIntrinsics.h = image.rows;

        auto points = pattern.detect(image);
        if(points.size() > 0)
        {
            images.push_back(points);
        }else{
            cout << "could not find pattern :(" << endl;
        }
        recomputeIntrinsics();
    }

    void recomputeIntrinsics()
    {
        Intrinsics intr = currentIntrinsics;
        auto objPointss = pattern.duplicate(images.size());

        cv::Mat rvecs,tvecs;
        auto error = calibrateCamera(
                    objPointss,
                    images,
                    cv::Size(intr.w,intr.h),
                    intr.K,
                    intr.dist,
                    rvecs,
                    tvecs);
        cout << "calibrateCamera error: " << error << endl;

        currentIntrinsics = intr;
    }

    Intrinsics currentIntrinsics;
protected:
    CalibrationPattern& pattern;
    std::vector<std::vector<CalibrationPattern::ImagePointType>> images;
};

}
