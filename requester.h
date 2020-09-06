#ifndef REQUESTER_H
#define REQUESTER_H

#include <map>
#include <string>
#include <functional>

class Requester
{
public:
    Requester() = default;
    std::map<std::string, std::function<bool(int)>> performers_;

    template<typename ...T>
    void addRequestType(std::string, std::function<bool(int,  T& ...)> func)
    {
        performers_.insert({cmd, func});
    }

    template<typename ...T>
    bool performQuery(const std::string& query, int id)
    {
        return performers_[query](id);
    }
};

#endif // REQUESTER_H
