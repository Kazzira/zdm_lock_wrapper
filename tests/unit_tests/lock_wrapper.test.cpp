#include <catch2/catch_all.hpp>
#include <ranges>
#include <thread>
#include <zdm/lock_wrapper.hpp>

namespace {

void
increment_value(
    int& value
)
{
    value += 1;
}

int
get_incremented_value(
    const int& value
)
{
    return value + 1;
}

} // namespace

TEST_CASE(
    "lock_wrapper with std::mutex - reference lambda",
    "[lock_wrapper]"
)
{
    zdm::lock_wrapper<int> wrapper( 42 );

    wrapper.with_lock(
        []( int& value )
        {
            value += 1;
        }
    );

    REQUIRE( *wrapper == 43 );
}

TEST_CASE(
    "lock_wrapper with std::mutex - const reference lambda",
    "[lock_wrapper]"
)
{
    zdm::lock_wrapper<int> wrapper( 42 );

    auto                   result = wrapper.with_lock(
        []( const int& value )
        {
            return value + 1;
        }
    );

    REQUIRE( result == 43 );
}

TEST_CASE(
    "lock_wrapper with std::mutex - function pointer",
    "[lock_wrapper]"
)
{
    zdm::lock_wrapper<int> wrapper( 42 );

    STATIC_REQUIRE( std::is_same_v<
                    std::tuple_element_t<
                        0,
                        zdm::detail::function_traits<
                            decltype( get_incremented_value )>::arg_types>,
                    const int&> );
    STATIC_REQUIRE(
        std::tuple_size_v<zdm::detail::function_traits<
            decltype( get_incremented_value )>::arg_types>
        == 1
    );
    wrapper.with_lock( increment_value );

    REQUIRE( *wrapper == 43 );
}

TEST_CASE(
    "lock_wrapper with std::mutex - const function reference",
    "[lock_wrapper]"
)
{
    zdm::lock_wrapper<int> wrapper( 42 );

    auto                   result = wrapper.with_lock( get_incremented_value );

    REQUIRE( result == 43 );
}

TEST_CASE(
    "lock_wrapper with std::mutex - const function reference",
    "[lock_wrapper]"
)
{
    zdm::lock_wrapper<int> wrapper( 42 );

    auto                   result = wrapper.with_lock( &get_incremented_value );

    REQUIRE( result == 43 );
}

TEST_CASE(
    "lock_wrapper - test with 4 threads",
    "[lock_wrapper]"
)
{
    constexpr size_t         number_of_threads = 4;
    zdm::lock_wrapper<int>   wrapper( 0 );

    std::vector<std::thread> threads;
    threads.reserve( number_of_threads );

    auto increment_wrapper = [&wrapper]( [[maybe_unused]]
                                         auto )
    {
        wrapper.with_lock(
            []( int& value )
            {
                ++value;
            }
        );
    };

    for( size_t i = 0; i < number_of_threads; ++i )
    {
        threads.emplace_back(
            [increment_wrapper]()
            {
                std::ranges::
                    for_each( std::views::iota( 0, 25 ), increment_wrapper );
            }
        );
    }

    for( auto& thread : threads )
    {
        thread.join();
    }

    REQUIRE( *wrapper == 100 );
}
