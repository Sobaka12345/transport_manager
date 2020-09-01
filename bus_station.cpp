#include <cmath>

#include "bus_station.h"
#include "bus.h"

using namespace std;

BusStation::BusStation(string name, double latitude, double longitude) :
    name_(name),
    latitude_(latitude),
    longitude_(longitude)
{}

bool BusStation::setWaitVertex(size_t vertexID)
{
    if(!waitVertex_.has_value())
    {
        waitVertex_ = vertexID;
        return true;
    }
    return false;
}

size_t BusStation::getWaitVertex() const
{
    return waitVertex_.value();
}

bool BusStation::setMainVertex(size_t vertexID)
{
    if(!mainVertex_.has_value())
    {
        mainVertex_ = vertexID;
        return true;
    }
    return false;
}

size_t BusStation::getMainVertex() const
{
    return mainVertex_.value();
}

double BusStation::getLatitude() const
{
    return latitude_;
}

double BusStation::getLongitude() const
{
    return longitude_;
}

void BusStation::addDistance(string_view station, size_t distance)
{
    distances_.insert({ station, distance });
}

const string& BusStation::getName() const
{
    return name_;
}

optional<double> BusStation::getDistance(shared_ptr<BusStation> station)
{
    if(auto it = distances_.find(station->getName()); it != distances_.end())
        return it->second;
    else if (auto it = station->distances_.find(getName()); it != station->distances_.end())
        return it->second;
    else
        return nullopt;
}

void BusStation::addBus(shared_ptr<Bus> bus)
{
    buses_.insert({bus->getName() , bus });
}

double BusStation::operator-(const BusStation& val) const
{
    double d = acos(
                sin(val.latitude_) * sin(latitude_) +
                cos(val.latitude_) * cos(latitude_) *
                cos(val.longitude_ - longitude_));
    return d * EARTH_RADIUS;
}

void BusStation::printInJson(size_t req_id, ostream& stream)
{
    stream << "{";
    stream << "\"buses\": [";
    size_t i = 0;
    for(auto it = buses_.begin(); it != buses_.end(); it++)
    {
        stream << "\"" << it->first << "\"";
        if(i++ < buses_.size() - 1)
            stream << ",";
        else
            stream << "";
    }
    stream << "],";
    stream << "\"request_id\": " << req_id << "";
    stream << "}";
}
