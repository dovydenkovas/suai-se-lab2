#include <boost/json/conversion.hpp>
#include <boost/json/fwd.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>
#include <optional>
#include <string>

#include "entities.h"

using std::optional;
using std::string;

boost::json::value role_as_json(Role role) {
  if (role == STUDENT) {
    return "student";
  } else if (role == TEACHER) {
    return "teacher";
  } else {
    return "unknown";
  }
}

boost::json::object User::as_json() {
  boost::json::object obj;
  obj["id"] = id;
  obj["login"] = login;
  obj["full_name"] = full_name;
  obj["role"] = role_as_json(role);
  if (group_number)
    obj["group_number"] = *group_number;
  return obj;
}

boost::json::object Task::as_json() {
  boost::json::object obj;
  obj["id"] = id;
  obj["teacher"] = teacher.full_name;
  obj["subject"] = subject;
  obj["title"] = title;
  if (report.has_value()) {
    obj["report"] = report->as_json();
  }
  return obj;
}

boost::json::object Report::as_json() {
    boost::json::object obj;
    obj["status"] = status == ACCEPTED ? "ACCEPTED" : "SENT";
    if (status == ACCEPTED) {
        obj["grade"] = grade;
    }
    return obj;
}
