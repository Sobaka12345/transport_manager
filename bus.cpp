#include "bus.h"
#include "bus_station.h"

using namespace std;

Bus::Bus(string name, vector<StationPtr> stations, bool isLooped) :
    name_(move(name)),
    isLooped_(isLooped),
    stations_(move(stations))
{}

Bus::Bus(string name, bool isLooped) :
    name_(move(name)),
    isLooped_(isLooped)
{}

const string& Bus::getName() const
{
    return name_;
}

const vector<Bus::StationPtr>& Bus::getStations() const
{
    return stations_;
}


size_t Bus::getUniqueStations()
{
    if(uniqueStations_.size())
        return uniqueStations_.size();

    for(auto & x : stations_)
    {
        uniqueStations_.emplace(x->getName());
    }
    return uniqueStations_.size();
}

bool Bus::isLooped() const
{
    return isLooped_;
}

void Bus::addStation(shared_ptr<BusStation> station)
{
    stations_.push_back(station);
}

size_t Bus::getRealLength()
{
    if(realLength_)
        return realLength_.value();
    realLength_ = 0.0;

    for(auto it = stations_.begin() + 1; it != stations_.end(); it++)
    {
        if(auto dist = (*(it - 1))->getDistance(*it); dist.has_value())
            realLength_ = realLength_.value() + dist.value();
    }

    if(isLooped_)
        return realLength_.value();

    for(auto it = stations_.rbegin() + 1; it != stations_.rend(); it++)
    {
        if(auto dist = (*(it - 1))->getDistance(*it); dist.has_value())
            realLength_ = realLength_.value() + dist.value();
    }

    return realLength_.value();
}

double Bus::getGlobalLength()
{
    if(globalLength_)
        return globalLength_.value();
    globalLength_ = 0.0;

    for(auto it = stations_.begin() + 1; it != stations_.end(); it++)
    {
        globalLength_ = globalLength_.value() + (**it - **(it - 1));
    }

    if(isLooped_)
        return globalLength_.value();

    globalLength_.value() *= 2;

    return globalLength_.value();
}

size_t Bus::getStationCount() const
{
    return isLooped_ ? stations_.size() : stations_.size() * 2 - 1;
}

double Bus::getCurvature()
{
    return getRealLength() / getGlobalLength();
}

void Bus::printInJson(size_t req_id, ostream& stream)
{
    stream << "{";
    stream << "\"route_length\": " << getRealLength() << ",";
    stream << "\"request_id\": " << req_id << ",";
    stream << "\"curvature\": " << getCurvature() << ",";
    stream << "\"stop_count\": " << getStationCount() << ",";
    stream << "\"unique_stop_count\": " << getUniqueStations() << "";
    stream << "}";
}
