// examples/custom/custom_darken_lighten_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #7: Darken/Lighten)
// =============================================================================
//
// What you learn here:
//   * Brightness is an arithmetic point operator: out = clamp01(in + amount).
//     A NEGATIVE amount darkens, a POSITIVE amount lightens. One kernel, two
//     effects — controlled by a parameter, not two copies of the code.
//   * This is the OOP lesson in miniature: one function, parameterized inputs,
//     and no hidden state. Call it with amount=-0.3 and it darkens; call it
//     with +0.3 and it lightens. The same code is reused everywhere.
//   * The clamp matters: normalized pixels live in [0,1]. Writing 1.3 into a
//     UInt8 image would overflow, so we clamp before writeNorm.
//
// How it is OPTIMIZED:
//   * Row-parallel with parallelForRows + wantsParallel.
//   * Vectorizable: `v + amount` maps to a single vector FADD; the clamp is a
//     couple of VMINPS/VMAXPS instructions once auto-vectorized.
//
// Usage: custom_darken_lighten_demo <input> <output> [serial] [amount|-0.2]

#include "../example_util.h"

using namespace iml;

namespace {

inline float clamp01(float v) noexcept { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

inline void customBrightness(const ConstImageView& src, ImageView dst, float amount,
                             const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customBrightness: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() ||
        src.channels() != dst.channels())
        throw InvalidParameterError("customBrightness: view mismatch");

    const uint32 w = src.width(), h = src.height(), ch = src.channels();

    auto processRow = [&](uint32 y) {
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                proc::detail::writeNorm(dst, x, y, c,
                    clamp01(proc::detail::readNorm(src, x, y, c) + amount));
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_darken_lighten_demo <input> <output> [serial] [amount]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const float amount = argc >= 5 ? static_cast<float>(std::atof(argv[4])) : -0.2f;
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM darken/lighten (kernel)", src);
    std::printf("  backend: %s | amount: %+.2f (%s)\n",
                iml_example::policyName(serial), amount,
                amount < 0.f ? "darken" : "lighten");

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customBrightness", pixels, [&]() {
        customBrightness(src.view(), out.view(), amount, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}