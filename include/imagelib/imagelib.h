#pragma once
// imagelib/imagelib.h
//
// Umbrella header for ImageLib. Pulls in the public surface of the library.
//
// ImageLib is a low-level image processing / graphics foundation:
//
//   Memory + Data layout + Math + SIMD + Threading + Compression
//   + Image processing + Procedural generation + Codecs
//
// The core execution hierarchy is: Scalar -> SIMD -> Multithreaded
// -> SIMD + Multithreaded. Algorithms select the fastest available path
// automatically (see iml::ExecutionPolicy).

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/ImageView.h"
#include "imagelib/core/Image.h"
#include "imagelib/core/ExecutionPolicy.h"

#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"
#include "imagelib/math/Matrix.h"
#include "imagelib/math/Quaternion.h"
#include "imagelib/math/Geometry.h"
#include "imagelib/math/Random.h"
#include "imagelib/math/Interpolation.h"
#include "imagelib/math/Noise.h"

#include "imagelib/simd/Simd.h"
#include "imagelib/threading/Thread.h"
#include "imagelib/threading/ThreadPool.h"
#include "imagelib/threading/Job.h"
#include "imagelib/threading/ParallelFor.h"

#include "imagelib/compression/BitPacking.h"
#include "imagelib/compression/PackedPixels.h"
#include "imagelib/compression/Quantization.h"
#include "imagelib/compression/PackedImage.h"

#include "imagelib/processing/Pixel.h"
#include "imagelib/processing/Color.h"
#include "imagelib/processing/Convolution.h"
#include "imagelib/processing/Filters.h"
#include "imagelib/processing/Resize.h"
#include "imagelib/processing/Transform.h"
#include "imagelib/processing/Morphology.h"
#include "imagelib/processing/Analysis.h"

#include "imagelib/procedural/Noise.h"
#include "imagelib/procedural/Masks.h"
#include "imagelib/procedural/Heightmaps.h"
#include "imagelib/procedural/Textures.h"

#include "imagelib/codecs/Codec.h"
#include "imagelib/codecs/StbCodec.h"