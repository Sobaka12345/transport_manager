#ifndef JSON_H
#define JSON_H

#include <variant>
#include <functional>
#include <string>
#include <vector>
#include <map>

namespace Json {

  class Node : std::variant<std::vector<Node>,
                            std::map<std::string, Node>,
                            int,
                            double,
                            bool,
                            std::string> {
  public:
    using variant::variant;

    bool IsArray() const {
        return std::holds_alternative<std::vector<Node>>(*this);
    }

    bool IsMap() const {
        return std::holds_alternative<std::map<std::string, Node>>(*this);
    }

    bool IsString() const {
        return std::holds_alternative<std::string>(*this);
    }

    const auto& AsArray() const {
      return std::get<std::vector<Node>>(*this);
    }
    const auto& AsMap() const {
      return std::get<std::map<std::string, Node>>(*this);
    }

    double AsDouble() const {
        if(!std::holds_alternative<double>(*this))
            return double(std::get<int>(*this));
        return std::get<double>(*this);
    }

    bool AsBool() const {
        return std::get<bool>(*this);
    }

    int AsInt() const {
      return std::get<int>(*this);
    }

    const auto& AsString() const {
      return std::get<std::string>(*this);
    }
  };

  class Document {
  public:
    explicit Document(Node root);

    const Node& GetRoot() const;

  private:
    Node root;
  };

  Document Load(std::istream& input);

}

#endif // JSON_H
