#include <algorithm>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>

#include <boost/json.hpp>
#include <cstdlib>
#include <iostream>

#include "apihandler.h"
#include "auth.h"
#include "database.h"
#include "entities.h"

using namespace std;

class Ecampus {
private:
  Database &db;
  ApiHandler &api;

public:
  Ecampus(Database &db, ApiHandler &api) : db(db), api(api) {}

  void handle() {
    switch (api.request()) {
    case Request::LOGIN:
      login();
      break;
    case Request::ME:
      me();
      break;
    case TASKS:
      show_tasks();
      break;
    case Request::TASK_BY_ID:
      show_task();
      break;
    case Request::REPORTS:
      show_reports();
      break;
    case Request::ADD_REPORT:
      add_report();
      break;
    case Request::REPORTS_BY_ID:
      report_by_id();
      break;
    case Request::GRADE_REPORT:
      grade_report();
      break;
    default:
      api.send_error(500);
    }
  }

private:
  void login() {
    // token, user{id, role, full_name, group_number?}
    string login = api.get("login");
    string password = api.get("password");

    auto u = db.get_user_by_login(login);
    if (!u.has_value()) {
      api.send_error(403);
      return;
    }

    User user = u.value();
    if (!auth::check_password(password, user.password)) {
      api.send_error(403);
      return;
    }

    string token = auth::new_token(login);
    boost::json::object response = {{"token", token}, {"user", user.as_json()}};
    api.send(response);
  }

  void me() {
    // id, role, full_name, group_number?
    User user;
    if (auth_and_load(user, ANY)) {
      api.send(user.as_json());
    }
  }

  void show_tasks() {
    // [{id, title, subject, teacher, report?{status, grade?}}]
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }
    vector<Task> tasks = db.get_tasks_for(student);
    boost::json::array res;
    for (auto task : tasks) {
      res.push_back(task.as_json());
    }
    boost::json::object t;
    t["tasks"] = res;
    api.send(t);
  }

  void show_task() {
    // {id, title, description, subject, teacher, group_number, report?{id,
    // text,
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }

    size_t id = api.get_route_index();

    auto t = db.get_task(id);
    if (!t.has_value()) {
      api.send_error(404);
    }
    Task task = t.value();
    auto task_json = task.as_json();
    task_json["descriprion"] = task.description;
    task_json["group_number"] = task.group_number;

    api.send(task_json);
  }

  void show_reports() {
    // [{id, task, subject, student, group, status, grade?}]
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    vector<Report> reports = db.get_reports_for(teacher);
    boost::json::array res;
    for (auto r : reports) {
      auto report = r.as_json();
      report["student"] = r.student;
      if (r.task_id) {
        auto task = *db.get_task(r.task_id);

        report["task"] = {
            {"title", task.title},
            {"description", task.description},
        };

        report["subject"] = task.subject;
        report["group"] = task.group_number;
      }

      res.push_back(report);
    }
    boost::json::object t;
    t["reports"] = res;
    api.send(t);
  }

  void add_report() {
    // ok: true
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }

    size_t task_id = api.get_int("task_id");
    string text = api.get("text");

    auto tasks = db.get_tasks_for(student);
    auto t_it = find_if(tasks.begin(), tasks.end(),
                        [task_id](Task t) { return t.id == task_id; });
    if (t_it == tasks.end()) {
      api.send(406);
      return;
    }
    Task task = *t_it;

    if (task.report && task.report->status == Report::ACCEPTED) {
      api.send(406);
      return;
    }

    Report report = Report{0, task_id, student.id, text, 0, Report::SENT};
    db.insert_report(report);
    api.send_ok();
  }

  void report_by_id() {
    // ok: true
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    auto r = db.get_report(api.get_route_index());
    if (!r) {
      api.send_error(418);
      return;
    }

    Report report = *r;
    Task task = *db.get_task(report.task_id);

    if (task.teacher != teacher.id) {
      api.send_error(406);
      return;
    }

    User student = *db.get_user(report.student_id);

    auto jo = report.as_json();
    jo["text"] = report.text;
    jo["task"] = {{"title", task.title}, {"description", task.description}};
    jo["subject"] = task.subject;
    jo["student"] = {{"id", report.student_id}, {"full_name", student.full_name}};
    jo["group"] = task.group_number;
    api.send(jo);
  }

  void grade_report() {
    // ok: true
    api.send_error(418);
  }

  bool auth_and_load(User &user, Role role) {
    string token = api.get_token();
    if (!auth::check_token(token)) {
      api.send_error(401);
      return false;
    }

    string login = auth::login_by_token(token);

    if (!db.get_user_by_login(login)) {
      api.send_error(500);
      return false;
    }

    if (!(role | user.role)) {
      api.send_error(406);
      return false;
    }

    return true;
  }
};

string db_connection() {
  const char *s = getenv("DB_CONNECTION");
  return s ? string(s)
           : "host=localhost port=5432 dbname=ecampus user=ecampus "
             "password=ecampus";
}

int main() {
  ApiHandler api;
  try {
    string conn = db_connection();
    Database db(conn);
    if (db.fail()) {
      api.send_error(500);
      return 0;
    }

    Ecampus ecampus(db, api);
    ecampus.handle();
  } catch (std::exception &e) {
    api.send_error(500);
    return 0;
  }
}
