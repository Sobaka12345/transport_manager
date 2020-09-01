#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <vector>

namespace  {

template <typename It>
class Range {
public:
  using ValueType = typename std::iterator_traits<It>::value_type;

  Range(It begin, It end) : begin_(begin), end_(end) {}
  It begin() const { return begin_; }
  It end() const { return end_; }

private:
  It begin_;
  It end_;
};

}

namespace Graph {

  using VertexId = size_t;
  using EdgeId = size_t;

  template <typename Weight>
  struct Edge {
    VertexId from;
    VertexId to;
    Weight weight;
  };

  template <typename Weight>
  class DirectedWeightedGraph {
  private:
    using IncidenceList = std::vector<EdgeId>;
    using IncidentEdgesRange = Range<typename IncidenceList::const_iterator>;

  public:
    DirectedWeightedGraph(size_t vertex_count);
    EdgeId AddEdge(const Edge<Weight>& edge);

    size_t GetVertexCount() const;
    size_t GetEdgeCount() const;
    const Edge<Weight>& GetEdge(EdgeId edge_id) const;
    IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

  private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
  };


  template <typename Weight>
  DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count) : incidence_lists_(vertex_count) {}

  template <typename Weight>
  EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
    edges_.push_back(edge);
    const EdgeId id = edges_.size() - 1;
    incidence_lists_[edge.from].push_back(id);
    return id;
  }

  template <typename Weight>
  size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
    return incidence_lists_.size();
  }

  template <typename Weight>
  size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
    return edges_.size();
  }

  template <typename Weight>
  const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
    return edges_[edge_id];
  }

  template <typename Weight>
  typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
  DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
    const auto& edges = incidence_lists_[vertex];
    return {std::begin(edges), std::end(edges)};
  }
}

namespace Graph {

  template <typename Weight>
  class Router {
  private:
    using Graph = DirectedWeightedGraph<Weight>;

  public:
    Router(const Graph& graph);

    using RouteId = uint64_t;

    struct RouteInfo {
      RouteId id;
      Weight weight;
      size_t edge_count;
    };

    std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;
    EdgeId GetRouteEdge(RouteId route_id, size_t edge_idx) const;
    void ReleaseRoute(RouteId route_id);

  private:
    const Graph& graph_;

    struct RouteInternalData {
      Weight weight;
      std::optional<EdgeId> prev_edge;
    };
    using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

    using ExpandedRoute = std::vector<EdgeId>;
    mutable RouteId next_route_id_ = 0;
    mutable std::unordered_map<RouteId, ExpandedRoute> expanded_routes_cache_;

    void InitializeRoutesInternalData(const Graph& graph) {
      const size_t vertex_count = graph.GetVertexCount();
      for (VertexId vertex = 0; vertex < vertex_count; ++vertex) {
        routes_internal_data_[vertex][vertex] = RouteInternalData{0, std::nullopt};
        for (const EdgeId edge_id : graph.GetIncidentEdges(vertex)) {
          const auto& edge = graph.GetEdge(edge_id);
          //assert(edge.weight.length >= 0);
          auto& route_internal_data = routes_internal_data_[vertex][edge.to];
          if (!route_internal_data || edge.weight < route_internal_data->weight) {
            route_internal_data = RouteInternalData{edge.weight, edge_id};
          }
        }
      }
    }

    void RelaxRoute(VertexId vertex_from, VertexId vertex_to,
                    const RouteInternalData& route_from, const RouteInternalData& route_to) {
      auto& route_relaxing = routes_internal_data_[vertex_from][vertex_to];
      const Weight candidate_weight = route_from.weight + route_to.weight;
      if (!route_relaxing || candidate_weight < route_relaxing->weight) {
        route_relaxing = {
            candidate_weight,
            route_to.prev_edge
                ? route_to.prev_edge
                : route_from.prev_edge
        };
      }
    }

    void RelaxRoutesInternalDataThroughVertex(size_t vertex_count, VertexId vertex_through) {
      for (VertexId vertex_from = 0; vertex_from < vertex_count; ++vertex_from) {
        if (const auto& route_from = routes_internal_data_[vertex_from][vertex_through]) {
          for (VertexId vertex_to = 0; vertex_to < vertex_count; ++vertex_to) {
            if (const auto& route_to = routes_internal_data_[vertex_through][vertex_to]) {
              RelaxRoute(vertex_from, vertex_to, *route_from, *route_to);
            }
          }
        }
      }
    }

    RoutesInternalData routes_internal_data_;
  };


  template <typename Weight>
  Router<Weight>::Router(const Graph& graph)
      : graph_(graph),
        routes_internal_data_(graph.GetVertexCount(), std::vector<std::optional<RouteInternalData>>(graph.GetVertexCount()))
  {
    InitializeRoutesInternalData(graph);

    const size_t vertex_count = graph.GetVertexCount();
    for (VertexId vertex_through = 0; vertex_through < vertex_count; ++vertex_through) {
      RelaxRoutesInternalDataThroughVertex(vertex_count, vertex_through);
    }
  }

  template <typename Weight>
  std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexId from, VertexId to) const {
    const auto& route_internal_data = routes_internal_data_[from][to];
    if (!route_internal_data) {
      return std::nullopt;
    }
    const Weight weight = route_internal_data->weight;
    std::vector<EdgeId> edges;
    for (std::optional<EdgeId> edge_id = route_internal_data->prev_edge;
         edge_id;
         edge_id = routes_internal_data_[from][graph_.GetEdge(*edge_id).from]->prev_edge) {
      edges.push_back(*edge_id);
    }
    std::reverse(std::begin(edges), std::end(edges));

    const RouteId route_id = next_route_id_++;
    const size_t route_edge_count = edges.size();
    expanded_routes_cache_[route_id] = std::move(edges);
    return RouteInfo{route_id, weight, route_edge_count};
  }

  template <typename Weight>
  EdgeId Router<Weight>::GetRouteEdge(RouteId route_id, size_t edge_idx) const {
    return expanded_routes_cache_.at(route_id)[edge_idx];
  }

  template <typename Weight>
  void Router<Weight>::ReleaseRoute(RouteId route_id) {
    expanded_routes_cache_.erase(route_id);
  }

}


#endif // GRAPH_H
