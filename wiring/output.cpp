#include <mutex>

#include <3dprnet/core/logging.hpp>
#include <wiringPi.h>

#include "output.hpp"

using namespace std;
using namespace prnet;

namespace repw {
namespace wiring {

static Logger logger( "wiring::Output" );

class Output::OutputImpl
{
public:
    OutputImpl( int pin, bool invert )
            : pin_( pin )
            , invert_( invert )
    {
        call_once( initialized, [] { wiringPiSetup(); } );

        pinMode( pin_, OUTPUT );
        set( false, true );
    }

    bool get() const { return value_; }

    void set( bool value, bool force = false )
    {
        if ( value_ != value || force ) {
            logger.debug( "setting GPIO pin ", pin_, " to ", value ? "HIGH" : "LOW" );

            digitalWrite( pin_, value ^ invert_ ? HIGH : LOW );
            value_ = value;
        }
    }

private:
    static once_flag initialized;

    int pin_;
    bool invert_;
    bool value_ {};
};

once_flag Output::OutputImpl::initialized;

Output::Output( int pin, bool invert )
        : impl_( make_unique< OutputImpl >( pin, invert ) ){}

Output::Output( Output&& ) = default;
Output::~Output() = default;

bool Output::get() const
{
    return impl_->get();
}

void Output::set( bool value )
{
    impl_->set( value );
}

} // namespace wiring
} // namespace repw


