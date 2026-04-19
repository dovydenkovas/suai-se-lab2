#pragma once

#include <boost/json/conversion.hpp>
#include <boost/json/fwd.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>
#include <optional>
#include <string>

using std::optional;
using std::string;

enum Role { ANY = 3, TEACHER = 1, STUDENT = 2 };

// Вспомогательные структуры
struct User {
  int id;
  string login;
  string password;
  string full_name;
  Role role;
  optional<string> group_number;

  boost::json::object as_json();
};

struct Report {
  enum { SENT, ACCEPTED } status;
  int grade;

  boost::json::object as_json();
};

struct Task {
  size_t id;
  string group_number;
  string teacher;
  string subject;

  string title;
  string description;
  optional<Report> report;

  boost::json::object as_json();
};
