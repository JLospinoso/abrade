//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_DETAIL_CONFIG_HPP
#define BOOST_BEAST_CORE_DETAIL_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION >= 106500 || ! defined(BOOST_GCC) || BOOST_GCC < 70000
# define BOOST_BEAST_FALLTHROUGH BOOST_FALLTHROUGH
#else
# define BOOST_BEAST_FALLTHROUGH __attribute__((fallthrough))
#endif

#endif
