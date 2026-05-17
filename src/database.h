#pragma once

#include "entities.h"
#include <cstddef>
#include <optional>
#include <pqxx/pqxx>
#include <string>

using std::string;
using std::vector;

class Database {
private:
  pqxx::connection conn;
  pqxx::work txn;
  bool failed;

public:
  Database(string connection);

  std::optional<User> get_user_by_login(string login);
  std::optional<User> get_student(size_t id);
  std::optional<User> get_teacher(size_t id);

  std::optional<size_t> get_group_id(string name);
  std::optional<size_t> get_subject_id(string name);

  vector<Task> get_tasks_for(const User &student);
  vector<Task> get_tasks_from(const User &teacher);
  vector<Report> get_reports_for(const size_t &task_id);
  vector<Report> get_reports_for(const User &teacher);

  std::optional<Task> get_task(size_t id);
  bool insert_report(Report report);
  bool insert_task(Task task);
  bool update_grade(Report report);
  optional<Report> get_report(size_t id);
  bool fail();

private:
  void prepare_statements();
};
