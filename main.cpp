#include <iostream>
//#include <fstream>

#include "transport_manager.h"



int main()
{
    //std::ofstream file("bibs.json");

    const Json::Document document(Json::Load(std::cin));

    const auto& base_requests = document.GetRoot().AsMap().at("base_requests").AsArray();
    const auto& render_settings = document.GetRoot().AsMap().at("render_settings").AsMap();
    const auto& routing_settings = document.GetRoot().AsMap().at("routing_settings").AsMap();


    TransportManager& manager = TransportManager::createInstance(base_requests)
                                .setRoutingSettings(routing_settings)
                                .setRenderSettings(render_settings);
    manager.initRouter();

    const auto& stat_requests = document.GetRoot().AsMap().at("stat_requests").AsArray();
    manager.performQueries(stat_requests, std::cout);

    return 0;
}
