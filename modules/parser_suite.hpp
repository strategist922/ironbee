/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee++ --- ParserSuite
 *
 * Simple HTTP parsers.
 *
 * XXX
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __IBPP__PARSER_SUITE__
#define __IBPP__PARSER_SUITE__

#include <boost/exception/all.hpp>
#include <boost/range/iterator_range.hpp>

#include <vector>

namespace IronBee {

/**
 * Collection of pure HTTP parsers.
 *
 * This namespace defines a variety of HTTP parsers.  All parsers are pure:
 * they store no state and have no side effects.  Every parser parses
 * an input represented as a range of bytes (@ref span_t) and returns a
 * structure with parser results.  It also  modifies the input span to
 * represent the remainder of the data.  E.g.,  calling parse_request_line()
 * on a span representing an entire HTTP request will modify the input to
 * begin just after the request line.  Parsers that require additional
 * context will have additional parameters.
 *
 * Parse results are usually also represented by spans (@ref span_t).  Parsers
 * assume that the underlying memory of the input span will outlive the
 * results.  I.e., freeing the buffer being parsed will invalidate the
 * results.
 *
 * These parsers are intentionally minimal.  For example, parse_request_line()
 * will provide the URI as a result but does not attempt to parse the URI;
 * the URI can be parsed by parse_uri().  This approach has the advantage of
 * code simplicity and not performing unneeded work, but has the disadvantage
 * that deeper parsing require multiple passes.
 *
 * All results are returned by move and support ostream output.  They provide
 * direct results (usually subspans) as public data members and may provide
 * function members for additional logic, e.g.,
 * parse_request_line_result_t::http09().
 **/
namespace ParserSuite {

/**
 * Exception base for all ParserSuite errors.
 **/
struct error : public boost::exception, public std::exception {};

/**
 * Location where error occurred
 **/
using errinfo_location =
    boost::error_info<struct tag_errinfo_location, const char *>;

/**
 * Error message.
 **/
using errinfo_what =
    boost::error_info<struct tag_errinfo_what, std::string>;

/**
 * A span of bytes.
 **/
using span_t = boost::iterator_range<const char*>;

/**
 * A sequence of @ref span_t.
 **/
using span_vec_t = std::vector<span_t>;

//! Result of parse_request_line()
struct parse_request_line_result_t
{
    //! Method.  First of space separated list.
    span_t method;
    //! URI.  Second of space separated list.
    span_t uri;
    //! Version.  Third of space separated list.
    span_t version;

    //! Is this a HTTP 0.9 request, i.e., is @ref version empty?
    bool http09() const
    {
        return version.empty();
    }
};

/**
 * Parse @a input as a request line.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the next line.
 * @return Result.
 * @throw error On any parse error; e.g., less than two items.
 **/
parse_request_line_result_t parse_request_line(span_t& input);

//! Ostream output operator for @ref parse_request_line_result_t.
std::ostream& operator<<(
    std::ostream&                      o,
    const parse_request_line_result_t& R
);

//! Result of parse_response_line()
struct parse_response_line_result_t
{
    //! Version.  First of space separated list.
    span_t version;
    //! Status.  Second of space separated list.
    span_t status;
    //! Message.  Remainder of space separated list.
    span_t message;
};

/**
 * Parse @a input as a response line.
 *
 * @note Early versions of HTTP do not have response lines.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the next line.
 * @return Result.
 * @throw error On any parse error; e.g., less than two items.
 **/
parse_response_line_result_t parse_response_line(span_t& input);

//! Ostream output operator for @ref parse_response_line_result_t.
std::ostream& operator<<(
    std::ostream&                      o,
    const parse_response_line_result_t& R
);

//! Result of parse_uri().
struct parse_uri_result_t
{
    //! Scheme.  Item before first :.
    span_t scheme;
    //! Authority.  Item between // and next / after scheme.
    span_t authority;
    //! Path.  Item after authority (if present) until ?.
    span_t path;
    //! Query.  Item after ? until #.
    span_t query;
    //! Fragment.  Item after #.
    span_t fragment;
};

/**
 * Parse @a input as a URI.
 *
 * @note Will handle schemeless and authorityless URIs.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the first
 *                       whitespace.
 * @return Result.
 * @throw error On any parse error; should not happen.
 **/
parse_uri_result_t parse_uri(span_t& input);

//! Ostream output operator for @ref parse_uri_result_t.
std::ostream& operator<<(
    std::ostream&             o,
    const parse_uri_result_t& R
);

//! Result of parse_headers().
struct parse_headers_result_t
{
    //! A single header.
    struct header_t {
        //! Key.
        span_t key;
        //! Value as sequence of spans: one per line (extended headers).
        span_vec_t value;
    };
    //! Type of @ref headers.
    using headers_t = std::vector<header_t>;

    //! All headers.
    headers_t headers;

    //! True iff a blank line was present after headers.
    bool terminated = false;
};

/**
 * Parse @a input as headers block.
 *
 * @note: Does not require final blank line.  See
 *        parse_headers_result_t::terminated.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the beginning
 *                       of the body.
 * @return Result.
 * @throw error On any parse error; e.g., no key.
 **/
parse_headers_result_t parse_headers(span_t& input);

//! Ostream output operator for @ref parse_headers_result_t
std::ostream& operator<<(
    std::ostream&                 o,
    const parse_headers_result_t& R
);

//! Result of parse_request().
struct parse_request_result_t
{
    //! The request line.
    span_t                      raw_request_line;
    //! The request line as parsed by parse_request_line().
    parse_request_line_result_t request_line;
    //! The URI as parsed by parse_ur().
    parse_uri_result_t          uri;
    //! The headers as parsed by parse_headers().
    parse_headers_result_t      headers;
};

/**
 * Parse @a input as request.
 *
 * @sa parse_request_line()
 * @sa parse_uri()
 * @sa parse_headers()
 *
 * @note Currently does not parse body.  Stay tuned.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the beginning
 *                       of the body.
 * @return Result.
 * @throw error On any parse error.
 **/
parse_request_result_t parse_request(span_t& input);

//! Ostream output operator for @ref parse_request_result_t.
std::ostream& operator<<(
    std::ostream&                 o,
    const parse_request_result_t& R
);

//! Result of parse_response().
struct parse_response_result_t
{
    //! The response line.
    span_t                       raw_response_line;
    //! The reponse line as parsed by parse_response_line().
    parse_response_line_result_t response_line;
    //! The headers as parsed by parse_headers().
    parse_headers_result_t       headers;
};

/**
 * Parse @a input as response.
 *
 * @sa parse_response_line()
 * @sa parse_headers()
 *
 * @note Currently does not parse body.  Stay tuned.
 *
 * @param[in, out] input Span to parse; will be updated such that beginning
 *                       is just after successful parse, i.e., the beginning
 *                       of the body.
 * @return Result.
 * @throw error On any parse error.
 **/
parse_response_result_t parse_response(span_t& input);

//! Ostream output operator for @ref parse_response_result_t
std::ostream& operator<<(
    std::ostream&                  o,
    const parse_response_result_t& R
);

} // ParserSuite
} // IronBee

#endif
