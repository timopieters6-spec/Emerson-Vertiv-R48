#include "CurrentCalibration.h"
#include <stddef.h>

struct CurrentCalibrationPoint
{
    float measuredAmps;
    float selectedAmps;
};

static const CurrentCalibrationPoint calibration[] = {
    {4.15f, 4.0f},
    {5.03f, 6.0f},
    {5.79f, 7.0f},
    {6.69f, 8.0f},
    {7.6f, 9.0f},
    {8.5f, 10.0f},
    {9.35f, 11.0f},
    {9.6f, 12.0f},
    {11.3f, 13.0f},
    {11.3f, 14.0f},
    {12.18f, 15.0f},
    {13.0f, 16.0f},
    {13.76f, 17.0f},
    {15.4f, 18.0f},
    {16.4f, 19.0f},
    {17.05f, 20.0f},
    {17.8f, 21.0f},
    {18.66f, 22.0f},
    {19.35f, 23.0f},
    {20.1f, 24.0f},
    {20.73f, 25.0f},
    {21.5f, 26.0f},
    {22.2f, 27.0f},
    {21.8f, 28.0f},
    {22.1f, 29.0f},
    {24.0f, 30.0f},
};

bool mapMeasuredAmpsToSelected(float measuredAmps, float &selectedAmps)
{
    const size_t calibrationCount = sizeof(calibration) / sizeof(calibration[0]);
    const float minimumMeasuredAmps = calibration[0].measuredAmps;
    const float maximumMeasuredAmps = calibration[calibrationCount - 1].measuredAmps;

    if (measuredAmps < minimumMeasuredAmps || measuredAmps > maximumMeasuredAmps)
    {
        return false;
    }

    for (size_t i = 0; i + 1 < calibrationCount; ++i)
    {
        const CurrentCalibrationPoint &lower = calibration[i];
        const CurrentCalibrationPoint &upper = calibration[i + 1];

        if (lower.measuredAmps == upper.measuredAmps)
        {
            if (measuredAmps == lower.measuredAmps)
            {
                selectedAmps = lower.selectedAmps;
                return true;
            }
            continue;
        }

        const float fraction =
            (measuredAmps - lower.measuredAmps) /
            (upper.measuredAmps - lower.measuredAmps);
        if (fraction >= 0.0f && fraction <= 1.0f)
        {
            selectedAmps = lower.selectedAmps +
                           fraction * (upper.selectedAmps - lower.selectedAmps);
            return true;
        }
    }

    return false;
}
