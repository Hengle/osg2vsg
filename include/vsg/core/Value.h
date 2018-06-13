#pragma once

#include <vsg/core/Object.h>

namespace vsg
{

    template<typename T>
    class Value : public Object
    {
    public:
        Value() {}
        Value(const T& in_value) : value(in_value) {}
        Value(const Value& rhs) : value(rhs.value) {}

        virtual void accept(Visitor& visitor) { visitor.apply(*this); }

        T value;

    protected:
        virtual ~Value() {}
    };

    template<typename T>
    void Object::setValue(const Key& key, const T& value)
    {
        using ValueT = Value<T>;
        setObject(key, new ValueT(value));
    }

    template<typename T>
    bool Object::getValue(const Key& key, T& value) const
    {
        using ValueT = Value<T>;
        const ValueT* vo = dynamic_cast<const ValueT*>(getObject(key));
        if (vo)
        {
            value = vo->value;
            return true;
        }
        else
        {
            return false;
        }
    }

#if 0
    // would prefer to be able to just provide "using" declrartions, but for some reason this is incompatible with Visitor forward ceclaring as class.
    using StringValue = Value<std::string>;
    using IntValue = Value<int>;
    using FloatValue = Value<float>;
    using DoubleValue = Value<double>;
#else

    class StringValue : public Value<std::string> {};
    class IntValue : public Value<int> {};
    class FloatValue : public Value<float> {};
    class DoubleValue : public Value<double> {};

#endif

}