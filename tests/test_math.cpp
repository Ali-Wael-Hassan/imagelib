// tests/test_math.cpp – Scalar, Vector, Matrix, Random
#include "tests.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"
#include "imagelib/math/Matrix.h"
#include "imagelib/math/Random.h"
#include <cmath>

using namespace iml;
using namespace iml::math;

bool test_math() {
    // Constants
    TEST_CHECK_NEAR(pi, 3.14159265358979323846, 1e-12);
    TEST_CHECK_NEAR(twoPi, 2.0 * pi, 1e-12);
    TEST_CHECK_NEAR(halfPi, pi / 2.0, 1e-12);
    TEST_CHECK_NEAR(toRadians(180.0), pi, 1e-12);
    TEST_CHECK_NEAR(toDegrees(pi), 180.0, 1e-12);

    // Scalar helpers
    TEST_CHECK_NEAR(clamp(1.7, 0.0, 1.0), 1.0, 1e-12);
    TEST_CHECK_NEAR(clamp(-0.2, 0.0, 1.0), 0.0, 1e-12);
    TEST_CHECK_NEAR(saturate(0.4), 0.4, 1e-12);
    TEST_CHECK(sign(-5) < 0 && sign(0) == 0 && sign(3) > 0);
    TEST_CHECK_NEAR(lerp(0.0, 10.0, 0.5), 5.0, 1e-12);

    // Vec2
    {
        Vec2 v(3, 4);
        TEST_CHECK_NEAR(v.length(), 5, 1e-6);
        TEST_CHECK_NEAR(v.squaredLength(), 25, 1e-6);
        v.normalize();
        TEST_CHECK_NEAR(v.length(), 1, 1e-6);
        Vec2 a(1, 0), b(0, 1);
        TEST_CHECK_NEAR(a.dot(b), 0, 1e-12);
        TEST_CHECK_NEAR(a.dot(a), 1, 1e-12);
        Vec2 c = a + b;
        TEST_CHECK_NEAR(c.x, 1, 1e-12);
        TEST_CHECK_NEAR(c.y, 1, 1e-12);
    }
    // Vec3
    {
        Vec3 v(1, 2, 2);
        TEST_CHECK_NEAR(v.length(), 3, 1e-6);
        Vec3 x(1, 0, 0), y(0, 1, 0);
        Vec3 z = x.cross(y);
        TEST_CHECK_NEAR(z.x, 0, 1e-12);
        TEST_CHECK_NEAR(z.y, 0, 1e-12);
        TEST_CHECK_NEAR(z.z, 1, 1e-12);
        TEST_CHECK_NEAR(x.dot(y), 0, 1e-12);
    }
    // Vec4
    {
        Vec4 v(1, 2, 2, 4);
        TEST_CHECK_NEAR(v.length(), 5, 1e-6);
    }

    // Mat2
    {
        Mat2 id = Mat2::identity();
        TEST_CHECK_NEAR(id(0, 0), 1, 1e-12);
        TEST_CHECK_NEAR(id(1, 0), 0, 1e-12);
        Mat2 r = Mat2::rotation(halfPi);
        Vec2 p = r * Vec2(1, 0);
        TEST_CHECK_NEAR(p.x, 0, 1e-6);
        TEST_CHECK_NEAR(p.y, -1, 1e-6); // library rotates +CCW visually: (1,0) -> (0,-1)
        TEST_CHECK_NEAR(r.determinant(), 1, 1e-6);
        Mat2 inv = r;
        TEST_CHECK(inv.invert());
        Vec2 back = inv * p;
        TEST_CHECK_NEAR(back.x, 1, 1e-5);
        TEST_CHECK_NEAR(back.y, 0, 1e-5);
    }
    // Mat3
    {
        Mat3 r = Mat3::rotationZ(halfPi);
        Vec3 v = r * Vec3(1, 0, 0);
        TEST_CHECK_NEAR(v.x, 0, 1e-6);
        TEST_CHECK_NEAR(v.y, -1, 1e-6);
        Mat3 s = Mat3::scale(Vec3(2, 3, 1));
        Vec3 sc = s * Vec3(1, 1, 0);
        TEST_CHECK_NEAR(sc.x, 2, 1e-12);
        TEST_CHECK_NEAR(sc.y, 3, 1e-12);
    }
    // Mat4
    {
        Mat4 t = Mat4::translation(Vec3(1, 2, 3));
        Vec4 v = t * Vec4(0, 0, 0, 1);
        TEST_CHECK_NEAR(v.x, 1, 1e-12);
        TEST_CHECK_NEAR(v.y, 2, 1e-12);
        TEST_CHECK_NEAR(v.z, 3, 1e-12);
        Mat4 inv = t.inverse();
        Vec4 back = inv * Vec4(1, 2, 3, 1);
        TEST_CHECK_NEAR(back.x, 0, 1e-4);
        TEST_CHECK_NEAR(back.y, 0, 1e-4);
        TEST_CHECK_NEAR(back.z, 0, 1e-4);
    }

    // Random
    {
        Rng rng(12345);
        Rng rng2(12345);
        TEST_CHECK(rng.nextU32() == rng2.nextU32());
        TEST_CHECK((rng.nextInt(10)) <= 10);

        Rng rng3(7);
        bool sawZero = true, sawHigh = true;
        for (int i = 0; i < 1000; ++i) {
            float f = rng3.nextFloat01();
            if (f < 0.001f) sawZero = false;
            if (f > 0.999f) sawHigh = false;
            TEST_CHECK(f >= 0.0f && f < 1.0f);
        }
        TEST_CHECK(sawZero && sawHigh);

        // distribution center of mass roughly 0.5
        double sum = 0;
        for (int i = 0; i < 10000; ++i) sum += rng3.nextFloat01();
        TEST_CHECK_NEAR(sum / 10000.0, 0.5, 0.05);

        // nextInt(lo, hi) stays in range
        for (int i = 0; i < 1000; ++i) {
            int32 v = rng3.nextInt(-5, 5);
            TEST_CHECK(v >= -5 && v <= 5);
        }

        // hashCombine is non-trivial
        uint64 h1 = hashCombine(hashU64(1), hashU64(2));
        uint64 h2 = hashCombine(hashU64(2), hashU64(1));
        TEST_CHECK(h1 != h2);
    }

    return true;
}