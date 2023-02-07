#ifndef KERNEL_H
#define KERNEL_H
#include <cmath>
#include "vec.h"

namespace Kernel {

struct RBF {
    static float f(const Vec& x1, const Vec& x2)
    {
        float sigma = 1;
        float xL2 = Vec::dot(x1, x1) + Vec::dot(x2, x2) - 2*Vec::dot(x1, x2);
        xL2 = xL2/(-2*sigma*sigma);
        return exp(xL2);
    }
};
struct Laplace {
    static float f(const Vec& x1, const Vec& x2)
    {
        float sigma = 1;
        float xL2 = Vec::dot(x1, x1) + Vec::dot(x2, x2) - 2*Vec::dot(x1, x2);
        xL2 = -sqrt(xL2)/sigma;
        return exp(xL2);
    }
};

struct Sigmoid {
    static float f(const Vec& x1, const Vec& x2)
    {
        float beta1 = 1;
        float theta = -1;
        return tanh(beta1 * Vec::dot(x1, x2) + theta);
    }
};

struct Polynomial {
    static float f(const Vec& x1, const Vec& x2)
    {
        float d = 1.0;
        float p = 100;
        return pow(Vec::dot(x1, x2) + d, p);
    }
};

struct Linear {
    static float f(const Vec& x1, const Vec& x2)
    {
        return Vec::dot(x1, x2);
    }
};

}

#endif // KERNEL_H
