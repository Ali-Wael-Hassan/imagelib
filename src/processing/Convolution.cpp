#include "imagelib/processing/Convolution.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace iml {
namespace proc {
namespace conv {

Kernel::Kernel(int w, int h, std::vector<float> d) : width(w), height(h), data(std::move(d)) {}

bool Kernel::valid() const noexcept {
    return width > 0 && height > 0 && data.size() == static_cast<size_t>(width) * height
        && (width & 1) == 1 && (height & 1) == 1;
}

float Kernel::at(int kx, int ky) const noexcept {
    return data[static_cast<size_t>(ky) * width + kx];
}

int Kernel::radiusX() const noexcept { return width / 2; }
int Kernel::radiusY() const noexcept { return height / 2; }

float sum(const Kernel& k) noexcept {
    float s = 0.f;
    for (float w : k.data) s += w;
    return s;
}

Kernel identityKernel() noexcept { return Kernel(1, 1, {1.f}); }

Kernel boxKernel(int radius) noexcept {
    const int d = 2 * radius + 1;
    return Kernel(d, d, std::vector<float>(static_cast<size_t>(d) * d, 1.f));
}

Kernel gaussianKernel(float sigma, int radius) noexcept {
    if (radius <= 0) radius = std::max(1, static_cast<int>(std::ceil(3.f * sigma)));
    const int d = 2 * radius + 1;
    std::vector<float> w(static_cast<size_t>(d) * d);
    float s = 0.f;
    const float inv2s2 = 1.f / (2.f * sigma * sigma);
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            const float v = std::exp(-static_cast<float>(x * x + y * y) * inv2s2);
            w[(y + radius) * d + (x + radius)] = v;
            s += v;
        }
    }
    for (float& v : w) v /= s;
    return Kernel(d, d, std::move(w));
}

Kernel sobelXKernel() noexcept {
    return Kernel(3, 3, {1.f, 0.f, -1.f,
                          2.f, 0.f, -2.f,
                          1.f, 0.f, -1.f});
}

Kernel sobelYKernel() noexcept {
    return Kernel(3, 3, {1.f,  2.f,  1.f,
                          0.f,  0.f,  0.f,
                         -1.f, -2.f, -1.f});
}

Kernel laplacianKernel() noexcept {
    return Kernel(3, 3, { 0.f, -1.f,  0.f,
                         -1.f,  4.f, -1.f,
                          0.f, -1.f,  0.f});
}

Kernel sharpenKernel(float strength) noexcept {
    return Kernel(3, 3, {0.f, -strength, 0.f,
                         -strength, 1.f + 4.f * strength, -strength,
                         0.f, -strength, 0.f});
}

Kernel embossKernel() noexcept {
    return Kernel(3, 3, {-2.f, -1.f, 0.f,
                         -1.f,  1.f, 1.f,
                          0.f,  1.f, 2.f});
}

int reflectIndex(int i, int n) noexcept {
    if (n <= 1) return 0;
    while (i < 0 || i >= n) {
        if (i < 0) i = -i - 1;
        else       i = 2 * n - i - 1;
    }
    return i;
}

float sampleNorm(const ConstImageView& src, int32 x, int32 y, uint32 c, BorderMode border) {
    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    int32 sx = x, sy = y;
    switch (border) {
        default:
        case BorderMode::Clamp:
            sx = std::min(std::max(x, 0), w - 1);
            sy = std::min(std::max(y, 0), h - 1);
            break;
        case BorderMode::Mirror:
            sx = reflectIndex(x, w);
            sy = reflectIndex(y, h);
            break;
        case BorderMode::Wrap:
            sx = ((x % w) + w) % w;
            sy = ((y % h) + h) % h;
            break;
        case BorderMode::Zero:
            if (x < 0 || x >= w || y < 0 || y >= h) return 0.f;
            break;
    }
    return detail::readNorm(src, sx, sy, c);
}

void convolve(const ConstImageView& src, ImageView dst, const Kernel& k,
              BorderMode border) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::convolve: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("conv::convolve: dimension/channel mismatch");
    if (!k.valid()) throw InvalidParameterError("conv::convolve: invalid kernel");

    const int32 rx = k.radiusX(), ry = k.radiusY();
    const uint32 ch = src.channels();
    const uint32 w = src.width(), h = src.height();
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int32 ky = -ry; ky <= ry; ++ky) {
                    for (int32 kx = -rx; kx <= rx; ++kx) {
                        acc += k.at(kx + rx, ky + ry)
                             * sampleNorm(src, static_cast<int32>(x) + kx,
                                          static_cast<int32>(y) + ky, c, border);
                    }
                }
                detail::writeNorm(dst, x, y, c, acc);
            }
        }
    }
}

void convolve(const ConstImageView& src, ImageView dst, const Kernel& k,
              BorderMode border, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::convolve: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("conv::convolve: dimension/channel mismatch");
    if (!k.valid()) throw InvalidParameterError("conv::convolve: invalid kernel");

    const int32 rx = k.radiusX(), ry = k.radiusY();
    const uint32 ch = src.channels();
    const uint32 w = src.width(), h = src.height();
    if (!wantsParallel(policy, static_cast<size_t>(w) * h * ch, 16384)) {
        convolve(src, dst, k, border);
        return;
    }
    const uint32 width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int32 ky = -ry; ky <= ry; ++ky) {
                    for (int32 kx = -rx; kx <= rx; ++kx) {
                        acc += k.at(kx + rx, ky + ry)
                             * sampleNorm(src, static_cast<int32>(x) + kx,
                                          static_cast<int32>(y) + ky, c, border);
                    }
                }
                detail::writeNorm(dst, x, y, c, acc);
            }
        }
    }, policy);
}

namespace sep {
std::vector<float> gaussianLine(float sigma, int radius) noexcept {
    if (radius <= 0) radius = std::max(1, static_cast<int>(std::ceil(3.f * sigma)));
    std::vector<float> w(static_cast<size_t>(2) * radius + 1);
    float s = 0.f;
    const float inv2s2 = 1.f / (2.f * sigma * sigma);
    for (int i = -radius; i <= radius; ++i) {
        const float v = std::exp(-static_cast<float>(i * i) * inv2s2);
        w[i + radius] = v;
        s += v;
    }
    for (float& v : w) v /= s;
    return w;
}
} // namespace sep

void convolve1D(const ConstImageView& src, ImageView dst,
                const std::vector<float>& kernel, int radius,
                bool horizontal, BorderMode border) {
    if (kernel.empty() || radius < 0)
        throw InvalidParameterError("conv::convolve1D: invalid kernel");
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::convolve1D: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("conv::convolve1D: dimension/channel mismatch");
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int i = -radius; i <= radius; ++i) {
                    const float we = kernel[i + radius];
                    const int32 px = horizontal ? static_cast<int32>(x) + i : static_cast<int32>(x);
                    const int32 py = horizontal ? static_cast<int32>(y) : static_cast<int32>(y) + i;
                    acc += we * sampleNorm(src, px, py, c, border);
                }
                detail::writeNorm(dst, x, y, c, acc);
            }
        }
    }
}

void convolve1D(const ConstImageView& src, ImageView dst,
                const std::vector<float>& kernel, int radius,
                bool horizontal, BorderMode border,
                const ExecutionPolicy& policy) {
    if (kernel.empty() || radius < 0)
        throw InvalidParameterError("conv::convolve1D: invalid kernel");
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::convolve1D: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("conv::convolve1D: dimension/channel mismatch");
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    if (!wantsParallel(policy, static_cast<size_t>(w) * h * ch, 16384)) {
        convolve1D(src, dst, kernel, radius, horizontal, border);
        return;
    }
    const uint32 width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int i = -radius; i <= radius; ++i) {
                    const float we = kernel[i + radius];
                    const int32 px = horizontal ? static_cast<int32>(x) + i : static_cast<int32>(x);
                    const int32 py = horizontal ? static_cast<int32>(y) : static_cast<int32>(y) + i;
                    acc += we * sampleNorm(src, px, py, c, border);
                }
                detail::writeNorm(dst, x, y, c, acc);
            }
        }
    }, policy);
}

void gaussianBlur(const ConstImageView& src, ImageView dst, float sigma,
                  int radius, BorderMode border) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::gaussianBlur: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("conv::gaussianBlur: dimension mismatch");
    if (radius <= 0) radius = std::max(1, static_cast<int>(std::ceil(3.f * sigma)));
    const std::vector<float> line = sep::gaussianLine(sigma, radius);
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    convolve1D(src, tmp.view(), line, radius, true, border);
    convolve1D(tmp.view(), dst, line, radius, false, border);
}

void gaussianBlur(const ConstImageView& src, ImageView dst, float sigma,
                  BorderMode border, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("conv::gaussianBlur: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("conv::gaussianBlur: dimension mismatch");
    int radius = std::max(1, static_cast<int>(std::ceil(3.f * sigma)));
    const std::vector<float> line = sep::gaussianLine(sigma, radius);
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    convolve1D(src, tmp.view(), line, radius, true, border, policy);
    convolve1D(tmp.view(), dst, line, radius, false, border, policy);
}

void boxBlur(const ConstImageView& src, ImageView dst, int radius,
             BorderMode border) {
    if (radius < 0) throw InvalidParameterError("conv::boxBlur: negative radius");
    if (radius == 0) {
        detail::mapPixels(src, dst, [](const Pixel& p) { return p; });
        return;
    }
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    const float we = 1.f / static_cast<float>(2 * radius + 1);
    std::vector<float> k(static_cast<size_t>(2) * radius + 1, we);
    convolve1D(src, tmp.view(), k, radius, true, border);
    convolve1D(tmp.view(), dst, k, radius, false, border);
}

void boxBlur(const ConstImageView& src, ImageView dst, int radius,
             BorderMode border, const ExecutionPolicy& policy) {
    if (radius < 0) throw InvalidParameterError("conv::boxBlur: negative radius");
    if (radius == 0) {
        detail::mapPixels(src, dst, [](const Pixel& p) { return p; }, policy);
        return;
    }
    const uint32 w = src.width(), h = src.height();
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32), w, h, src.colorSpace());
    const float we = 1.f / static_cast<float>(2 * radius + 1);
    std::vector<float> k(static_cast<size_t>(2) * radius + 1, we);
    convolve1D(src, tmp.view(), k, radius, true, border, policy);
    convolve1D(tmp.view(), dst, k, radius, false, border, policy);
}

} // namespace conv
} // namespace proc
} // namespace iml