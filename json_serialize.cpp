#include "json_serialize.hpp"

namespace Json {

void PrintJsonString(std::ostream& stream, std::string_view str)
{
    stream << quoted(str);
}

JsonArray<JsonBase> PrintJsonArray(std::ostream& stream)
{
    return JsonArray<JsonBase>(stream);
}

JsonObject<JsonBase> PrintJsonObject(std::ostream& stream)
{
    return JsonObject<JsonBase>(stream);
}

}
