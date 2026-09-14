#pragma once
#define IMAGELIB_PROCESSING_TRANSFORM_H_
// imagelib/processing/Transform.h
//
// Geometric transforms that move whole pixels: flips, 90-degree rotations,
// transpose and ROI copy. All require dst.valid() and matching channel count.
// Source access uses readNorm(src, col, row); dst access writes dst(x, y) with
// column x and row y.

#include "imagelib/processing/Pixel.h"

namespace iml {
namespace proc {
namespace transform {

enum class Rotation : uint8 { Rotate90 = 0, Rotate180, Rotate270 };

/// dst[x, y] = src[x, H - 1 - y].
void flipVertical(const ConstImageView& src, ImageView dst);

/// Row-parallel flipVertical.
void flipVertical(const ConstImageView& src, ImageView dst,
                  const ExecutionPolicy& p);

/// dst[x, y] = src[W - 1 - x, y].
void flipHorizontal(const ConstImageView& src, ImageView dst);

/// Row-parallel flipHorizontal.
void flipHorizontal(const ConstImageView& src, ImageView dst,
                    const ExecutionPolicy& p);

/// dst[x, y] = src[W - 1 - x, H - 1 - y].
void rotate180(const ConstImageView& src, ImageView dst);

/// Row-parallel rotate180.
void rotate180(const ConstImageView& src, ImageView dst,
               const ExecutionPolicy& p);

/// Rotate 90 degrees clockwise. dst: W = H_src, H = W_src.
void rotate90(const ConstImageView& src, ImageView dst);

/// Rotate 90 degrees counter-clockwise. dst: W = H_src, H = W_src.
void rotate270(const ConstImageView& src, ImageView dst);

/// dst[x, y] = src[y, x]. dst: W = H_src, H = W_src.
void transpose(const ConstImageView& src, ImageView dst);

/// Copy a ROI [x0, x0+w) x [y0, y0+h) of src into dst.
void copyRoi(const ConstImageView& src, ImageView dst,
             uint32 x0, uint32 y0, uint32 w, uint32 h);

/// Row-parallel copyRoi.
void copyRoi(const ConstImageView& src, ImageView dst,
             uint32 x0, uint32 y0, uint32 w, uint32 h,
             const ExecutionPolicy& p);

} // namespace transform
} // namespace proc
} // namespace iml