#include "fhem_service.hpp"

#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <core/logging.hpp>

using namespace std;
using namespace prnet;

using tcp = boost::asio::ip::tcp;

namespace asio = boost::asio;
namespace http = boost::beast::http;

namespace repw {
namespace fhem {

static logger logger( "fhem::service" );

service::service( asio::io_context &context, string host, string port )
        : context_( context )
        , host_( move( host ) )
        , port_( move( port ) )
{
}

void service::setreading( string devspec, string reading, string value )
{
    checked_spawn( [this, devspec { move( devspec ) }, reading { move( reading ) }, value { move( value ) }]( auto yield ) {
		logger.debug( ">>> setreading ", devspec, " ", reading, " ", value );

        auto target { "/fhem?fwcsrf=" + this->request_csrf_token( yield ) + "&cmd=setreading%20" + devspec + "%20" + reading + "%20" + value };

        auto socket { this->create_socket( yield ) };

        http::request< http::empty_body > request { http::verb::get, target, 11 };
        request.set( http::field::host, host_ );
        request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
        http::async_write( socket, request, yield );

        boost::beast::multi_buffer buffer;
        http::response< http::dynamic_body > response;
        http::async_read( socket, buffer, response, yield );
    } );
}

template< typename Func >
void service::checked_spawn( Func&& func )
{
    asio::spawn( context_, [this, func { move( func ) }]( auto yield ) mutable {
        try {
            func( yield );
        } catch ( system_error const& e ) {
            logger.error( "error communicating with fhem: ", e.what() );
        } catch ( boost::beast::system_error const& e ) {
            logger.error( "error communicating with fhem: ", e.what() );
        }
    } );
}

tcp::socket service::create_socket( asio::yield_context yield )
{
    tcp::resolver resolver { context_ };
    auto resolved { resolver.async_resolve( host_, port_, yield ) };

    tcp::socket socket { context_ };
    asio::async_connect( socket, resolved, yield );

    return move( socket );
}

string service::request_csrf_token( asio::yield_context yield ) {
    auto socket { create_socket( yield ) };

    http::request< http::empty_body > request { http::verb::get, "/fhem?xhr=1", 11 };
    request.set( http::field::host, host_ );
    request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
    http::async_write( socket, request, yield );

    boost::beast::multi_buffer buffer;
    http::response< http::dynamic_body > response;
    http::async_read( socket, buffer, response, yield );

    return string { response[ "X-FHEM-csrfToken" ] };
}

} // namespace fhem
} // namespace repw
