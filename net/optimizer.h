#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include <vector>
#include "../basic/tensor.hpp"
#include "optimize.h"
#include "layerdef.h"

template<typename Net, typename OptimizeMethod>
class Optimizer
{
public:
    using Layers = typename Net::Layers;
    using Grads = typename Net::Grads;
    using Optimizers = typename Net::template OptimzeBlocks<OptimizeMethod>;
    float learningRate;
    Net &net;
    Grads grads;
    Optimizers optimizers;
public:
    /* generate grad */
    template<typename Layers, typename Grads, typename Optimizers, std::size_t N>
    struct Generate {
        static void impl(Layers& layers, Grads &grads, Optimizers &optimizers)
        {
            Generate<Layers, Grads, Optimizers, N - 1>::impl(layers, grads, optimizers);
            using Layer = std::tuple_element_t<N - 1, Layers>;
            using GradType = typename Layer::Grad;
            Layer &layer = std::get<N - 1>(layers);

            /* grad */
            GradType &grad = std::get<N - 1>(grads);

            using ParamType = typename Layer::ParamType;

            grad = GradType(static_cast<ParamType>(layer));
            /* optimizer */
            using OptimizeBlock = typename Layer::template OptimizeBlock<OptimizeMethod>;
            OptimizeBlock& opt = std::get<N - 1>(optimizers);
            opt = OptimizeBlock(layer);
            return;
        }
    };

    template<typename Layers, typename Grads, typename Optimizers>
    struct Generate<Layers, Grads, Optimizers, 1> {
        static void impl(Layers& layers, Grads &grads, Optimizers &optimizers)
        {
            auto &layer = std::get<0>(layers);
            auto &grad = std::get<0>(grads);
            /* grad */
            using Layer = std::tuple_element_t<0, Layers>;
            using ParamType = typename Layer::ParamType;
            using GradType = typename Layer::Grad;
            grad = GradType(static_cast<ParamType>(layer));
            /* optimizer */
            using OptimizeBlock = typename Layer::template OptimizeBlock<OptimizeMethod>;
            auto& opt = std::get<0>(optimizers);
            opt = OptimizeBlock(layer);
            return;
        }
    };
    /* backward */
    template<typename Layers, typename Grads, std::size_t N, typename LayerN1>
    struct Backward {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layerN1 = std::get<N - 1>(layers);
            auto& gradN1 = std::get<N - 1>(grads);
            auto& gradN2 = std::get<N - 2>(grads);
            /* backward */
            gradN1.backward(layerN1, gradN2.delta);
            /* evaluate */
            Tensor &x1 = std::get<N - 2>(layers).o;
            gradN1.eval(x1, layerN1.o);
            /* next */
            using LayerN2 = std::tuple_element_t<N - 2, Layers>;
            Backward<Layers, Grads, N - 1, LayerN2>::impl(grads, layers, x);
            return;
        }
    };

    template<typename Layers, typename Grads, typename LayerN1>
    struct Backward<Layers, Grads, 1, LayerN1> {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layer0 = std::get<0>(layers);
            auto& grad0 = std::get<0>(grads);
            /* no backward */
            /* evaluate */
            grad0.eval(x, layer0.o);
            return;
        }
    };

    template<typename Layers, typename Grads>
    struct Backward<Layers, Grads, 1, LSTM> {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layer1 = std::get<1>(layers);
            auto& grad1 = std::get<1>(grads);
            auto& grad0 = std::get<0>(grads);
            /* backward through time */
            grad1.backwardAtTime(layer1, grad1.delta, x);
            return;
        }
    };

    template<typename Layers, typename Grads>
    struct Backward<Layers, Grads, 1, BatchNorm1d> {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layer1 = std::get<1>(layers);
            auto& grad1 = std::get<1>(grads);
            auto& batchNormGrad = std::get<0>(grads);
            Tensor &delta = batchNormGrad.deltas[0];
            /* backward delta to batchnorm */
            grad1.backward(layer1, delta);
            /* evaluate */
            batchNormGrad.eval(x, layer1.o);
            return;
        }
    };
    template<typename Layers, typename Grads, std::size_t N>
    struct Backward<Layers, Grads, N, MaxPooling2d> {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layerN1 = std::get<N - 1>(layers);
            auto& gradN1 = std::get<N - 1>(grads);
            auto& gradN2 = std::get<N - 2>(grads);
            /* backward */
            gradN1.backward(layerN1, gradN2.delta);
            /* no gradient */
            /* next */
            using LayerN2 = std::tuple_element_t<N - 2, Layers>;
            Backward<Layers, Grads, N - 1, LayerN2>::impl(grads, layers, x);
            return;
        }
    };

    template<typename Layers, typename Grads, std::size_t N>
    struct Backward<Layers, Grads, N, AvgPooling2d> {
        static void impl(Grads &grads, Layers& layers, const Tensor &x)
        {
            auto& layerN1 = std::get<N - 1>(layers);
            auto& gradN1 = std::get<N - 1>(grads);
            auto& gradN2 = std::get<N - 2>(grads);
            /* backward */
            gradN1.backward(layerN1, gradN2.delta);
            /* no gradient */
            using LayerN2 = std::tuple_element_t<N - 2, Layers>;
            Backward<Layers, Grads, N - 1, LayerN2>::impl(grads, layers, x);
            return;
        }
    };

    /* update */
    template<typename Layers, typename Grads, typename Optimizers, std::size_t N>
    struct Update {
        static void impl(Optimizers &optimizers, Layers& layers, Grads &grads, float learningRate)
        {
            Update<Layers, Grads, Optimizers, N - 1>::impl(optimizers, layers, grads, learningRate);
            auto &layer = std::get<N - 1>(layers);
            auto &grad = std::get<N - 1>(grads);
            auto &opt = std::get<N - 1>(optimizers);
            opt(layer, grad, learningRate);
            return;
        }
    };

    template<typename Layers, typename Grads, typename Optimizers>
    struct Update<Layers, Grads, Optimizers, 1> {
        static void impl(Optimizers &optimizers, Layers& layers, Grads &grads, float learningRate)
        {
            auto &layer = std::get<0>(layers);
            auto &grad = std::get<0>(grads);
            auto &opt = std::get<0>(optimizers);
            opt(layer, grad, learningRate);
            return;
        }
    };
public:
    explicit Optimizer(Net &net_, float learningRate_)
        :learningRate(learningRate_),net(net_)
    {
        Generate<Layers, Grads, Optimizers, Net::N>::impl(net_.layers, grads, optimizers);
    }

    void backward(const Tensor &loss, const Tensor &x)
    {
        auto &grad = std::get<Net::N - 1>(grads);
        grad.delta = loss;
        using LayerN1 = std::tuple_element_t<Net::N - 1, Layers>;
        Backward<Layers, Grads, Net::N, LayerN1>::impl(grads, net.layers, x);
        return;
    }

    void backward(const Tensor &loss, const Tensor &x, const Tensor &yt)
    {
        auto &grad = std::get<Net::N - 1>(grads);
        grad.delta = loss;
        auto &layer = std::get<Net::N - 1>(net.layers);
        Tensor &x_ = std::get<Net::N - 2>(net.layers).o;
        grad.eval(x_, layer.o, yt);
        using LayerN2 = std::tuple_element_t<Net::N - 2, Layers>;
        Backward<Layers, Grads, Net::N - 1, LayerN2>::impl(grads, net.layers,  x);
        return;
    }

    void backward(const std::vector<Tensor> &loss, const std::vector<Tensor> &x)
    {
        for (int t = x.size() - 1; t >= 0; t--) {
            backward(loss[t], x[t]);
        }
        return;
    }

    void update()
    {
        /* update */
        Update<Layers, Grads, Optimizers, Net::N>::impl(optimizers, net.layers, grads, learningRate);
        return;
    }
};


#endif // OPTIMIZER_H
