#ifndef PATH_ITEM_H
#define PATH_ITEM_H

#include <iostream>
#include <string_view>
#include <sstream>
#include <iomanip>
#include "json_serialize.hpp"

class PathItem
{
    enum Type {
        WAIT,
        DRIVE,
        NONE
    };

    double time_;
    std::string_view name_;
    size_t spanCount_;
    Type type_;

public:
    PathItem(double time) :
        time_(time),
        spanCount_(0),
        type_(NONE)
    {}

    PathItem(std::string_view name, double time, size_t spanCount) :
        time_(time),
        name_(name),
        spanCount_(spanCount),
        type_(DRIVE)
    {}

    PathItem(std::string_view name, double time) :
        time_(time),
        name_(name),
        type_(WAIT)
    {}

    double getTime() const
    {
        return time_;
    }

    PathItem operator+(const PathItem& val) const
    {
        return PathItem(time_ + val.time_);
    }

    bool operator<(const PathItem& val) const
    {
        return time_ < val.time_;
    }

    template<typename JsonArray>
    void printInJson(JsonArray& arr) const
    {
        auto& obj = arr.BeginObject()
            .Key("time").Double(time_);

        switch (type_) {
        case DRIVE:
            obj.Key("span_count").Integer(spanCount_)
               .Key("bus").String(name_)
               .Key("type").String("Bus");
            break;
        case WAIT:
            obj.Key("stop_name").String(name_)
               .Key("type").String("Wait");
            break;
        }
        obj.EndObject();
    }
};

#endif // PATH_ITEM_H
