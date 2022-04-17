#pragma once

#include <d2vins/d2vins_types.hpp>
#include "d2frontend/d2featuretracker.h"

namespace D2VINS {
class D2LandmarkManager : public D2Frontend::LandmarkManager {
    std::vector<int> frame_ids;
    virtual void addKeyframe(const D2Frontend::VisualImageDescArray & images) {
        for (auto & image : images.images) {
            for (auto & lm : image.landmarks) {
                updateLandmark(lm);
            }
        }
    }
};

}