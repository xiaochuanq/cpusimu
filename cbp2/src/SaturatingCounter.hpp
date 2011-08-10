#ifndef SATURATING_COUNTER_H
#define SATURATING_COUNTER_H

#include <algorithm>
#include <assert.h>

class SaturatingCounter {
public:
    SaturatingCounter(int bits)  : _counter(0), _bits(bits) {
        assert(_bits > 0);

        int pow = 1 << (bits -1);
        _min = -pow;
        _max = pow - 1;
    }

    void reset() {
        _counter = 0;
    }

    void inc() {
        _counter = std::min(_max, _counter+1);
    }

    void dec() {
        _counter = std::max(_min, _counter-1);
    }

    int val() const {
        return _counter;
    }

    int size() const {
        return _bits;
    }

    bool isSaturated() const {
        return (_min == _counter) || (_max == _counter);

    }
private:
    int _counter;
    int _bits;
    int _min, _max;
};

#endif
