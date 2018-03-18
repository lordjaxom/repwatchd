#ifndef REPWATCHD_MQTT_TYPES_HPP
#define REPWATCHD_MQTT_TYPES_HPP

#include <cstdint>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace repw {
namespace mqtt {

class Endpoint
{
    friend void from_json( nlohmann::json const& src, Endpoint& dst );

public:
    Endpoint();
    Endpoint( std::string clientId, std::string host, std::uint16_t port );

    std::string const& clientId() const { return clientId_; }
    std::string const& host() const { return host_; }
    std::uint16_t port() const { return port_; }

private:
    std::string clientId_;
    std::string host_;
    std::uint16_t port_ {};
};

} // namespace mqtt
} // namespace repw

#endif // REPWATCHD_MQTT_TYPES_HPP
