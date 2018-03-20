#ifndef REPWATCHD_WIRING_OUTPUT_HPP
#define REPWATCHD_WIRING_OUTPUT_HPP

#include <memory>

namespace repw {
namespace wiring {

class Output
{
    class OutputImpl;

public:
    Output( Output const& ) = delete;
    Output( Output&& ) noexcept;
    ~Output();

    bool get() const;

    void set( bool value = true );
    void clear() { set( false ); }

private:
    Output( int pin, bool invert );

    std::unique_ptr< OutputImpl > impl_;
};

} // namespace wiring
} // namespace repw

#endif // REPWATCHD_WIRING_OUTPUT_HPP
