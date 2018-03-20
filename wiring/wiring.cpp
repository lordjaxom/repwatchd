#include <mutex>

#include <wiringPi.h>

namespace repw {
namespace wiring {

namespace detail {

struct WiringSetup
{
    WiringSetup() noexcept { wiringPiSetup(); }
};

[[unused]] static WiringSetup wiringSetup;

} // namespace detail

} // namespace wiring
} // namespace repw