//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CONSUMING_BUFFERS_HPP
#define BOOST_BEAST_CONSUMING_BUFFERS_HPP

#include <boost/beast/config.hpp>
#include <boost/beast/core/detail/in_place_init.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {

/** Adapter to trim the front of a `BufferSequence`.

    This adapter wraps a buffer sequence to create a new sequence
    which may be incrementally consumed. Bytes consumed are removed
    from the front of the buffer. The underlying memory is not changed,
    instead the adapter efficiently iterates through a subset of
    the buffers wrapped.

    The wrapped buffer is not modified, a copy is made instead.
    Ownership of the underlying memory is not transferred, the application
    is still responsible for managing its lifetime.

    @tparam BufferSequence The buffer sequence to wrap.
*/
template<class BufferSequence>
class consuming_buffers
{
    using buffers_type =
        typename std::decay<BufferSequence>::type;

    using iter_type =
        typename buffers_type::const_iterator;

    BufferSequence bs_;
    iter_type begin_;
    std::size_t skip_ = 0;

    template<class Deduced>
    consuming_buffers(Deduced&& other, std::size_t dist)
        : bs_(std::forward<Deduced>(other).bs_)
        , begin_(std::next(bs_.begin(), dist))
        , skip_(other.skip_)
    {
    }

public:
    /** The type for each element in the list of buffers.

        If the buffers in the underlying sequence are convertible to
        `boost::asio::mutable_buffer`, then this type will be
        `boost::asio::mutable_buffer`, else this type will be
        `boost::asio::const_buffer`.
    */
#if BOOST_BEAST_DOXYGEN
    using value_type = implementation_defined;
#else
    using value_type = typename std::conditional<
        std::is_convertible<typename
            std::iterator_traits<iter_type>::value_type,
                boost::asio::mutable_buffer>::value,
                    boost::asio::mutable_buffer,
                        boost::asio::const_buffer>::type;
#endif

#if BOOST_BEAST_DOXYGEN
    /// A bidirectional iterator type that may be used to read elements.
    using const_iterator = implementation_defined;

#else
    class const_iterator;

#endif

    /// Constructor
    consuming_buffers();

    /// Constructor
    consuming_buffers(consuming_buffers&&);

    /// Constructor
    consuming_buffers(consuming_buffers const&);

    /** Constructor

        A copy of the buffer sequence is made. Ownership of the
        underlying memory is not transferred or copied.
    */
    explicit
    consuming_buffers(BufferSequence const& buffers);

    /** Constructor

        This constructs the buffer sequence in-place from
        a list of arguments.

        @param args Arguments forwarded to the buffers constructor.
    */
    template<class... Args>
    consuming_buffers(boost::in_place_init_t, Args&&... args);

    /// Assignment
    consuming_buffers& operator=(consuming_buffers&&);

    /// Assignment
    consuming_buffers& operator=(consuming_buffers const&);

    /// Returns the underlying buffers, without modification
    BufferSequence const&
    get() const
    {
        return bs_;
    }

    /// Get a bidirectional iterator to the first element.
    const_iterator
    begin() const;

    /// Get a bidirectional iterator to one past the last element.
    const_iterator
    end() const;

    /** Remove bytes from the beginning of the sequence.

        @param amount The number of bytes to remove. If this is
        larger than the number of bytes remaining, all the
        bytes remaining are removed.
    */
    void
    consume(std::size_t amount);
};

} // beast
} // boost

#include <boost/beast/core/impl/consuming_buffers.ipp>

#endif
