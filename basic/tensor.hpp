#ifndef TENSOR_H
#define TENSOR_H
#include <vector>
#include <functional>
#include <iostream>
#include <assert.h>
#include "alignallocator.hpp"
#include "simd.hpp"

template<typename T, template<typename Ti> class Alloc=std::allocator>
class Tensor_
{
public:
    using ValueType = T;
    using Vector = std::vector<T, Alloc<T> >;
    using Shape = std::vector<int>;
    using Size = std::vector<int>;
    using iterator = typename Vector::iterator;
    using const_iterator = typename Vector::const_iterator;
    class Sub
    {
    public:
        Tensor_ *pointer;
        std::size_t pos;
    public:
        Sub():pointer(nullptr),pos(0){}
        Sub(const Sub &r):pointer(r.pointer),pos(r.pos){}
        Sub& operator=(const Sub &r)
        {
            if (this == &r) {
                return *this;
            }
            pointer = r.pointer;
            pos = r.pos;
            return *this;
        }
        inline void operator=(const Tensor_ &x)
        {
            for (std::size_t i = 0; i < x.totalSize; i++) {
                pointer->val[i + pos] = x.val[i];
            }
            return;
        }
        inline void operator=(const std::vector<T> &x)
        {
            for (std::size_t i = 0; i < x.size(); i++) {
                pointer->val[i + pos] = x[i];
            }
            return;
        }
    };

public:
    std::size_t totalSize;
    Vector val;
    Size sizes;
    Shape shape;
private:
    Sub subset;
public:
    /* default construct */
    Tensor_():totalSize(0){}

    static void initParams(const Shape &shape, Size &sizes, std::size_t &totalsize)
    {
        totalsize = 1;
        for (std::size_t i = 0; i < shape.size(); i++) {
            totalsize *= shape[i];
        }
        sizes = std::vector<int>(shape.size(), 1);
        for (std::size_t i = 0; i < shape.size() - 1; i++) {
            for (std::size_t j = i + 1; j < shape.size(); j++) {
                 sizes[i] *= shape[j];
            }
        }
        return;
    }
    /* contruct with shape */
    explicit Tensor_(const Shape &shape_):totalSize(1),shape(shape_)
    {
        initParams(shape, sizes, totalSize);
        val = std::vector<T, Alloc<T>>(totalSize, T(0));
    }

    explicit Tensor_(const Shape &shape_, const std::vector<T, Alloc<T>> &val_):
        totalSize(1),shape(shape_),val(val_)
    {
        initParams(shape, sizes, totalSize);
    }


    explicit Tensor_(const std::initializer_list<int> &shape_, const std::initializer_list<T> &val_):
        totalSize(1),shape(shape_),val(val_)
    {
        initParams(shape, sizes, totalSize);
    }

    explicit Tensor_(const std::vector<Tensor_> &x)
    {
        totalSize = x.size()*x[0].totalsize;
        sizes.push_back(x.size()*x[0].sizes[0]);
        sizes.push_back(x[0].sizes);
        shape.push_back(x.size());
        shape.push_back(x[0].shape);
        for (std::size_t i = 0; i < x.size(); i++) {
            val.push_back(x[i].val);
        }
    }

    /* construct with shape */
    template<typename ...Dim>
    explicit Tensor_(Dim ...dim):totalSize(1),shape({int(dim)...})
    {
        initParams(shape, sizes, totalSize);
        val = std::vector<T, Alloc<T> >(totalSize, T(0));
    }

    /* copy constructor */
    Tensor_(const Tensor_ &r)
        :totalSize(r.totalSize),shape(r.shape),sizes(r.sizes),val(r.val){}

    /* move construct */
    Tensor_(Tensor_ &&r):totalSize(r.totalSize)
    {
        totalSize = r.totalSize;
        shape.swap(r.shape);
        sizes.swap(r.sizes);
        val.swap(r.val);
        r.totalSize = 0;
    }

    inline operator T* () noexcept
    {
        return val.data();
    }

    inline operator Vector ()
    {
        return val;
    }

    inline Tensor_& operator - ()
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] = -val[i];
        }
        return *this;
    }

    inline T* ptr() noexcept { return val.data(); }
    inline const T* ptr() const noexcept { return val.data(); }
    inline bool empty() const {return totalSize == 0;}
    /* iterator */
    inline iterator begin() noexcept { return val.begin();}
    inline const_iterator begin() const noexcept { return val.begin();}
    inline iterator end() noexcept { return val.end();}
    inline const_iterator end() const noexcept { return val.end();}
    /* size */
    inline std::size_t size() const {return totalSize;}

    template<typename ...Index>
    inline std::size_t size(Index ...index) const
    {
        std::size_t N = sizeof ...(Index) - 1;
        return sizes[N];
    }

    inline std::size_t size(const Shape &indexes) const
    {
        std::size_t N = indexes.size() - 1;
        return sizes[N];
    }

    void zero(){val.assign(totalSize, 0);}
    void fill(T value){val.assign(totalSize, value);}
    inline T &operator[](std::size_t i) {return val[i];}
    inline T operator[](std::size_t i) const {return val[i];}

    /* assign operator */
    inline Tensor_& operator=(const Tensor_ &r)
    {
        if (this == &r) {
            return *this;
        }
        totalSize = r.totalSize;
        shape = r.shape;
        sizes = r.sizes;
        val = r.val;
        return *this;
    }

    inline Tensor_& operator=(const std::vector<T> &x)
    {
        val.assign(x.begin(), x.end());
        return *this;
    }
    inline Tensor_& operator=(T x)
    {
        val.assign(totalSize, x);
        return *this;
    }

    /* move */
    Tensor_ &operator=(Tensor_ &&r)
    {
        if (this == &r) {
            return *this;
        }
        totalSize = r.totalSize;
        shape.swap(r.shape);
        sizes.swap(r.sizes);
        val.swap(r.val);
        r.totalSize = 0;
        return *this;
    }
    /* init */
    static Tensor_ zeros(Shape &shape)
    {
        Tensor_ x(shape);
        return x;
    }

    template<typename ...Dim>
    static Tensor_ zeros(Dim ...dim)
    {
        Tensor_ x(dim...);
        return x;
    }

    static Tensor_ ones(Shape &shape)
    {
        Tensor_ x(shape);
        x.fill(1);
        return x;
    }

    template<typename ...Dim>
    static Tensor_ ones(Dim ...dim)
    {
        Tensor_ x(dim...);
        x.fill(1);
        return x;
    }
    /* subset */
    template<typename ...Index>
    Tensor_ sub(Index ...index) const
    {
        std::size_t N = sizeof ...(Index);
        std::vector<int> subIndex(shape.begin() + N, shape.end());
        Tensor_ y(subIndex);
        std::size_t pos = posOf(index...);
        for (std::size_t i = 0; i < y.totalSize; i++) {
            y.val[i] = val[i + pos];
        }
        return y;
    }

    template<typename ...Index>
    void slice(Tensor_ &y, Index ...index) const
    {
        std::size_t pos = posOf(index...);
        for (std::size_t i = 0; i < y.totalSize; i++) {
            y.val[i] = val[i + pos];
        }
        return;
    }

    template<typename ...Index>
    inline Sub& at(Index ...index)
    {
        subset.pointer = this;
        subset.pos = posOf(index...);
        return subset;
    }

    /* visit */
    template<typename ...Index>
    inline int posOf(Index ...index) const
    {
        int indexs[] = {index...};
        std::size_t pos = 0;
        std::size_t N = sizeof... (Index);
        for (std::size_t i = 0; i < N; i++) {
            pos += sizes[i]*indexs[i];
        }
        return pos;
    }

    inline std::size_t posOf(const std::vector<int> &indexs) const
    {
        std::size_t pos = 0;
        for (std::size_t i = 0; i < sizes.size(); i++) {
            pos += sizes[i]*indexs[i];
        }
        return pos;
    }

    inline void indexOf(int pos, std::vector<int> &indexs) const
    {
        /*
            shape: (2, 3, 4, 5)
            sizes: (60, 20, 5, 1)
            totalsize 2*3*4*5 = 120
            indexs:(1, 2, 3, 4)
            pos : 60*1 + 20*2 + 5*3 + 4*1 = 119

            i0 = pos/60
            i1 = (pos - i0*60)/20
            i2 = (pos - i0*60 - i1*20)/5
            i3 = pos - i0*60 - i1*20 - i2*5
        */
        int pos_ = 0;
        for (std::size_t i = 0; i < sizes.size(); i++) {
            indexs[i] = (pos - pos_)/sizes[i];
            pos_ += indexs[i]*sizes[i];
        }
        return;
    }

    inline std::vector<int> indexOf(int pos) const
    {
        std::vector<int> indexes(shape.size(), 0);
        indexOf(pos, indexes);
        return indexes;
    }

    template<typename ...Index>
    inline T &operator()(Index ...index) { return val[posOf(index...)]; }

    template<typename ...Index>
    inline T operator()(Index ...index) const { return val[posOf(index...)]; }

    inline T &operator()(const Shape &indexs) { return val[posOf(indexs)]; }

    inline T operator()(const Shape &indexs) const { return val[posOf(indexs)]; }

    template<typename ...Index>
    Tensor_& reshape(Index ...index)
    {
        shape = {index...};
        initParams(shape, sizes, totalSize);
        return *this;
    }

    Tensor_ flatten() const
    {
        Tensor_ x(totalSize);
        x.val = val;
        return x;
    }

    static void permuteIndexs(const std::vector<int> &indexs,
                              const std::vector<int> &permuteMap,
                              std::vector<int> &newIndexs)
    {
        for (std::size_t i = 0; i < permuteMap.size(); i++) {
            int k = permuteMap[i];
            newIndexs[i] = indexs[k];
        }
        return;
    }

    template<typename ...Pos>
    inline Tensor_ permute(Pos ...p) const
    {
        /*
            shape: [3, 2, 1]
            permute: (2, 1, 0)
            new shape: [1, 2, 3]
        */
        std::vector<int> permuteMap = {p...};
        /* permute shape */
        std::vector<int> newShape(shape.size(), 0);
        permuteIndexs(shape, permuteMap, newShape);
        Tensor_ x(newShape);
        /* permute value */
        std::vector<int> indexs(shape.size(), 0);
        std::vector<int> newIndexs(shape.size(), 0);
        for (std::size_t i = 0; i < val.size(); i++) {
            indexOf(i, indexs);
            permuteIndexs(indexs, permuteMap, newIndexs);
            x(newIndexs) = val[i];
        }
        return x;
    }

    Tensor_ tr() const
    {
        int r = shape[0];
        int c = shape[1];
        Tensor_ y(c, r);
        for (int i = 0; i < r; i++) {
            for (int j = 0; j < c; j++) {
                y.val[j*c + i] = val[i*c + j];
            }
        }
        return y;
    }

    Tensor_ row(int i) const
    {
        Tensor_ r(shape[1]);
        int pos = i*shape[1];
        for (int j = 0; j < shape[1]; j++) {
            r[j] = val[j + pos];
        }
        return r;
    }

    void row(int i, const Tensor_ &r)
    {
        int pos = i*shape[1];
        for (int j = 0; j < shape[1]; j++) {
            val[j + pos] = r[j];
        }
        return;
    }

    Tensor_ column(int j) const
    {
        Tensor_ c(shape[0]);
        for (int i = 0; i < shape[0]; i++) {
            c[i] = val[i*shape[1] + j];
        }
        return c;
    }

    void column(int j, const Tensor_ &c)
    {
        for (int i = 0; i < shape[0]; i++) {
            val[i*shape[1] + j] = c[i];
        }
        return;
    }

    /* operator */
    Tensor_ operator +(const Tensor_ &x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] + x.val[i];
        }
        return y;
    }

    Tensor_ operator -(const Tensor_ &x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] - x.val[i];
        }
        return y;
    }

    Tensor_ operator *(const Tensor_ &x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] * x.val[i];
        }
        return y;
    }

    Tensor_ operator /(const Tensor_ &x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] / x.val[i];
        }
        return y;
    }

    Tensor_ &operator +=(const Tensor_ &x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] += x.val[i];
        }
        return *this;
    }

    Tensor_ &operator -=(const Tensor_ &x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] -= x.val[i];
        }
        return *this;
    }

    Tensor_ &operator *=(const Tensor_ &x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] *= x.val[i];
        }
        return *this;
    }

    Tensor_ operator /=(const Tensor_ &x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] /= x.val[i];
        }
        return *this;
    }

    Tensor_ operator +(T x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] + x;
        }
        return y;
    }

    Tensor_ operator -(T x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] - x;
        }
        return y;
    }

    Tensor_ operator *(T x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] * x;
        }
        return y;
    }

    Tensor_ operator /(T x) const
    {
        Tensor_ y(shape);
        for (std::size_t i = 0; i < val.size(); i++) {
            y.val[i] = val[i] / x;
        }
        return y;
    }

    Tensor_ &operator +=(T x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] += x;
        }
        return *this;
    }

    Tensor_ &operator -=(T x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] -= x;
        }
        return *this;
    }

    Tensor_ &operator *=(T x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] *= x;
        }
        return *this;
    }

    Tensor_ &operator /=(T x)
    {
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] /= x;
        }
        return *this;
    }

    /* statistics */
    template<typename ...Index>
    T sum(Index ...index) const
    {
        std::size_t totalsize = size(index...);
        std::size_t pos = posOf(index...);
        T s = 0;
        for (std::size_t i = 0; i < totalsize; i++) {
            s += val[i + pos];
        }
        return s;
    }

    T sum() const
    {
        T s = 0;
        for (std::size_t i = 0; i < totalSize; i++) {
            s += val[i];
        }
        return s;
    }

    template<typename ...Index>
    T mean(Index ...index) const
    {
        T N = T(size(index...));
        return sum(index...)/N;
    }

    T mean() const
    {
        T s = sum();
        return s/T(totalSize);
    }

    template<typename ...Index>
    T variance(T u, Index ...index) const
    {
        T N = T(size(index...));
        std::size_t pos = posOf(index...);
        T s = 0;
        for (std::size_t i = 0; i < N; i++) {
            s += (val[i + pos] - u)*(val[i + pos] - u);
        }
        return s/N;
    }

    T variance(T u) const
    {
        T s = 0;
        for (std::size_t i = 0; i < val.size(); i++) {
            s += (val[i] - u)*(val[i] - u);
        }
        return s/T(totalSize);
    }

    template<typename ...Index>
    T max(Index ...index) const
    {
        std::size_t N = size(index...);
        std::size_t pos = posOf(index...);
        T value = val[0];
        for (std::size_t i = 0; i < N; i++) {
            if (value < val[i + pos]) {
                value = val[i + pos];
            }
        }
        return value;
    }
    T max() const
    {
        T value = val[0];
        for (std::size_t i = 0; i < val.size(); i++) {
            if (value < val[i]) {
                value = val[i];
            }
        }
        return value;
    }

    template<typename ...Index>
    T min(Index ...index) const
    {
        std::size_t N = size(index...);
        std::size_t pos = posOf(index...);
        T value = val[0];
        for (std::size_t i = 0; i < N; i++) {
            if (value > val[i + pos]) {
                value = val[i + pos];
            }
        }
        return value;
    }
    T min() const
    {
        T value = val[0];
        for (std::size_t i = 0; i < val.size(); i++) {
            if (value > val[i]) {
                value = val[i];
            }
        }
        return value;
    }


    template<typename ...Index>
    std::size_t argmax(Index ...index) const
    {
        std::size_t N = size(index...);
        std::size_t pos = posOf(index...);
        T value = val[0];
        std::size_t index_ = 0;
        for (std::size_t i = 0; i < N; i++) {
            if (value < val[i + pos]) {
                value = val[i + pos];
                index_ = i;
            }
        }
        return index_ + pos;
    }

    int argmax() const
    {
        T value = val[0];
        int index = 0;
        for (std::size_t i = 0; i < val.size(); i++) {
            if (value < val[i]) {
                value = val[i];
                index = i;
            }
        }
        return index;
    }

    template<typename ...Index>
    std::size_t argmin(Index ...index) const
    {
        std::size_t N = size(index...);
        std::size_t pos = posOf(index...);
        T value = val[0];
        std::size_t index_ = 0;
        for (std::size_t i = 0; i < N; i++) {
            if (value > val[i + pos]) {
                value = val[i + pos];
                index_ = i;
            }
        }
        return index_ + pos;
    }

    int argmin() const
    {
        T value = val[0];
        int index = 0;
        for (std::size_t i = 0; i < val.size(); i++) {
            if (value > val[i]) {
                value = val[i];
                index = i;
            }
        }
        return index;
    }

    /* initialize */
    void normalize()
    {
        double minValue = val[0];
        double maxValue = val[0];
        for (std::size_t i = 0; i < val.size(); i++) {
            if (minValue > val[i]) {
                minValue = val[i];
            }
            if (maxValue < val[i]) {
                maxValue = val[i];
            }
        }
        for (std::size_t i = 0; i < val.size(); i++) {
            val[i] = (val[i] - minValue)/(maxValue - minValue);
        }
        return;
    }

    T norm2() const
    {
        T s = 0;
        for (std::size_t i = 0; i < totalSize; i++) {
            s += val[i]*val[i];
        }
        return std::sqrt(s);
    }

    struct Mul {
        static void ikkj(Tensor_ &x, const Tensor_ &x1, const Tensor_ &x2)
        {
            for (std::size_t i = 0; i < x.shape[0]; i++) {
                for (std::size_t k = 0; k < x1.shape[1]; k++) {
                    for (std::size_t j = 0; j < x.shape[1]; j++) {
                        /* (i, j) = (i, k) * (k, j) */
                        x.val[i*x.shape[1] + j] += x1.val[i*x1.shape[1] + k]*x2.val[k*x2.shape[1] + j];
                    }
                }
            }
            return;
        }
        static void kikj(Tensor_ &x, const Tensor_ &x1, const Tensor_ &x2)
        {
            /* transpose x1 */
            for (std::size_t i = 0; i < x.shape[0]; i++) {
                for (std::size_t k = 0; k < x1.shape[0]; k++) {
                    for (std::size_t j = 0; j < x.shape[1]; j++) {
                        /* (i, j) = (k, i)^T * (k, j) */
                        x.val[i*x.shape[1] + j] += x1.val[k*x1.shape[1] + i]*x2.val[k*x2.shape[1] + j];
                    }
                }
            }
            return;
        }
        static void ikjk(Tensor_ &x, const Tensor_ &x1, const Tensor_ &x2)
        {
            /* transpose x2 */
            for (std::size_t i = 0; i < x.shape[0]; i++) {
                for (std::size_t k = 0; k < x1.shape[1]; k++) {
                    for (std::size_t j = 0; j < x.shape[1]; j++) {
                        /* (i, j) = (i, k) * (j, k)^T */
                        x.val[i*x.shape[1] + j] += x1.val[i*x1.shape[1] + k]*x2.val[j*x2.shape[1] + k];
                    }
                }
            }
            return;
        }
        static void kijk(Tensor_ &x, const Tensor_ &x1, const Tensor_ &x2)
        {
            /* transpose x1, x2 */
            for (std::size_t i = 0; i < x.shape[0]; i++) {
                for (std::size_t k = 0; k < x1.shape[0]; k++) {
                    for (std::size_t j = 0; j < x.shape[1]; j++) {
                        /* (i, j) = (k, i)^T * (j, k)^T */
                        x.val[i*x.shape[1] + j] += x1.val[k*x1.shape[1] + i] * x2.val[j*x2.shape[1] + k];
                    }
                }
            }
            return;
        }
    };

    /* display */
    template<typename ...Index>
    void printValue(Index ...index) const
    {
        std::size_t N = size(index...);
        std::size_t pos = posOf(index...);
        std::cout<<"[";
        for (std::size_t i = 0; i < N; i++) {
            std::cout<<val[i + pos];
            if (i < N - 1) {
                std::cout<<",";
            }
        }
        std::cout<<"]"<<std::endl;
        return;
    }

    void printValue() const
    {
        std::cout<<"[";
        for (std::size_t i = 0; i < val.size(); i++) {
            std::cout<<val[i];
            if (i < totalSize - 1) {
                std::cout<<",";
            }
        }
        std::cout<<"]"<<std::endl;
        return;
    }

    void printShape() const
    {
        std::cout<<"(";
        for (std::size_t i = 0; i < shape.size(); i++) {
            std::cout<<shape[i];
            if (i < totalSize - 1) {
                std::cout<<",";
            }
        }
        std::cout<<")"<<std::endl;
        return;
    }

    inline static Tensor_ product2D(const Tensor_& x1, const Tensor_& x2)
    {
        int r = x1.shape[0]*x2.shape[0];
        int c = x1.shape[1]*x2.shape[1];
        Tensor_ y(r, c);
        for (int i = 0; i < x1.shape[0]; i++) {
            for (int j = 0; j < x1.shape[1]; j++) {
                for (int h = 0; h < x2.shape[0]; h++) {
                    for (int k = 0; k < x2.shape[1]; k++) {
                        y(h + i*x1.shape[0], k + j*x1.shape[1]) = x1(i, j)*x2(h, k);
                    }
                }
            }
        }
        return y;
    }


};

template<typename T , typename Simd, template<typename Ti> class Alloc=AlignAllocator32>
class Tensorsi_ : public Tensor_<T, Alloc>
{
public:
    using instruct = Simd;
    using __Tensor = Tensor_<T, Alloc>;
    using __Tensor::totalSize;
    using __Tensor::val;
    using __Tensor::shape;
    using __Tensor::sizes;
    using Shape = typename __Tensor::Shape;
    constexpr static std::size_t unit = Simd::template Selector<T>::value*Simd::template Selector<T>::value;
public:
    Tensorsi_(){}
    /* contruct with shape */
    explicit Tensorsi_(const std::vector<int> &shape_):__Tensor(shape_){}
    explicit Tensorsi_(const std::vector<int> &shape_, const __Tensor &val_)
        :__Tensor(shape_, val_){}
    explicit Tensorsi_(const std::initializer_list<int> &shape_, const std::initializer_list<T> &val_)
        :__Tensor(shape_, val_){}
    /* construct with shape */
    template<typename ...Dim>
    explicit Tensorsi_(Dim ...dim):__Tensor(dim...){}
    /* copy construct */
    Tensorsi_(const __Tensor &r):__Tensor (r){}
    Tensorsi_(const Tensorsi_ &r):__Tensor(r.shape, r.val){}
    /* assign operator */
    Tensorsi_ &operator=(const Tensorsi_ &r)
    {
        if (this == &r) {
            return *this;
        }
        __Tensor::operator=(r);
        return *this;
    }
    Tensorsi_ &operator=(const __Tensor &r)
    {
        if (this == &r) {
            return *this;
        }
        __Tensor::operator=(r);
        return *this;
    }

    Tensorsi_ &operator=(T value)
    {
        __Tensor::operator=(value);
        return *this;
    }

    void zero()
    {
        if (totalSize < unit) {
            return __Tensor::zero();
        }
        instruct::fill(__Tensor::ptr(), 0, totalSize);
        return;
    }
    void fill(T value)
    {
        if (totalSize < unit) {
            return __Tensor::fill(value);
        }
        instruct::fill(__Tensor::ptr(), value, totalSize);
        return;
    }

    static Tensorsi_ zeros(Shape &shape)
    {
        Tensorsi_ x(shape);
        return x;
    }

    template<typename ...Dim>
    static Tensorsi_ zeros(Dim ...dim)
    {
        Tensorsi_ x(dim...);
        return x;
    }

    static Tensorsi_ ones(Shape &shape)
    {
        Tensorsi_ x(shape);
        if (totalSize < unit) {
            return __Tensor::fill(1);
        }
        instruct::fill(__Tensor::ptr(), 1, totalSize);
        return x;
    }

    template<typename ...Index>
    static Tensorsi_ ones(Index ...index)
    {
        Tensorsi_ x(index...);
        instruct::fill(x.ptr(), 1, x.totalSize);
        return x;
    }

    /* operator */
    Tensorsi_ operator +(const Tensorsi_ &x) const
    {
        Tensorsi_ y(shape);
        if (x.totalSize < unit) {
            return __Tensor::operator+(x);
        }
        instruct::add(y.ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return y;
    }

    Tensorsi_ operator -(const Tensorsi_ &x) const
    {
        Tensorsi_ y(shape);
        if (x.totalSize < unit) {
            return __Tensor::operator-(x);
        }
        instruct::sub(y.ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return y;
    }

    Tensorsi_ operator *(const Tensorsi_ &x) const
    {
        Tensorsi_ y(shape);
        if (x.totalSize < unit) {
            return __Tensor::operator*(x);
        }
        instruct::mul(y.ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return y;
    }

    Tensorsi_ operator /(const Tensorsi_ &x) const
    {
        Tensorsi_ y(shape);
        if (totalSize < unit) {
            return __Tensor::operator/(x);
        }
        instruct::div(y.ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return y;
    }

    Tensorsi_ operator +(T x) const
    {
        Tensorsi_ y(shape);
        if (totalSize < unit) {
            return __Tensor::operator+(x);
        }
        instruct::add(y.ptr(), __Tensor::ptr(), x, totalSize);
        return y;
    }

    Tensorsi_ operator -(T x) const
    {
        Tensorsi_ y(shape);
        if (totalSize < unit) {
            return __Tensor::operator-(x);
        }
        instruct::sub(y.ptr(), __Tensor::ptr(), x, totalSize);
        return y;
    }

    Tensorsi_ operator *(T x) const
    {
        Tensorsi_ y(shape);
        if (totalSize < unit) {
            return __Tensor::operator*(x);
        }
        instruct::mul(y.ptr(), __Tensor::ptr(), x, totalSize);
        return y;
    }

    Tensorsi_ operator /(T x) const
    {
        Tensorsi_ y(shape);
        if (totalSize < unit) {
            return __Tensor::operator/(x);
        }
        instruct::div(y.ptr(), __Tensor::ptr(), x, totalSize);
        return y;
    }

    Tensorsi_ &operator +=(const Tensorsi_& x)
    {
        if (totalSize < unit) {
            __Tensor::operator+=(x);
            return *this;
        }
        instruct::add(__Tensor::ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return *this;
    }

    Tensorsi_ &operator -=(const Tensorsi_& x)
    {
        if (totalSize < unit) {
            __Tensor::operator-=(x);
            return *this;
        }
        instruct::sub(__Tensor::ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return *this;
    }

    Tensorsi_ &operator *=(const Tensorsi_& x)
    {
        if (totalSize < unit) {
            __Tensor::operator*=(x);
            return *this;
        }
        instruct::mul(__Tensor::ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return *this;
    }

    Tensorsi_ &operator /=(const Tensorsi_& x)
    {
        if (totalSize < unit) {
            __Tensor::operator/=(x);
            return *this;
        }
        instruct::div(__Tensor::ptr(), __Tensor::ptr(), x.ptr(), totalSize);
        return *this;
    }

    Tensorsi_ &operator +=(T x)
    {
        if (totalSize < unit) {
            __Tensor::operator+=(x);
            return *this;
        }
        instruct::add(__Tensor::ptr(), __Tensor::ptr(), x, totalSize);
        return *this;
    }

    Tensorsi_ &operator -=(T x)
    {
        if (totalSize < unit) {
            __Tensor::operator-=(x);
            return *this;
        }
        instruct::sub(__Tensor::ptr(), __Tensor::ptr(), x, totalSize);
        return *this;
    }

    Tensorsi_ &operator *=(T x)
    {
        if (totalSize < unit) {
            __Tensor::operator*=(x);
            return *this;
        }
        instruct::mul(__Tensor::ptr(), __Tensor::ptr(), x, totalSize);
        return *this;
    }

    Tensorsi_ &operator /=(T x)
    {
        if (totalSize < unit) {
            __Tensor::operator/=(x);
            return *this;
        }
        instruct::div(__Tensor::ptr(), __Tensor::ptr(), x, totalSize);
        return *this;
    }
    /* statistics */
    template<typename ...Index>
    T sum(Index ...index) const
    {
        return __Tensor::sum(index...);
    }

    T sum() const
    {
        if (totalSize < unit) {
            return __Tensor::sum();
        }
        return instruct::sum(__Tensor::ptr(), totalSize);
    }

    template<typename ...Index>
    T mean(Index ...index) const
    {
        return __Tensor::mean(index...);
    }

    T mean() const{ return sum()/T(totalSize);}

    template<typename ...Index>
    T variance(T u, Index ...index) const
    {
        return __Tensor::variance(u, index...);
    }

    T variance(T u) const
    {
        if (totalSize < unit) {
            return __Tensor::variance(u);
        }
        return instruct::variance(__Tensor::ptr(), u, totalSize);
    }

    template<typename ...Index>
    T max(Index ...index) const
    {
        return __Tensor::max(index...);
    }

    T max() const
    {
        if (totalSize < unit) {
            return __Tensor::max();
        }
        return instruct::max(__Tensor::ptr(), totalSize);
    }

    template<typename ...Index>
    T min(Index ...index) const
    {
        return __Tensor::min(index...);
    }

    T min() const
    {
        if (totalSize < unit) {
            return __Tensor::min();
        }
        return instruct::min(__Tensor::ptr(), totalSize);
    }

    /* matrix operation */
    struct Mul {
        static void ikkj(Tensorsi_ &x, const Tensorsi_ &x1, const Tensorsi_ &x2)
        {
            /* x = x1 * x2 */
//            if (x1.shape[0] < unit || x1.shape[1] < unit || x2.shape[1] < unit) {
//                return __Tensor::Mul::ikkj(x, x1, x2);
//            }
            instruct::MatMul::ikkj(x.ptr(), x.shape[0], x.shape[1],
                                   x1.ptr(), x1.shape[0], x1.shape[1],
                                   x2.ptr(), x2.shape[0], x2.shape[1]);
            return;
        }
        static void kikj(Tensorsi_ &x, const Tensorsi_ &x1, const Tensorsi_ &x2)
        {
            /* x = x1^T * x2 */
            if (x1.shape[0] < unit || x1.shape[1] < unit || x2.shape[1] < unit) {
                return __Tensor::Mul::kikj(x, x1, x2);
            }
            /* transpose x1 */
            instruct::MatMul::kikj(x.ptr(), x.shape[0], x.shape[1],
                                   x1.ptr(), x1.shape[0], x1.shape[1],
                                   x2.ptr(), x2.shape[0], x2.shape[1]);
            return;
        }
        static void ikjk(Tensorsi_ &x, const Tensorsi_ &x1, const Tensorsi_ &x2)
        {
            /* x = x1 * x2^T */
            if (x1.shape[0] < unit || x1.shape[1] < unit || x2.shape[0] < unit) {
                return __Tensor::Mul::ikjk(x, x1, x2);
            }
            /* transpose x2 */
            instruct::MatMul::ikjk(x.ptr(), x.shape[0], x.shape[1],
                                   x1.ptr(), x1.shape[0], x1.shape[1],
                                   x2.ptr(), x2.shape[0], x2.shape[1]);
            return;
        }
        static void kijk(Tensorsi_ &x, const Tensorsi_ &x1, const Tensorsi_ &x2)
        {
            /* x = x1^T * x2^T */
            if (x1.shape[0] < unit || x1.shape[1] < unit || x2.shape[0] < unit) {
                return __Tensor::Mul::kijk(x, x1, x2);
            }
            /* transpose x1, x2 */
            instruct::MatMul::kijk(x.ptr(), x.shape[0], x.shape[1],
                                   x1.ptr(), x1.shape[0], x1.shape[1],
                                   x2.ptr(), x2.shape[0], x2.shape[1]);
            return;
        }

    };

};
using Tensorc  = Tensor_<char>;
using Tensoru8 = Tensor_<unsigned char>;
using Tensori  = Tensor_<int>;
using Tensorf  = Tensor_<float>;
using Tensord  = Tensor_<double>;

#if defined(__AVX2__)
using Tensorsi = Tensorsi_<float, simd::AVX2, AlignAllocator32>;
//using Tensorsi = Tensorsi_<float, simd::AVX2, std::allocator>;
#endif

#if 0
    using Tensor = Tensorsi;
#else
    using Tensor = Tensorf;
    using InTensor = const Tensor&;
    using OutTensor = Tensor&;
#endif

#endif // TENSOR_H
