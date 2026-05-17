#pragma once
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>
#include <boost/log/trivial.hpp>

#include <boost/json.hpp>
#include <cstdlib>

#include "apihandler.h"
#include "database.h"
#include "entities.h"

using namespace std;

class Ecampus {
private:
  Database &db;
  ApiHandler &api;

public:
  Ecampus(Database &db, ApiHandler &api) : db(db), api(api) {}

  void handle();

private:
  /**
   * Role: Both     Method: POST      Endpoint: /api/auth/login
   * Headers: -     Request: login, password
   * Response: user{id, role, full_name, group_number?}
   * Discription: login page
   */
  void login();

  /**
   * Role: Both         Method: GET      Endpoint: /api/me
   * Headers: token     Request: -
   * Response: id, role, full_name, group_number?
   * Discription: home page
   */
  void me();

  /**
   * Role: Student      Method: GET      Endpoint: /api/tasks
   * Headers: token     Request: -
   * Response: [{id, title, subject, teacher, report?{status, grade?}]
   * Discription: student's task list.
   */
  void show_tasks();

  /**
   * Role: Student      Method: GET      Endpoint: /api/tasks/{task_id}
   * Headers: token     Request: -
   * Response: {id, title, description, subject, teacher, group_number,
   * report?{id, text, status, grade?}}
   * Discription: task page.
   */
  void show_task();

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/reports
   * Headers: token     Request: -
   * Response: [{id, task, subject, student, group, status, grade?}]
   * Discription: list of reports for teacher.
   */
  void show_reports();

  /**
   * Role: Student      Method: POST      Endpoint: /api/reports
   * Headers: token     Request: task_id, text
   * Response: ok: true
   * Discription: add report.
   */
  void add_report();

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/reports/{report_id}
   * Headers: token     Request: -
   * Response: {id, task{title, description}, subject, student, group, text,
   * status, grade?} Discription: report page.
   */
  void report_by_id();

  /**
   * Role: Teacher      Method: POST      Endpoint: /api/reports/{report_id}
   * Headers: token     Request: status, grade?
   * Response: {ok: true}
   * Discription: update report info.
   */
  void grade_report();

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/teacher/tasks
   * Headers: token     Request: -
   * Response: [{ task_id, title, subject_name, group_number, description }]
   * Discription: list of tasks for teacher
   */
  void show_teacher_tasks();

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/teacher/tasks/{task_id}
   * Headers: token     Request: -
   * Response: { task_id, title, subject_name, group_number, description }
   * Discription: task page for teacher.
   */
  void show_teacher_task();

  /**
   * Role: Teacher      Method: POST       Endpoint: /api/teacher/tasks
   * Headers: token     Request: title, description, group_number, subject_name
   * Response: {ok: true}
   * Discription: add new task.
   */
  void add_task();

  /**
   * Check login token and load user from database.
   */
  bool auth_and_load(User &user, Role role);
};
