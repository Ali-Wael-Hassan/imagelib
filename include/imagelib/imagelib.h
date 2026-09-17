#pragma once
/// @file imagelib/imagelib.h
/// Umbrella header for ImageLib. Pulls in the public surface of the library.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/image/ImageView.h"
#include "imagelib/core/image/Image.h"
#include "imagelib/core/image/Pixel.h"
#include "imagelib/core/ExecutionPolicy.h"

#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"
#include "imagelib/math/Matrix.h"
#include "imagelib/math/Quaternion.h"
#include "imagelib/math/Geometry.h"
#include "imagelib/math/Random.h"
#include "imagelib/math/Interpolation.h"
#include "imagelib/math/Noise.h"
#include "imagelib/math/Gradient.h"

#include "imagelib/simd/Simd.h"
#include "imagelib/threading/Thread.h"
#include "imagelib/threading/ThreadPool.h"
#include "imagelib/threading/Job.h"
#include "imagelib/threading/ParallelFor.h"

#include "imagelib/processing/Primitives.h"
#include "imagelib/processing/Color.h"
#include "imagelib/processing/Convolution.h"

#include "imagelib/procedural/Noise.h"

#include "imagelib/codecs/Codec.h"
#include "imagelib/codecs/StbCodec.h"