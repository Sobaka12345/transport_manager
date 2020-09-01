#ifndef BUS_H
#define BUS_H

#include <string_view>
#include <optional>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <set>

class BusStation;

class Bus
{
public:

protected:
    using StationPtr = std::shared_ptr<BusStation>;

    std::string name_;
    bool isLooped_;

    std::vector<StationPtr> stations_;
    std::set<std::string_view> uniqueStations_;
    std::optional<size_t> realLength_;
    std::optional<double> globalLength_;


public:
    Bus(std::string name, std::vector<StationPtr> stations, bool isLooped = false);
    Bus(std::string name, bool isLooped = false);

    void addStation(std::shared_ptr<BusStation> station);

    virtual size_t getRealLength();
    virtual double getGlobalLength();
    virtual size_t getStationCount() const;

    const std::vector<StationPtr>& getStations() const;
    const std::string& getName() const;
    double getCurvature();
    size_t getUniqueStations();
    bool isLooped() const;

    void printInJson(size_t req_id, std::ostream &stream = std::cout);
};

#endif // BUS_H
