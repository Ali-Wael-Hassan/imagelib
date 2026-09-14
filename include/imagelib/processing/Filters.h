#pragma once
#define IMAGELIB_PROCESSING_FILTERS_H_
// imagelib/processing/Filters.h
//
// Point (per-pixel) filters and a single-pixel-neighbourhood median filter.
// Operate on any storage type through normalized sample I/O.

#include "imagelib/processing/Pixel.h"
#include "imagelib/processing/Color.h"
#include "imagelib/processing/Convolution.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace iml {
namespace proc {
namespace filter {

void checkShape(const ConstImageView& src, ImageView dst);

// ---------------------------------------------------------------------------
// Basic point transforms.
// ---------------------------------------------------------------------------

/// out = 1 - in.
void invert(const ConstImageView& src, ImageView dst);

/// out = in + delta (delta in [-1,1]).
void brightness(const ConstImageView& src, ImageView dst, float delta);

/// out = (in - 0.5) * factor + 0.5.
void contrast(const ConstImageView& src, ImageView dst, float factor);

/// out = in * scale + offset.
void linear(const ConstImageView& src, ImageView dst, float scale, float offset);

/// Gamma correction: out = in^(1/gamma).
void gammaCorrect(const ConstImageView& src, ImageView dst, float gamma);

/// Binary threshold on channel 0: out = (in >= t) ? 1 : 0 across RGB.
void threshold(const ConstImageView& src, ImageView dst, float t,
               bool above = true);

/// Per-channel sRGB -> linear then linear -> sRGB round trip.
void toLinear(const ConstImageView& src, ImageView dst);

void toSrgb(const ConstImageView& src, ImageView dst);

/// Desaturate: map every pixel to its luminance.
void desaturate(const ConstImageView& src, ImageView dst);

// ---------------------------------------------------------------------------
// Median (square window, radius >= 1).
// ---------------------------------------------------------------------------

void median(const ConstImageView& src, ImageView dst, int radius,
            conv::BorderMode border = conv::BorderMode::Mirror);

/// Row-parallel invert.
void invert(const ConstImageView& src, ImageView dst, const ExecutionPolicy& p);

/// Row-parallel brightness.
void brightness(const ConstImageView& src, ImageView dst, float delta,
                const ExecutionPolicy& p);

/// Row-parallel contrast.
void contrast(const ConstImageView& src, ImageView dst, float factor,
              const ExecutionPolicy& p);

/// Row-parallel threshold.
void threshold(const ConstImageView& src, ImageView dst, float t,
               const ExecutionPolicy& p);

/// Row-parallel desaturate.
void desaturate(const ConstImageView& src, ImageView dst,
                const ExecutionPolicy& p);

/// Row-parallel linear (scale/offset).
void linear(const ConstImageView& src, ImageView dst, float scale,
            float offset, const ExecutionPolicy& p);

} // namespace filter
} // namespace proc
} // namespace iml