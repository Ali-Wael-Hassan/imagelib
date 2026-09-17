// io_speed_demo.cpp - measures disk load and save throughput.
// Decode (load) and encode (PNG / BMP) are timed with the bench helpers and
// reported as best ms plus MB/s. The default PNG level 0 uses the codec's
// parallel stored-block writer; levels 1-9 use STB's compressed writer.
// Run (optional: iteration count, PNG zlib level 0-9; default is fastest level 0):
//   io_speed_demo.exe assets/mario.jpg 5 0
#include "example_util.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

using namespace iml;
using namespace iml_example;

namespace {

/// Size of a file in MiB (1024^2 bytes), or 0 when missing.
double fileSizeMiB(const std::string& path) {
    std::error_code ec;
    const uint64 bytes = static_cast<uint64>(std::filesystem::file_size(path, ec));
    return ec ? 0.0 : static_cast<double>(bytes) / (1024.0 * 1024.0);
}

/// Prints one measured row: label, best ms and MB/s over `bytes` (decimal MB).
void reportRow(const char* label, uint64 bytes, double ms) {
    const double mbps = static_cast<double>(bytes) / (ms / 1000.0) / (1000.0 * 1000.0);
    std::printf("  %-14s %10.2f ms  %10.1f MB/s\n", label, ms, mbps);
}

} // namespace

int main(int argc, char** argv) {
    const std::string input = pickInput(argc > 1 ? argv[1] : "");
    const int iterations = argc > 2 ? std::atoi(argv[2]) : 5;
    const int pngLevel = std::clamp(argc > 3 ? std::atoi(argv[3]) : 0, 0, 9);

    try {
        if (input.empty()) {
            std::cerr << "Error: no input image\n";
            return 1;
        }

        const Image probe(input); // validates the file and pulls it into the page cache
        const uint32 w = probe.width(), h = probe.height();
        const uint64 pixels = static_cast<uint64>(w) * h;
        const uint64 rawBytes = probe.bufferBytes();
        const double rawMiB = static_cast<double>(rawBytes) / (1024.0 * 1024.0);

        std::printf("image        %u x %u   %u megapixels   %.1f MiB raw   (%s)\n",
                    w, h, static_cast<unsigned>(pixels / 1000000u), rawMiB, input.c_str());
        std::printf("%-14s %12s %12s\n", "operation", "best ms", "MB/s");

        const double loadMs = bench::bestMs(iterations, 3, [&] { Image img(input); });
        reportRow("load", rawBytes, loadMs);

        SaveOptions pngOpts;
        pngOpts.pngCompressionLevel = static_cast<uint16>(pngLevel);
        const std::string pngPath = "out_io_speed.png";
        const double savePngMs = bench::bestMs(iterations, 2, [&] { probe.save(pngPath, pngOpts); });
        const double pngBytes = fileSizeMiB(pngPath) * 1024.0 * 1024.0;
        reportRow("save png", static_cast<uint64>(pngBytes), savePngMs);
        std::printf("           (zlib level %d -> %.2f MiB file)\n", pngLevel, fileSizeMiB(pngPath));

        const std::string bmpPath = "out_io_speed.bmp";
        const double saveBmpMs = bench::bestMs(iterations, 2, [&] { probe.save(bmpPath); });
        const double bmpBytes = fileSizeMiB(bmpPath) * 1024.0 * 1024.0;
        reportRow("save bmp", static_cast<uint64>(bmpBytes), saveBmpMs);
        std::printf("           (%.2f MiB file)\n", fileSizeMiB(bmpPath));

        std::printf(">> load %.1f Mpix/s | png %.1f MB/s | bmp %.1f MB/s (best of %d)\n",
                    static_cast<double>(pixels) / 1e6 / (loadMs / 1000.0),
                    pngBytes / (savePngMs / 1000.0) / 1e6,
                    bmpBytes / (saveBmpMs / 1000.0) / 1e6,
                    iterations);
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}