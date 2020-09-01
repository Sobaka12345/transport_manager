#ifndef PATH_ITEM_H
#define PATH_ITEM_H

#include <iostream>
#include <string_view>
#include <iomanip>

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

    friend std::ostream& operator<<(std::ostream& stream, const PathItem& val)
    {
        stream << "{\"time\":" << std::setprecision(6) << val.time_ << ",";
        switch (val.type_) {
        case DRIVE:
            stream << "\"span_count\":" << val.spanCount_ << ",";
            stream << "\"bus\":\"" << val.name_ << "\",";
            stream << "\"type\":\"Bus\"";
            break;
        case WAIT:
            stream << "\"stop_name\":\"" << val.name_ << "\",";
            stream << "\"type\":\"Wait\"";
            break;
        }
        stream << "}";
        return stream;
    }
};

#endif // PATH_ITEM_H
