#include "parser_suite.hpp"

#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>

using namespace std;

// Tell Fusion about the result structures.
// These must be in the top namespace.

BOOST_FUSION_ADAPT_STRUCT(
    ::IronBee::ParserSuite::parse_request_line_result_t,
    (::IronBee::ParserSuite::span_t, method)
    (::IronBee::ParserSuite::span_t, uri)
    (::IronBee::ParserSuite::span_t, version)
)

BOOST_FUSION_ADAPT_STRUCT(
    ::IronBee::ParserSuite::parse_response_line_result_t,
    (::IronBee::ParserSuite::span_t, version)
    (::IronBee::ParserSuite::span_t, status)
    (::IronBee::ParserSuite::span_t, message)
)

BOOST_FUSION_ADAPT_STRUCT(
    ::IronBee::ParserSuite::parse_uri_result_t,
    (::IronBee::ParserSuite::span_t, scheme)
    (::IronBee::ParserSuite::span_t, authority)
    (::IronBee::ParserSuite::span_t, path)
    (::IronBee::ParserSuite::span_t, query)
    (::IronBee::ParserSuite::span_t, fragment)
)

namespace IronBee {
namespace ParserSuite {

namespace  {

//! Parser matching horizontal whitespace only.
const static auto sp = boost::spirit::ascii::char_(" \t");

}

ostream& operator<<(
    ostream&                      o,
    const parse_headers_result_t& R
)
{
    for (const auto& header : R.headers) {
        bool first = true;
        o << header.key << "=";
        for (const auto& v : header.value) {
            if (! first) {
                o << " ";
            }
            first = false;
            o << v;
        }
        o << endl;
    }

    o << "terminated=" << (R.terminated ? "true" : "false") << endl;

    return o;
}

parse_headers_result_t parse_headers(span_t& input)
{
    using namespace boost::spirit::qi;
    using ascii::char_;

    parse_headers_result_t R;

    auto begin = input.begin();
    auto end   = input.end();

    auto push_value = [&](const span_t& value)
    {
        R.headers.back().value.push_back(value);
    };

    auto new_header = [&](const span_t& key)
    {
        R.headers.push_back(parse_headers_result_t::header_t {key, {}});
    };

    static const auto value = raw[+(char_-char_("\r\n"))];
    static const auto key = raw[*(char_-char_(" :\r\n"))] >> omit[lit(":")];
    static const auto header =
        (
              key                           [new_header]
           >> *sp
           >> value                         [push_value]
           >> (eol|eoi)
        ) |
        (
              +char_(" ")
           >> value                        [push_value]
           >> (eol|eoi)
        )
        ;
    static const auto terminator = *sp >> eol;

    static const auto grammar =
           +header
        >> -terminator [([&]() {R.terminated = true;})]
        ;

    auto success = parse(begin, end, grammar);
    // Note: begin updated.

    if (! success) {
        BOOST_THROW_EXCEPTION(error()
            << errinfo_what("Incomplete headers.")
            << errinfo_location(begin)
        );
    }

    input = span_t(begin, end);
    return move(R);
}

parse_request_line_result_t parse_request_line(span_t& input)
{
    using namespace boost::spirit::qi;
    using ascii::char_;

    parse_request_line_result_t R;

    auto begin = input.begin();
    auto end   = input.end();

    static const auto word = raw[+(char_-char_(" \r\n"))];
    static const auto grammar =
           omit[*sp]
        >> word  // method
        >> omit[*sp]
        >> word  // uri
        >> omit[*sp]
        >> -word // version
        >> omit[eol|eoi]
        ;

    auto success = parse(begin, end, grammar, R);
    // Note: begin updated.

    if (! success) {
        BOOST_THROW_EXCEPTION(error()
            << errinfo_what("Incomplete request line.")
            << errinfo_location(begin)
        );
    }

    input = span_t(begin, end);
    return move(R);
}

ostream& operator<<(
    ostream&                           o,
    const parse_request_line_result_t& R
)
{
    o << "method="  << R.method  << endl
      << "uri="     << R.uri     << endl
      << "version=" << R.version << endl
      ;

    return o;
}

parse_response_line_result_t parse_response_line(span_t& input)
{
    using namespace boost::spirit::qi;
    using ascii::char_;

    parse_response_line_result_t R;

    auto begin = input.begin();
    auto end   = input.end();

    static const auto word = raw[+(char_-char_(" \r\n"))];
    static const auto grammar =
           omit[*sp]
        >> word                        // version
        >> omit[*sp]
        >> word                        // status
        >> omit[*sp]
        >> raw[*(char_-char_("\r\n"))] // message
        >> omit[eol|eoi]
        ;

    auto success = parse(begin, end, grammar, R);
    // Note: begin updated.

    if (! success) {
        BOOST_THROW_EXCEPTION(error()
            << errinfo_what("Incomplete response line.")
            << errinfo_location(begin)
        );
    }

    input = span_t(begin, end);
    return move(R);
}

ostream& operator<<(
    ostream&                            o,
    const parse_response_line_result_t& R
)
{
    o << "version=" << R.version << endl
      << "status="  << R.status  << endl
      << "message=" << R.message << endl
      ;

    return o;
}

parse_uri_result_t parse_uri(span_t& input)
{
    using namespace boost::spirit::qi;
    using ascii::char_;

    parse_uri_result_t R;

    auto begin = input.begin();
    auto end   = input.end();

    static const rule<const char*, parse_uri_result_t()> grammar =
           -(raw[+char_("-A-Za-z0-9+.")] >> omit[char_(":")])     // scheme
        >> -(omit[lit("//")] >> (raw[*(char_-char_("/?#\r\n"))])) // authority
        >> -(raw[*(char_-char_("?#\r\n"))])                       // path
        >> -(omit[char_("?")] >> raw[*(char_-char_("#\r\n"))])    // query
        >> -(omit[char_("#")] >> raw[*(char_-char_("\r\n"))])     // fragment
        >> omit[eol|eoi]
        ;

    auto success = parse(begin, end, grammar, R);
    // Note: begin updated.

    if (! success) {
        BOOST_THROW_EXCEPTION(error()
            << errinfo_what("Incomplete URI.")
            << errinfo_location(begin)
        );
    }

    input = span_t(begin, end);
    return move(R);
}

ostream& operator<<(
    ostream&                  o,
    const parse_uri_result_t& R
)
{
    o << "scheme="    << R.scheme    << endl
      << "authority=" << R.authority << endl
      << "path="      << R.path      << endl
      << "query="     << R.query     << endl
      << "fragment="  << R.fragment  << endl
      ;

    return o;
}

parse_request_result_t parse_request(span_t& input)
{
    parse_request_result_t R;

    const auto begin = input.begin();
    R.request_line = parse_request_line(input);
    R.raw_request_line = span_t(begin, input.begin() - 1);
    span_t uri = R.request_line.uri;
    R.uri = parse_uri(uri);
    if (! uri.empty()) {
        BOOST_THROW_EXCEPTION(error()
            << errinfo_what("URI not fully parsed.")
        );
    }
    R.headers = parse_headers(input);

    return move(R);
}

ostream& operator<<(
    ostream& o,
const parse_request_result_t& R
)
{
    o << "raw_request_line=" << R.raw_request_line << endl
      << R.request_line << endl
      << R.uri          << endl
      << R.headers      << endl
      ;

    return o;
}


parse_response_result_t parse_response(span_t& input)
{
    parse_response_result_t R;

    const auto begin = input.begin();
    R.response_line = parse_response_line(input);
    R.raw_response_line = span_t(begin, input.begin() - 1);
    R.headers = parse_headers(input);

    return move(R);
}

ostream& operator<<(
    ostream& o,
const parse_response_result_t& R
)
{
    o << "raw_response_line=" << R.raw_response_line << endl
      << R.response_line << endl
      << R.headers      << endl
      ;

    return o;
}

} // ParserSuite
} // IronBee
