#include "PresentationProfile.h"

namespace viz {

PresentationProfile PresentationProfile::cinematic_default() {
    return PresentationProfile{};
}

PresentationProfile PresentationProfile::interactive_default() {
    return PresentationProfile::cinematic_default();
}

} // namespace viz
