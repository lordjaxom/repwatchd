#ifndef REPWATCHD_MQTT_SESSION_HPP
#define REPWATCHD_MQTT_SESSION_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <3dprnet/core/string_view.hpp>

#include "types.hpp"

namespace repw {
namespace mqtt {

class Session
{
public:
    using MessageHandler = std::function< void ( prnet::string_view payload ) >;

private:
    class SessionImpl;

public:
    explicit Session( Endpoint const& endpoint );
    Session( Session const& ) = delete;
    ~Session();

    void publish( std::string const& topic, prnet::string_view payload );
    void subscribe( std::string topic, MessageHandler handler );

private:
    std::unique_ptr< SessionImpl > impl_;
};

} // namespace mqtt
} // namespace repw

#endif // REPWATCHD_MQTT_SESSION_HPP
