#ifndef JSON_SERIALIZE_HPP
#define JSON_SERIALIZE_HPP

#include <iostream>
#include <optional>
#include <iomanip>
#include <variant>
#include <vector>
#include <memory>
#include <queue>

namespace Json {

class JsonBase
{
protected:
    JsonBase* parent_;
    std::queue<std::unique_ptr<JsonBase>> vals_;
    std::ostream& stream_;
    bool is_ended_ = false;

    JsonBase(std::ostream& stream, JsonBase* parent = nullptr) :
        parent_(parent),
        stream_(stream)
    {}


public:
    virtual ~JsonBase() = default;
    void PushValue(std::unique_ptr<JsonBase> val)
    {
        vals_.push(move(val));
    }
    std::ostream& GetStream()
    {
        return stream_;
    }
};

class JsonValue : public JsonBase, public std::variant<std::monostate, std::string_view, bool, std::int64_t, double>
{
public:
    JsonValue(std::ostream& stream, std::variant<std::monostate, std::string_view, bool, int64_t, double> val):
        JsonBase(stream),
        std::variant<std::monostate, std::string_view, bool, int64_t, double>(val)
    {}

    ~JsonValue() override
    {
        switch (this->index()) {
        case 0:
            stream_ << "null";
            break;
        case 1:
            stream_ << quoted(std::get<1>(*this));
            break;
        case 2:
            stream_ << std::boolalpha << std::get<2>(*this);
            break;
        case 3:
            stream_ << std::get<3>(*this);
            break;
        case 4:
            stream_ << std::get<4>(*this);
            break;
        }
    }
};

template <typename T>
class JsonAdders
{
    auto& GetType()
    {
        return static_cast<T&>(*this);
    }

    auto& ReturnValue()
    {
        return GetType().ValueToReturn();
    }

public:
    auto& String(std::string_view str)
    {
        GetType().PushValue(std::make_unique<JsonValue>(GetType().GetStream(), str));
        return ReturnValue();
    }

    auto& Null()
    {
        GetType().PushValue(std::make_unique<JsonValue>(GetType().GetStream(), std::monostate()));
        return ReturnValue();
    }

    auto& Double(double val)
    {
        GetType().PushValue(std::make_unique<JsonValue>(GetType().GetStream(), val));
        return ReturnValue();
    }

    auto& Integer(int64_t val)
    {
        GetType().PushValue(std::make_unique<JsonValue>(GetType().GetStream(), val));
        return ReturnValue();
    }

    auto& Boolean(bool val)
    {
        GetType().PushValue(std::make_unique<JsonValue>(GetType().GetStream(), val));
        return ReturnValue();
    }
};

template <typename T>
class JsonObject;

template <typename T>
class JsonArray: public JsonBase, public JsonAdders<JsonArray<T>>
{

public:
    JsonArray(std::ostream& stream, JsonBase* parent = nullptr) :
        JsonBase(stream, parent)
    {}

    JsonArray<T>& ValueToReturn()
    {
        return *this;
    }

    JsonArray<JsonArray<T>>& BeginArray()
    {
        vals_.push(std::make_unique<JsonArray<JsonArray<T>>>(stream_, this));
        return static_cast<JsonArray<JsonArray<T>>&>(*vals_.back());
    }

    JsonObject<JsonArray<T>>& BeginObject()
    {
        vals_.push(std::make_unique<JsonObject<JsonArray<T>>>(stream_, this));
        return static_cast<JsonObject<JsonArray<T>>&>(*vals_.back());
    }

    T& EndArray()
    {
        return static_cast<T&>(*parent_);
    }

    ~JsonArray() override
    {
        stream_ << "[";
        while(vals_.size() > 1)
        {
            vals_.pop();
            stream_ << ',';
        }
        if(vals_.size())
            vals_.pop();
        stream_ << "]";
    }
};

template <typename T>
class JsonKey: public JsonBase, public JsonAdders<JsonKey<T>>
{
    std::string_view name_;

public:
    JsonKey(std::ostream& stream, std::string_view name, JsonBase* parent = nullptr) :
        JsonBase(stream, parent),
        name_(name)
    {}

    T& ValueToReturn()
    {
        return static_cast<T&>(*parent_);
    }

    JsonArray<T>& BeginArray()
    {
        vals_.push(std::make_unique<JsonArray<T>>(stream_, parent_));
        return static_cast<JsonArray<T>&>(*vals_.back());
    }

    JsonObject<T> &BeginObject()
    {
        vals_.push(std::make_unique<JsonObject<T>>(stream_, parent_));
        return static_cast<JsonObject<T>&>(*vals_.back());
    }

    ~JsonKey() override
    {
        stream_ << quoted(name_) << ':';
        if(vals_.size())
            vals_.pop();
        else stream_ << "null";
    }
};

template <typename T>
class JsonObject: public JsonBase
{
public:
    JsonObject(std::ostream& stream, JsonBase* parent = nullptr) :
        JsonBase(stream, parent)
    {}

    JsonObject<T>& ValueToReturn()
    {
        return *this;
    }

    JsonKey<JsonObject<T>>& Key(std::string_view name)
    {
        vals_.push(std::make_unique<JsonKey<JsonObject<T>>>(this->stream_, name, this));
        return static_cast<JsonKey<JsonObject<T>>&>(*vals_.back());
    }

    T& EndObject()
    {
        return static_cast<T&>(*parent_);
    }

    ~JsonObject() override
    {
        stream_ << "{";
        while(vals_.size() > 1)
        {
            vals_.pop();
            stream_ << ',';
        }
        if(vals_.size())
            vals_.pop();
        stream_ << "}";
    }
};


void PrintJsonString(std::ostream& stream, std::string_view str);

JsonArray<JsonBase> PrintJsonArray(std::ostream& stream);

JsonObject<JsonBase> PrintJsonObject(std::ostream& stream);

}
#endif // JSON_SERIALIZE_HPP

