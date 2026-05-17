#pragma once
#include <boost/json/object.hpp>
#include <optional>
#include <string>

using std::string;

enum Request {
  LOGIN,
  ME,
  TASKS,
  TASK_BY_ID,
  REPORTS,
  ADD_REPORT,
  REPORTS_BY_ID,
  GRADE_REPORT,
  SHOW_TEACHER_TASKS,
  SHOW_TEACHER_TASK,
  ADD_TASK,
  HANDLED_ERROR,
};

struct Header {
  enum { GET, POST } method;
  size_t content_length;
  string token;
  string path;
};

class ApiHandler {
private:
  Header header;
  boost::json::object request;
  bool responsed = false;
  size_t route_index = 0;

public:
  Request get_request();

  string get(string key);
  size_t get_int(string key);
  size_t get_route_index();
  string get_token();

  void send(boost::json::value response, size_t err = 200);
  void send_error(size_t err);
  void send_ok(size_t err=200);

private:
  bool parse_body();
  Request router();
};

namespace internal {
/**
 * Returns description of http code.
 */
string reason_for(int code);

/**
 * Parse token string.
 *
 * @return auth token.
 */
string extract_bearer_token(const string &auth_header);

/**
 * Parse index from route
 *
 * @return prefix of route and index
 *
 * Example:
 * try_parse_route("/example/12") -> {"/example", 12}
 */
std::optional<std::pair<string, size_t>> try_parse_route(string route);
void print_enviroment();
Header parse_header();
}; // namespace internal
