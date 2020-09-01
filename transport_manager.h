#ifndef TRANSPORTMANAGER_H
#define TRANSPORTMANAGER_H

#include <string_view>
#include <memory>
#include <vector>

#include "json.h"
#include "graph.h"
#include "svg.h"

class BusStation;
class PathItem;
class Bus;


class TransportManager
{
    using GraphType = Graph::DirectedWeightedGraph<PathItem>;
    using RouterType = Graph::Router<PathItem>;

    std::map<std::string_view, std::shared_ptr<BusStation>> stations_;
    std::map<std::string_view, std::shared_ptr<Bus>> buses_;
    std::unordered_map<std::string, std::function<void(void)>> renderFuncs_;

    std::unique_ptr<GraphType> graph_;
    std::unique_ptr<RouterType> router_;

    std::optional<double> maxLatitude_, minLatitude_, maxLongitude_, minLongitude_, zoomCoef_;
    std::optional<Svg::Document> map_;

    struct RoutingSettings
    {
        size_t busWait = 0;
        double busVelocity = 0.0;
    } routingSettings_;

    struct RenderSettings
    {
        double width = 1.0;
        double height = 1.0;
        double padding = 0.0;
        double lineWidth = 1.0;
        double stopRadius = 1.0;
        Svg::Color underlayerColor;
        size_t stopLblFontSize = 1;
        size_t busLblFontSize = 1;
        double underlayerWidth = 1.0;
        Svg::Point busLblOffset = { 0.0, 0.0 };
        Svg::Point stopLblOffset = { 0.0, 0.0 };
        std::vector<Svg::Color> colorPalette;
        std::vector<std::string> renderOrder;
    } renderSettings_;

public:
    void performQueries(const std::vector<Json::Node> &statRequests, std::ostream &stream);
    void addBus(std::string name, std::vector<Json::Node> stations, bool isLooped);

    TransportManager& setRoutingSettings(const std::map<std::string, Json::Node>& routingSettings);
    TransportManager& setRenderSettings(const std::map<std::string, Json::Node>& renderSettings);

    static TransportManager& createInstance(const std::vector<Json::Node> &base_requests);
    void initRouter();

private:
    TransportManager(const std::vector<Json::Node> &base_requests);

    void addStation(std::string name, double latitude, double longitude,
                    const std::map<std::string, Json::Node> &distances);

    void updateZoomCoef();
    const Svg::Document& getMap();
    void renderBusLines();
    void renderBusLabels();
    void renderStationPoints();
    void renderStationLabels();

    Svg::Point createPoint(double latitude, double longitude) const;
};

#endif // TRANSPORTMANAGER_H
