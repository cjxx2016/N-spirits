#ifndef LINEARREGRESSION_H
#define LINEARREGRESSION_H
#include "vec.h"

class LinearModel
{
public:
    Vec w;
    float b;
public:
    LinearModel(){}
    static float sigmoid(float x)
    {
        return 1/(1 + exp(-x));
    }
    static float dSigmoid(float y)
    {
        return y*(1 - y);
    }
    float operator()(const Vec &x)
    {
        return sigmoid(Vec::dot(w, x) + b);
    }
    void update(const Vec &x, float y, float yt, float learningRate)
    {
        /* sgd */
        for (std::size_t i = 0; i < w.size(); i++) {
            w[i] -= learningRate*(y - yt)*dSigmoid(y)*x[i];
        }
        b -= learningRate*(y - yt)*dSigmoid(y);
        return;
    }
    void train(const std::vector<Vec> &x, const Vec &yt, std::size_t maxEpoch, std::size_t batchSize=30, float learningRate=1e-3)
    {
        for (std::size_t i = 0; i < maxEpoch; i++) {
            for (std::size_t j = 0; j < batchSize; j++) {
                int k = rand() % x.size();
                float y = sigmoid(Vec::dot(w, x[k]) + b);
                update(x[k], y, yt[k], learningRate);
            }
        }
        return;
    }
};

#endif // LINEARREGRESSION_H
