#pragma once
#include <boost/json/object.hpp>
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
  HANDLED_ERROR,
};

class ApiHandler {
private:
    string token;
    boost::json::object req;
    bool responsed = false;
    size_t route_index = 0;

public:
  Request request();

  string get(string key);
  size_t get_int(string key);
  size_t get_route_index();
  string get_token();

  void send(boost::json::value response, size_t err=200);
  void send_error(size_t err);
  void send_ok();
};
