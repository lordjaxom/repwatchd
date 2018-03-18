#include <utility>

#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>

#include "types.hpp"

using namespace std;
using namespace nlohmann;

namespace repw {
namespace mqtt {

Endpoint::Endpoint() = default;

Endpoint::Endpoint( string clientId, string host, uint16_t port )
        : clientId_( move( clientId ) )
        , host_( move( host ) )
        , port_( port ) {}

void from_json( json const& src, Endpoint& dst )
{
    dst.clientId_ = src.at( "clientId" );
    dst.host_ = src.at( "host" );
    dst.port_ = boost::lexical_cast< uint16_t >( src.at( "port" ).get< string >() );
}

} // namespace mqtt
} // namespace repw


