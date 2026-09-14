#pragma once
#include "PackageTrust.h"
#include <string>

namespace zero {

struct PackageTrustPresentation {
    std::wstring shortLabel;
    std::wstring title;
    std::wstring body;
    bool blocked{false};
    bool verified{false};
};

PackageTrustPresentation PresentPackageTrust(const PackageTrustResult& trust);

} // namespace zero
