#ifndef REPWATCHD_FHEM_SERVICE_HPP
#define REPWATCHD_FHEM_SERVICE_HPP

#include <functional>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

namespace repw {
namespace fhem {

class service
{
public:
    service( boost::asio::io_context& context, std::string host, std::string port );

    void setreading( std::string devspec, std::string reading, std::string value );

private:
    boost::asio::ip::tcp::socket create_socket( boost::asio::yield_context yield );

    template< typename Callback >
    auto checked_invoke( boost::asio::yield_context yield, Callback&& cb )
    {
        return std::forward< Callback >( cb )( create_socket() );
    }

    std::string request_csrf_token( boost::asio::yield_context yield );

    boost::asio::io_context& context_;
    std::string host_;
    std::string port_;
};


} // namespace fhem
} // namespace repw

#endif // REPWATCHD_FHEM_SERVICE_HPP
