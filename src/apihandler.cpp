#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/algorithm/string.hpp>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPResponseHeader.h>
#include <cgicc/HTTPStatusHeader.h>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>

#include "apihandler.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

string extract_bearer_token(const string &auth_header) {
  if (auth_header.substr(0, 7) == "Bearer ") {
    string token = auth_header.substr(7);
    // Use your token here
    auto l = token.find_first_not_of(" \t\r\n");
    auto r = token.find_last_not_of(" \t\r\n");
    if (l == string::npos)
      return {};
    return token.substr(l, r - l + 1);
  }
  return {};
}

std::optional<std::pair<string, size_t>> try_parse_route(string route) {
  int separator = route.rfind("/", 0);
  string num = route.substr(separator+1);
  size_t pos = 0;
  size_t id = std::stoi(num, &pos);
  if (pos != num.size())
    return {};
  string prefix = route.substr(0, separator+1);
  return {std::pair<string, int>{prefix, id}};
}

Request ApiHandler::request() {
  try {
    cgicc::Cgicc cgi;
    const char *t = std::getenv("HTTP_AUTHORIZATION");
    token = (t != nullptr) ? extract_bearer_token(t) : "";

    auto content_length = cgi.getEnvironment().getContentLength();
    string body;
    if (content_length > 10'000) {
      send_error(431);
      return HANDLED_ERROR;
    }
    if (content_length == 0) {
      content_length = 10'000;
    }
    body.resize(content_length);
    cin.read(&body[0], content_length);
    if (!cin.eof()) {
      send_error(431);
      return HANDLED_ERROR;
    }

    int i = body.find("\r\n\r\n");
    body = body.substr(i);
    auto l = body.find_first_of("{[");
    auto r = body.find_last_of("}]");
    body = body.substr(l, r - l + 1);

    boost::json::error_code ec;
    boost::json::value jv = boost::json::parse(body, ec);
    if (ec) {
      std::cout << jv << std::endl;
      send_error(418);
      return HANDLED_ERROR;
    }

    if (!jv.is_object()) {
      send_error(418);
      return HANDLED_ERROR;
    }
    req = jv.as_object();

    const string route = std::getenv("PATH_INFO");
    if (route == "/auth/login")
      return LOGIN;
    if (route == "/me")
      return ME;
    if (route == "/tasks")
      return TASKS;

    const string method = std::getenv("REQUEST_METHOD");
    if (route == "/reports") {
      if (method == "GET")
        return REPORTS;
      if (method == "POST")
        return ADD_REPORT;
    }

    auto p = try_parse_route(route);
    if (p.has_value()) {
      string prefix = p->first;
      size_t id = p->second;
      route_index = id;
      if (prefix == "/tasks/")
        return TASK_BY_ID;
      if (prefix == "/reports/") {
        if (method == "GET")
          return REPORTS_BY_ID;
        if (method == "POST")
          return GRADE_REPORT;
      }
    }

  } catch (const std::exception &e) {
  }
  send_error(500);
  return HANDLED_ERROR;
}

string ApiHandler::get(string key) { return req[key].as_string().c_str(); }

size_t ApiHandler::get_int(string key) { return req[key].as_uint64(); }

size_t ApiHandler::get_route_index() { return route_index; }

string ApiHandler::get_token() { return token; }

string reason_for(int code) {
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

void ApiHandler::send(boost::json::object response, size_t err) {
  if (responsed) {
    return;
  }

  cout << cgicc::HTTPStatusHeader(err, reason_for(err)) << endl;
  cout << "Content-Type: application/json\r\n\r\n";
  cout << boost::json::serialize(response) << endl;
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
