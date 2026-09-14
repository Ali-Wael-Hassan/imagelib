#include "imagelib/processing/Morphology.h"

#include <algorithm>
#include <utility>

namespace iml {
namespace proc {
namespace morph {

bool StructuringElement::empty() const noexcept { return offsets.empty(); }
size_t StructuringElement::size() const noexcept { return offsets.size(); }

StructuringElement square(int radius) {
    if (radius < 0) throw InvalidParameterError("morph::square: negative radius");
    StructuringElement e;
    for (int32 dy = -radius; dy <= radius; ++dy)
        for (int32 dx = -radius; dx <= radius; ++dx)
            e.offsets.emplace_back(dx, dy);
    return e;
}

StructuringElement cross(int radius) {
    if (radius < 0) throw InvalidParameterError("morph::cross: negative radius");
    StructuringElement e;
    e.offsets.emplace_back(0, 0);
    for (int32 i = 1; i <= radius; ++i) {
        e.offsets.emplace_back(i, 0);
        e.offsets.emplace_back(-i, 0);
        e.offsets.emplace_back(0, i);
        e.offsets.emplace_back(0, -i);
    }
    return e;
}

void checkElement(const StructuringElement& e) {
    if (e.empty()) throw InvalidParameterError("morph: empty structuring element");
}

void erode(const ConstImageView& src, ImageView dst, const StructuringElement& e,
           conv::BorderMode border) {
    checkElement(e);
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("morph::erode: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c) {
                float m = 1.f;
                for (const auto& d : e.offsets) {
                    m = std::min(m, conv::sampleNorm(src, static_cast<int32>(x) + d.first,
                                                     static_cast<int32>(y) + d.second, c, border));
                }
                detail::writeNorm(dst, x, y, c, m);
            }
}

void dilate(const ConstImageView& src, ImageView dst, const StructuringElement& e,
            conv::BorderMode border) {
    checkElement(e);
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("morph::dilate: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c) {
                float m = 0.f;
                for (const auto& d : e.offsets) {
                    m = std::max(m, conv::sampleNorm(src, static_cast<int32>(x) + d.first,
                                                     static_cast<int32>(y) + d.second, c, border));
                }
                detail::writeNorm(dst, x, y, c, m);
            }
}

void open(const ConstImageView& src, ImageView dst, const StructuringElement& e,
          conv::BorderMode border) {
    checkElement(e);
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    erode(src, tmp.view(), e, border);
    dilate(tmp.view(), dst, e, border);
}

void close(const ConstImageView& src, ImageView dst, const StructuringElement& e,
           conv::BorderMode border) {
    checkElement(e);
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    dilate(src, tmp.view(), e, border);
    erode(tmp.view(), dst, e, border);
}

} // namespace morph
} // namespace proc
} // namespace iml