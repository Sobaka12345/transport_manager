#include "transport_manager.h"
#include "bus.h"
#include "bus_station.h"
#include "path_item.h"
#include "json_serialize.hpp"

#include <cmath>
#include <iostream>
#include <variant>

using namespace std;

Svg::Point TransportManager::createPoint(double latitude, double longitude) const
{
    return {
        (longitude - minLongitude_.value()) * zoomCoef_.value() + renderSettings_.padding,
        (maxLatitude_.value() - latitude) * zoomCoef_.value() + renderSettings_.padding
      };
}

TransportManager& TransportManager::createInstance(const vector<Json::Node>& base_requests)
{
    static TransportManager instance(base_requests);

    return instance;
}

TransportManager::TransportManager(const vector<Json::Node>& base_requests)
{
    vector<int> busQueries;

    for(size_t i = 0; i < base_requests.size(); i++)
    {
        const auto & map = base_requests[i].AsMap();

        if(map.at("type").AsString() == "Bus")
            busQueries.push_back(i);
        else
        {
            double latitude = map.at("latitude").AsDouble() / 180.0 * PI;
            double longitude = map.at("longitude").AsDouble() / 180.0 * PI;

            if(!minLatitude_.has_value())
            {
                minLatitude_ = latitude;
                maxLatitude_ = latitude;
                minLongitude_ = longitude;
                maxLongitude_ = longitude;
            } else
            {
                minLatitude_ = min(latitude, minLatitude_.value());
                maxLatitude_ = max(latitude, maxLatitude_.value());
                minLongitude_ = min(longitude, minLongitude_.value());
                maxLongitude_ = max(longitude, maxLongitude_.value());
            }

            addStation(map.at("name").AsString(),
                       latitude,
                       longitude,
                       map.at("road_distances").AsMap());
        }
    }

    for(auto x : busQueries)
    {
        const auto & map = base_requests[x].AsMap();
        const auto & stops = map.at("stops").AsArray();

        addBus(map.at("name").AsString(), stops, map.at("is_roundtrip").AsBool());
    }
}

void TransportManager::updateRouter()
{    
    size_t vertexCount = stations_.size() * 2;
    graph_ = make_unique<GraphType>(vertexCount);
    size_t waitTime = routingSettings_.busWait;
    double busVelocity = routingSettings_.busVelocity;

    for(auto & x : stations_)
        graph_->AddEdge({x.second->getWaitVertex(), x.second->getMainVertex(), PathItem(x.second->getName(), waitTime)});

    for(auto & x : buses_)
    {
        const auto & stations = x.second->getStations();

        for(auto first = stations.begin(); first != stations.end(); first++)
        {
            double val = 0;
            size_t spans = 0;
            for(auto second = first + 1; second != stations.end(); second++)
            {
                val += (*(second - 1))->getDistance(*second).value() / busVelocity;
                graph_->AddEdge({(*first)->getMainVertex(), (*second)->getWaitVertex(), PathItem(x.second->getName(), val, ++spans)});
            }

            if(!x.second->isLooped())
                for(auto second = stations.rbegin() + 1; second != stations.rend(); second++)
                {
                    val += (*(second - 1))->getDistance(*second).value() / busVelocity;
                    graph_->AddEdge({(*first)->getMainVertex(), (*second)->getWaitVertex(), PathItem(x.second->getName(), val, ++spans)});
                }
        }

        if(!x.second->isLooped())
            for(auto first = stations.rbegin(); first != stations.rend(); first++)
            {
                double val = 0;
                size_t spans = 0;
                for(auto second = first + 1; second != stations.rend(); second++)
                {
                    val += (*(second - 1))->getDistance(*second).value() / busVelocity;
                    graph_->AddEdge({(*first)->getMainVertex(), (*second)->getWaitVertex(), PathItem(x.second->getName(), val, ++spans)});
                }
            }
    }

    router_ = make_unique<RouterType>(*graph_);
}

void TransportManager::addBus(string name, vector<Json::Node> stations, bool isLooped)
{
    shared_ptr<Bus> bus = make_shared<Bus>(name, isLooped);

    auto first = stations_.find(stations.front().AsString());
    first->second->addBus(bus);
    bus->addStation(first->second);
    for(auto it = stations.begin() + 1; it != stations.end(); it++) {
        auto stat = stations_.find(it->AsString());
        stat->second->addBus(bus);
        bus->addStation(stat->second);
        first = stat;
    }

    buses_.insert({ bus->getName(), bus });
}

TransportManager& TransportManager::setRoutingSettings(const std::map<string, Json::Node> &routingSettings)
{
    routingSettings_.busWait = routingSettings.at("bus_wait_time").AsInt();
    routingSettings_.busVelocity = routingSettings.at("bus_velocity").AsDouble() * 1000.0 / 60.0; // km/h => m/s

    updateRouter();

    return *this;
}

TransportManager& TransportManager::setRenderSettings(const std::map<string, Json::Node> &renderSettings)
{
    renderSettings_.width = renderSettings.at("width").AsDouble();
    renderSettings_.height = renderSettings.at("height").AsDouble();
    renderSettings_.padding = renderSettings.at("padding").AsDouble();
    renderSettings_.lineWidth = renderSettings.at("line_width").AsDouble();
    renderSettings_.stopRadius = renderSettings.at("stop_radius").AsDouble();
    renderSettings_.stopLblFontSize = renderSettings.at("stop_label_font_size").AsInt();
    renderSettings_.stopLblOffset = { renderSettings.at("stop_label_offset").AsArray()[0].AsDouble(),
                                      renderSettings.at("stop_label_offset").AsArray()[1].AsDouble() };
    renderSettings_.busLblFontSize = renderSettings.at("bus_label_font_size").AsInt();
    renderSettings_.busLblOffset = { renderSettings.at("bus_label_offset").AsArray()[0].AsDouble(),
                                     renderSettings.at("bus_label_offset").AsArray()[1].AsDouble() };

    for(const auto& color : renderSettings.at("color_palette").AsArray())
        if(color.IsArray())
            renderSettings_.colorPalette.emplace_back(color.AsArray());
        else
            renderSettings_.colorPalette.emplace_back(color.AsString());

    renderSettings_.underlayerWidth = renderSettings.at("underlayer_width").AsDouble();

    auto underlayedColor = renderSettings.at("underlayer_color");
    if(underlayedColor.IsArray())
        renderSettings_.underlayerColor = { underlayedColor.AsArray() };
    else
        renderSettings_.underlayerColor = { underlayedColor.AsString() };

    for(const auto& layer: renderSettings.at("layers").AsArray())
        renderSettings_.renderOrder.push_back(layer.AsString());

    updateZoomCoef();

    return *this;
}

void TransportManager::addStation(string name, double latitude, double longitude, const map<string, Json::Node>& distances)
{
    auto station = make_shared<BusStation>(move(name), move(latitude), move(longitude));
    for(auto & x: distances)
        station->addDistance(x.first, x.second.AsInt());

    auto newStation = stations_.insert({station->getName(), station}).first->second;
    newStation->setMainVertex((stations_.size() - 1) * 2);
    newStation->setWaitVertex((stations_.size() - 1) * 2 + 1);
}

void TransportManager::updateZoomCoef()
{
    double coefLat = maxLatitude_.value() - minLatitude_.value();
    double coefLon = maxLongitude_.value() - minLongitude_.value();

    if(coefLat == 0 && coefLon == 0)
    {
        zoomCoef_ = 0.0;
    }
    else if(coefLon == 0)
    {
        zoomCoef_ = (renderSettings_.height - 2 * renderSettings_.padding) / coefLat;
    }
    else if(coefLat == 0)
    {
        zoomCoef_ = (renderSettings_.width - 2 * renderSettings_.padding) / coefLon;
    } else
    {
        double heightCoef = (renderSettings_.height - 2 * renderSettings_.padding) / coefLat;
        double widthCoef = (renderSettings_.width - 2 * renderSettings_.padding) / coefLon;
        zoomCoef_ = min(heightCoef, widthCoef);
    }
}

const Svg::Document &TransportManager::getMap()
{
    if(map_.has_value())
        return map_.value();

    map_.emplace();

    for(const auto& layer: renderSettings_.renderOrder)
        (this->*renderFuncs_.at(layer))();

    return map_.value();
}

void TransportManager::renderBusLines()
{
    auto colorIt = renderSettings_.colorPalette.begin();
    for(const auto& bus: buses_)
    {
        if(colorIt == renderSettings_.colorPalette.end())
            colorIt = renderSettings_.colorPalette.begin();
        Svg::Polyline line;
        line.SetStrokeColor(*colorIt++)
            .SetStrokeWidth(renderSettings_.lineWidth)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round");
        for(const auto& station: bus.second->getStations())
        {
            auto latitude = station->getLatitude();
            auto longitude = station->getLongitude();

            line.AddPoint(createPoint(latitude, longitude));

        }
        if(!bus.second->isLooped())
            for(auto station = bus.second->getStations().rbegin() + 1; station != bus.second->getStations().rend(); station++)
            {
                auto latitude = (*station)->getLatitude();
                auto longitude = (*station)->getLongitude();

                line.AddPoint(createPoint(latitude, longitude));

            }
        map_->Add(move(line));
    }
}

void TransportManager::renderBusLabels()
{
    auto colorIt = renderSettings_.colorPalette.begin();
    auto addText = [this](const auto& bus, const auto& station,
                          const auto& color) {
        map_->Add(Svg::Text{}.SetPoint(createPoint(
                                           station->getLatitude(),
                                           station->getLongitude())
                                       )
                  .SetData(bus.second->getName())
                  .SetOffset(renderSettings_.busLblOffset)
                  .SetFontSize(renderSettings_.busLblFontSize)
                  .SetFontFamily("Verdana")
                  .SetFontWeight("bold")
                  .SetFillColor(renderSettings_.underlayerColor)
                  .SetStrokeColor(renderSettings_.underlayerColor)
                  .SetStrokeWidth(renderSettings_.underlayerWidth)
                  .SetStrokeLineCap("round")
                  .SetStrokeLineJoin("round"));

        map_->Add(Svg::Text{}.SetPoint(createPoint(
                                           station->getLatitude(),
                                           station->getLongitude())
                                       )
                  .SetData(bus.second->getName())
                  .SetOffset(renderSettings_.busLblOffset)
                  .SetFontSize(renderSettings_.busLblFontSize)
                  .SetFontFamily("Verdana")
                  .SetFontWeight("bold")
                  .SetFillColor(color));

    };

    for(const auto& bus: buses_)
    {
        if(colorIt == renderSettings_.colorPalette.end())
            colorIt = renderSettings_.colorPalette.begin();
        const auto& stations = bus.second->getStations();

        addText(bus, stations.front(), *colorIt);

        if(!bus.second->isLooped() &&
           stations.back()->getName() != stations.front()->getName())
            addText(bus, stations.back(), *colorIt);

        ++colorIt;
    }
}

void TransportManager::renderStationPoints()
{
    for(const auto& station: stations_)
    {
        Svg::Circle stationCircle;

        auto latitude = station.second->getLatitude();
        auto longitude = station.second->getLongitude();

        stationCircle.SetCenter(createPoint(latitude, longitude))
        .SetRadius(renderSettings_.stopRadius)
        .SetFillColor("white");

        map_->Add(move(stationCircle));
    }
}

void TransportManager::renderStationLabels()
{
    for(const auto& station: stations_)
    {
        Svg::Text text, textShadow;

        auto latitude = station.second->getLatitude();
        auto longitude = station.second->getLongitude();

        text.SetPoint(createPoint(latitude, longitude))
            .SetOffset(renderSettings_.stopLblOffset)
            .SetFontSize(renderSettings_.stopLblFontSize)
            .SetFontFamily("Verdana")
            .SetData(station.second->getName());

        textShadow = text;

        text.SetFillColor("black");

        textShadow.SetFillColor(renderSettings_.underlayerColor)
                  .SetStrokeColor(renderSettings_.underlayerColor)
                  .SetStrokeWidth(renderSettings_.underlayerWidth)
                  .SetStrokeLineCap("round")
                  .SetStrokeLineJoin("round");

        map_->Add(move(textShadow));
        map_->Add(move(text));
    }
}

bool TransportManager::performStopQuery(
            const std::map<string, Json::Node> &query,
            Json::JsonArray<Json::JsonBase> &arr
        )
{
    if(auto it = stations_.find(query.at("name").AsString()); it != stations_.end())
        return it->second->printInJson(query.at("id").AsInt(), arr), true;
    else
        return false;
}

bool TransportManager::performBusQuery(
            const std::map<string, Json::Node> &query,
            Json::JsonArray<Json::JsonBase> &arr
        )
{
    if(auto it = buses_.find(query.at("name").AsString()); it != buses_.end())
        return it->second->printInJson(query.at("id").AsInt(), arr), true;
    else
        return false;
}

bool TransportManager::performRouteQuery(
            const std::map<string, Json::Node> &query,
            Json::JsonArray<Json::JsonBase> &arr
        )
{
    auto from = stations_.at(query.at("from").AsString())->getWaitVertex();
    auto to = stations_.at(query.at("to").AsString())->getWaitVertex();

    if(!router_)
        return false;

    auto info = router_->BuildRoute(from, to);

    if(!info.has_value())
        return false;

    auto& stationsArr = arr.BeginObject()
        .Key("total_time").Double(info->weight.getTime())
        .Key("request_id").Integer(query.at("id").AsInt())
        .Key("items").BeginArray();

    for(size_t i = 0; i < info->edge_count; i++)
        graph_->GetEdge(router_->GetRouteEdge(info->id, i)).weight.printInJson(stationsArr);

    stationsArr.EndArray().EndObject();

    return true;
}

bool TransportManager::performMapQuery(
            const std::map<string, Json::Node> &query,
            Json::JsonArray<Json::JsonBase>& arr
        )
{
    stringstream svgMap;
    getMap().Render(svgMap, true);
    mapString_ = svgMap.str();
    arr.BeginObject()
         .Key("request_id").Integer(query.at("id").AsInt())
         .Key("map").String(mapString_)
         .EndObject();

    return true;
}

template <typename JsonObject>
void print_error(JsonObject& obj, int req_id, string_view message)
{
    obj.BeginObject()
       .Key("request_id").Integer(req_id)
       .Key("error_message").String(message)
       .EndObject();
}


void TransportManager::performQueries(const std::vector<Json::Node>& statRequests, ostream& stream)
{
    auto array = Json::PrintJsonArray(stream);

    for(auto& req : statRequests)
    {
        const auto & map = req.AsMap();
        string type = map.at("type").AsString();
        if(!(this->*performers_.at(type))(map, array))
            print_error(array, map.at("request_id").AsInt(), "not_found");
    }
}
