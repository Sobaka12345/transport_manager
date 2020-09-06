#ifndef BUSSTATION_H
#define BUSSTATION_H

#include <unordered_map>
#include <iostream>
#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "json_serialize.hpp"

class Bus;

static const size_t  EARTH_RADIUS = 6'371'000;
static constexpr double PI = 3.1415926535;

class BusStation
{

    std::map<std::string_view, std::shared_ptr<Bus>> buses_;
    std::unordered_map<std::string_view, size_t> distances_;
    std::optional<size_t> mainVertex_, waitVertex_;
    std::string name_;

    double latitude_, longitude_;

public:
    BusStation(std::string name, double latitude, double longitude);

    double operator-(const BusStation &val) const;

    void printInJson(size_t req_id, Json::JsonArray<Json::JsonBase>& obj);

    bool setWaitVertex(size_t vertexID);
    bool setMainVertex(size_t vertexID);

    void addBus(std::shared_ptr<Bus> bus);
    void addDistance(std::string_view station, size_t distance);

    size_t getWaitVertex() const;
    size_t getMainVertex() const;
    double getLatitude() const;
    double getLongitude() const;
    const std::string &getName() const;
    std::optional<double> getDistance(std::shared_ptr<BusStation> station);
};

#endif // BUSSTATION_H
