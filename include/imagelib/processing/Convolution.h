#pragma once
#define IMAGELIB_PROCESSING_CONVOLUTION_H_
// imagelib/processing/Convolution.h
//
// Spatial convolution and separable filters. Kernels use correlation
// orientation (no kernel flip), which is the convention in image processing.
// All filters operate on normalized samples so they work for any storage type;
// separable filters accumulate in a float work image for precision.

#include "imagelib/processing/Pixel.h"
#include "imagelib/core/Image.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace iml {
namespace proc {
namespace conv {

/// How out-of-bounds samples are produced during a filter.
enum class BorderMode : uint8 { Clamp = 0, Mirror, Wrap, Zero };

/// A small 2D convolution kernel (odd dimensions; center = weight origin).
struct Kernel {
    int              width  = 0;
    int              height = 0;
    std::vector<float> data;

    Kernel() noexcept = default;
    Kernel(int w, int h, std::vector<float> d);

    bool valid() const noexcept;
    float at(int kx, int ky) const noexcept;
    int radiusX() const noexcept;
    int radiusY() const noexcept;
};

float sum(const Kernel& k) noexcept;

// ---------------------------------------------------------------------------
// Kernel factories.
// ---------------------------------------------------------------------------

Kernel identityKernel() noexcept;

Kernel boxKernel(int radius) noexcept;

Kernel gaussianKernel(float sigma, int radius = 0) noexcept;

Kernel sobelXKernel() noexcept;

Kernel sobelYKernel() noexcept;

/// Discrete Laplacian (4-neighbour).
Kernel laplacianKernel() noexcept;

/// Unsharp-style sharpening kernel: identity + strength * (4-neighbour laplacian).
Kernel sharpenKernel(float strength = 1.f) noexcept;

Kernel embossKernel() noexcept;

// ---------------------------------------------------------------------------
// Border sampling.
// ---------------------------------------------------------------------------

int reflectIndex(int i, int n) noexcept;

/// Sample normalized value with border handling (storage-order channels).
float sampleNorm(const ConstImageView& src, int32 x, int32 y, uint32 c, BorderMode border);

// ---------------------------------------------------------------------------
// 2D convolution.
// ---------------------------------------------------------------------------

void convolve(const ConstImageView& src, ImageView dst, const Kernel& k,
              BorderMode border = BorderMode::Clamp);

/// Row-parallel generic convolution (dynamic policy; serial for tiny images).
void convolve(const ConstImageView& src, ImageView dst, const Kernel& k,
              BorderMode border, const ExecutionPolicy& policy);

// ---------------------------------------------------------------------------
// Separable filters.
// ---------------------------------------------------------------------------

namespace sep {
std::vector<float> gaussianLine(float sigma, int radius) noexcept;
} // namespace sep

/// Apply a symmetric 1D kernel along one axis with 2D border handling.
void convolve1D(const ConstImageView& src, ImageView dst,
                const std::vector<float>& kernel, int radius,
                bool horizontal, BorderMode border);

/// Row-parallel separable 1D pass.
void convolve1D(const ConstImageView& src, ImageView dst,
                const std::vector<float>& kernel, int radius,
                bool horizontal, BorderMode border,
                const ExecutionPolicy& policy);

/// Separable Gaussian blur. Accumulates in a float work image.
void gaussianBlur(const ConstImageView& src, ImageView dst, float sigma,
                  int radius = 0, BorderMode border = BorderMode::Clamp);

/// Separable Gaussian blur, row-parallel (both passes share `policy`).
void gaussianBlur(const ConstImageView& src, ImageView dst, float sigma,
                  BorderMode border, const ExecutionPolicy& policy);

/// Separable box blur (radius passes summed once: weights already normalized).
void boxBlur(const ConstImageView& src, ImageView dst, int radius,
             BorderMode border = BorderMode::Clamp);

/// Separable box blur, row-parallel.
void boxBlur(const ConstImageView& src, ImageView dst, int radius,
             BorderMode border, const ExecutionPolicy& policy);

} // namespace conv
} // namespace proc
} // namespace iml