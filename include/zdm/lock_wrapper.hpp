#pragma once
/*
MIT License

Copyright (c) 2025 Zachary D Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <concepts>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <type_traits>

namespace zdm::detail {

template <typename F>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<R ( & )( Args... )>
{
        using return_type = R;
        using arg_types   = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct function_traits<R ( * )( Args... )>
{
        using return_type = R;
        using arg_types   = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct function_traits<R( Args... )>
{
        using return_type = R;
        using arg_types   = std::tuple<Args...>;
};

template <typename R, typename Class, typename... Args>
struct function_traits<R ( Class::* )( Args... ) const>
{
        using return_type = R;
        using arg_types   = std::tuple<Args...>;
};

} // namespace zdm::detail

namespace zdm::concepts {

/**
 * @brief Concept for a lockable type such as a mutex.
 */
template <class ALockable>
concept lockable = requires( ALockable &a_lockable ) {
    { a_lockable.lock() } -> std::same_as<void>;
    { a_lockable.unlock() } -> std::same_as<void>;
};

template <class AFunction, class T>
concept unary_reference_function = requires {
    requires std::tuple_size_v<
                 typename zdm::detail::function_traits<AFunction>::arg_types>
                 == 1;
    requires std::same_as<
        typename std::tuple_element_t<
            0,
            typename zdm::detail::function_traits<AFunction>::arg_types>,
        T &>;
} || requires {
    requires std::tuple_size_v<typename zdm::detail::function_traits<
                 decltype( &AFunction::operator() )>::arg_types>
                 == 1;
    requires std::same_as<
        typename std::tuple_element_t<
            0,
            typename zdm::detail::function_traits<
                decltype( &AFunction::operator() )>::arg_types>,
        T &>;
};

template <class AFunction, class T>
concept unary_const_reference_function = requires {
    requires std::tuple_size_v<
                 typename zdm::detail::function_traits<AFunction>::arg_types>
                 == 1;
    requires std::same_as<
        typename std::tuple_element_t<
            0,
            typename zdm::detail::function_traits<AFunction>::arg_types>,
        const T &>;
} || requires {
    requires std::tuple_size_v<typename zdm::detail::function_traits<
                 decltype( &AFunction::operator() )>::arg_types>
                 == 1;
    requires std::same_as<
        typename std::tuple_element_t<
            0,
            typename zdm::detail::function_traits<
                decltype( &AFunction::operator() )>::arg_types>,
        const T &>;
};

} // namespace zdm::concepts

namespace zdm {

/**
 * @brief Traits for mutex types.
 *
 * This is used for `zdm::lock_wrapper`. If you wish to use a custom mutex
 * type, then specialize this template and define the following aliases:
 * - `mutex_type`: The type of mutex to use, such as `std::mutex`.
 * - `unique_lock`: The type of unique lock to use, such as
 * `std::scoped_lock<mutex_type>` or `std::unique_lock<mutex_type>`.
 * - `shared_lock`: The type of shared lock to use, such as
 * `std::scoped_lock<mutex_type>` or `std::shared_lock<mutex_type>`.
 *
 * `unique_lock` and `shared_lock` are used for the `with_lock` function.
 * `with_lock` has overloads for reference and const reference functions,
 *  so use the appropriate lock type for your use case.
 */
template <zdm::concepts::lockable ALockable>
struct mutex_traits;

template <>
struct mutex_traits<std::mutex>
{
        using mutex_type  = std::mutex;
        using unique_lock = std::scoped_lock<mutex_type>;
        using shared_lock = std::scoped_lock<mutex_type>;
};

template <>
struct mutex_traits<std::shared_mutex>
{
        using mutex_type  = std::shared_mutex;
        using unique_lock = std::unique_lock<mutex_type>;
        using shared_lock = std::shared_lock<mutex_type>;
};

template <>
struct mutex_traits<std::recursive_mutex>
{
        using mutex_type  = std::recursive_mutex;
        using unique_lock = std::scoped_lock<mutex_type>;
        using shared_lock = std::scoped_lock<mutex_type>;
};

template <class AContainedType, zdm::concepts::lockable AMutexType>
class basic_lock_wrapper
{
    public:
        basic_lock_wrapper() = default;

        explicit basic_lock_wrapper(
            AContainedType &&a_contained
        )
            : m_contained( std::forward<AContainedType>( a_contained ) )
        {
        }

        AContainedType &
        operator*() noexcept
        {
            return m_contained;
        }

        const AContainedType &
        operator*() const noexcept
        {
            return m_contained;
        }

        AContainedType *
        operator->() noexcept
        {
            return &m_contained;
        }

        const AContainedType *
        operator->() const noexcept
        {
            return &m_contained;
        }

        /**
         * @brief Executes a function with a lock on the contained object.
         *
         * @tparam AFunction Any callable that takes a reference to the
         * contained object.
         * @param a_function A callable that takes a reference to the contained
         * object.
         * @return The result of the function.
         */
        inline auto
        with_lock(
            concepts::unary_reference_function<AContainedType> auto &&a_function
        ) -> decltype( a_function( std::declval<AContainedType &>() ) )
        {
            typename mutex_traits<AMutexType>::unique_lock lock( m_mutex );

            if constexpr( std::is_void_v<decltype( a_function( m_contained )
                          )> )
            {
                a_function( m_contained );
                return;
            }
            else
            {
                return a_function( m_contained );
            }
        }

        /**
         * @brief Executes a function with a lock on the contained object.
         *
         * @tparam AFunction Any callable that takes a const reference to the
         * contained object.
         * @param a_function A callable that takes a const reference to the
         * contained object.
         * @return The result of the function.
         */
        inline auto
        with_lock(
            concepts::unary_const_reference_function<AContainedType> auto
                &&a_function
        ) const
            -> decltype( a_function( std::declval<const AContainedType &>() ) )
        {
            typename mutex_traits<AMutexType>::shared_lock lock( m_mutex );

            if constexpr( std::is_void_v<decltype( a_function( m_contained )
                          )> )
            {
                a_function( m_contained );
                return;
            }
            else
            {
                return a_function( m_contained );
            }
        }

    private:
        mutable typename mutex_traits<AMutexType>::mutex_type m_mutex;
        AContainedType                                        m_contained;
};

template <class T>
using lock_wrapper = basic_lock_wrapper<T, std::mutex>;

template <class T>
using shared_lock_wrapper = basic_lock_wrapper<T, std::shared_mutex>;

template <class T>
using recursive_lock_wrapper = basic_lock_wrapper<T, std::recursive_mutex>;

} // namespace zdm
