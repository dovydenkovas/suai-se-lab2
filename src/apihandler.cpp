#include <boost/algorithm/string.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <boost/log/trivial.hpp>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPResponseHeader.h>
#include <cgicc/HTTPStatusHeader.h>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>

#include "apihandler.h"

using std::string;

using namespace internal;

Request ApiHandler::get_request() {
  BOOST_LOG_TRIVIAL(debug) << "Handle request";
  try {
    header = parse_header();

    if (header.content_length && header.method == Header::POST) {
      if (!parse_body())
        return HANDLED_ERROR;
    }

    return router();
  } catch (const std::exception &e) {
  }
  send_error(500);
  return HANDLED_ERROR;
}

Request ApiHandler::router() {
  BOOST_LOG_TRIVIAL(trace) << "=== Route ===";
  BOOST_LOG_TRIVIAL(debug) << "parse route: " << header.path;
  if (header.path == "/auth/login")
    return LOGIN;
  if (header.path == "/me")
    return ME;
  if (header.path == "/tasks")
    return TASKS;

  if (header.path == "/reports") {
    if (header.method == Header::GET)
      return REPORTS;
    if (header.method == Header::POST)
      return ADD_REPORT;
  }

  if (header.path == "/teacher/tasks") {
    if (header.method == Header::GET)
      return SHOW_TEACHER_TASKS;
    else
      return ADD_TASK;
  }

  auto p = try_parse_route(header.path);
  if (p.has_value()) {
    string prefix = p->first;
    route_index = p->second;


    if (prefix == "/tasks/")
      return TASK_BY_ID;
    if (prefix == "/reports/") {
      if (header.method == Header::GET)
        return REPORTS_BY_ID;
      else
        return GRADE_REPORT;
    }
    if (prefix == "/teacher/tasks/") {
      return SHOW_TEACHER_TASK;
    }
  }
  return HANDLED_ERROR;
}

string ApiHandler::get(string key) { return request[key].as_string().c_str(); }

size_t ApiHandler::get_int(string key) { return request[key].as_uint64(); }

size_t ApiHandler::get_route_index() { return route_index; }

string ApiHandler::get_token() { return header.token; }

void ApiHandler::send(boost::json::value response, size_t err) {
  if (responsed) {
    return;
  }

  std::cout << cgicc::HTTPStatusHeader(err, reason_for(err)) << std::endl;
  BOOST_LOG_TRIVIAL(debug) << boost::json::serialize(response);
  std::cout << boost::json::serialize(response) << std::endl;
  responsed = true;
}

void ApiHandler::send_error(size_t err) {
  if (responsed) {
    return;
  }

  boost::json::object resp;
  resp["ok"] = false;
  send(resp, err);
}

void ApiHandler::send_ok() {
  if (responsed) {
    return;
  }

  boost::json::object resp;
  resp["ok"] = true;
  send(resp);
}

string internal::extract_bearer_token(const string &auth_header) {
  BOOST_LOG_TRIVIAL(trace) << "token string: " << auth_header;
  if (auth_header.substr(0, 7) == "Bearer ") {
    string token = auth_header.substr(7);
    auto l = token.find_first_not_of(" \t\r\n");
    auto r = token.find_last_not_of(" \t\r\n");
    if (l == string::npos)
      return {};
    return token.substr(l, r - l + 1);
  }
  return {};
}

std::optional<std::pair<string, size_t>> internal::try_parse_route(string path) {
  BOOST_LOG_TRIVIAL(debug) << "try parse index from route";
  int separator = path.rfind("/");
  string num = path.substr(separator + 1);
  size_t pos = 0;
  size_t id = std::numeric_limits<size_t>::max();
  try {
    id = std::stoi(num, &pos);
  } catch (...) {
    return {};
  }
  string prefix = path.substr(0, separator);
  BOOST_LOG_TRIVIAL(trace) << "prefix: " << prefix;
  BOOST_LOG_TRIVIAL(trace) << "index: " << id;
  return {std::pair<string, int>{prefix, id}};
}

extern char **environ;

void internal::print_enviroment() {
  BOOST_LOG_TRIVIAL(trace) << "=== All enviroment variables ===";

  for (char **env = ::environ; env && *env; ++env) {
    const char *entry = *env;
    const char *eqPos = std::strchr(entry, '=');
    if (!eqPos)
      continue;

    std::string key(entry, eqPos - entry);
    std::string value(eqPos + 1);

    BOOST_LOG_TRIVIAL(trace) << key << " = " << value;
  }
}

Header internal::parse_header() {
  Header h;
  print_enviroment();
  BOOST_LOG_TRIVIAL(debug) << "=== Parse header ===";

  const char *t = std::getenv("HTTP_AUTHORIZATION");
  h.token = (t != nullptr) ? extract_bearer_token(t) : "";

  string method = std::getenv("REQUEST_METHOD");
  h.method = method == "GET" ? Header::GET : Header::POST;

  char *length_str = std::getenv("CONTENT_LENGTH");
  h.content_length = length_str ? std::stoi(length_str) : 0;
  h.path = std::getenv("PATH_INFO");
  return h;
}

/**
 * Parse body with json request to this->request.
 * Call send_error(errno) if fail.
 *
 * @return true - on success, false on error.
 */
bool ApiHandler::parse_body() {
  BOOST_LOG_TRIVIAL(debug) << "=== Parse header ===";
  BOOST_LOG_TRIVIAL(trace) << "Content length: " << header.content_length;
  if (header.content_length == 0 || header.content_length > 10'000) {
    BOOST_LOG_TRIVIAL(debug) << "Too large or too small content_length";
    send_error(431);
    return false;
  }

  std::string body;
  body.resize(static_cast<std::size_t>(header.content_length));

  std::cin >> std::noskipws;
  std::cin.read(&body[0], header.content_length);

  std::streamsize got = std::cin.gcount();
  body.resize(got);
  if (got != header.content_length) {
    BOOST_LOG_TRIVIAL(debug)
        << "Read only " << got << " of " << header.content_length << " bytes";
  }
  BOOST_LOG_TRIVIAL(trace) << "Body: " << body;

  // Strip for dirty input.
  auto l = body.find_first_of("{[");
  auto r = body.find_last_of("}]");
  body = body.substr(l, r - l + 1);

  boost::json::error_code ec;
  boost::json::value jv = boost::json::parse(body, ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "Fail to parse json (" << jv << ")";
    send_error(418);
    return HANDLED_ERROR;
  }

  if (!jv.is_object()) {
    BOOST_LOG_TRIVIAL(error) << "Json is not object: " << jv;
    send_error(418);
    return HANDLED_ERROR;
  }
  request = jv.as_object();
  return true;
}

string internal::reason_for(int code) {
  static const std::unordered_map<int, string> reason_map = {
      {100, "Continue"},
      {101, "Switching Protocols"},
      {102, "Processing"},
      {200, "OK"},
      {201, "Created"},
      {202, "Accepted"},
      {203, "Non-Authoritative Information"},
      {204, "No Content"},
      {205, "Reset Content"},
      {206, "Partial Content"},
      {300, "Multiple Choices"},
      {301, "Moved Permanently"},
      {302, "Found"},
      {303, "See Other"},
      {304, "Not Modified"},
      {305, "Use Proxy"},
      {307, "Temporary Redirect"},
      {308, "Permanent Redirect"},
      {400, "Bad Request"},
      {401, "Unauthorized"},
      {402, "Payment Required"},
      {403, "Forbidden"},
      {404, "Not Found"},
      {405, "Method Not Allowed"},
      {406, "Not Acceptable"},
      {407, "Proxy Authentication Required"},
      {408, "Request Timeout"},
      {409, "Conflict"},
      {410, "Gone"},
      {411, "Length Required"},
      {412, "Precondition Failed"},
      {413, "Payload Too Large"},
      {414, "URI Too Long"},
      {415, "Unsupported Media Type"},
      {416, "Range Not Satisfiable"},
      {417, "Expectation Failed"},
      {418, "I’m a teapot"},
      {426, "Upgrade Required"},
      {431, "Request Header Fields Too Large"},
      {500, "Internal Server Error"},
      {501, "Not Implemented"},
      {502, "Bad Gateway"},
      {503, "Service Unavailable"},
      {504, "Gateway Timeout"},
      {505, "HTTP Version Not Supported"}};

  auto it = reason_map.find(code);
  return (it != reason_map.end()) ? it->second : string("Status");
}
