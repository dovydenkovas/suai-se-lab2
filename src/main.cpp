// Требования: cgicc, libpqxx, jwt-cpp, -std=c++17
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPContentHeader.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTTPRedirectHeader.h>
#include <cgicc/HTTPResponseHeader.h>
#include <cgicc/HTTPStatusHeader.h>

#include <boost/json.hpp>

#include <cstdlib>

#include "apihandler.h"
#include "auth.h"
#include "database.h"
#include "entities.h"


using namespace cgicc;
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
      api.send_error("Unknown api request");
    }
  }

private:
  void login() {
    // token, user{id, role, full_name, group_number?}
    string login = api.get("login");
    string password = api.get("password");

    User user = db.get_user_by_login(login);
    if (!auth::check_password(user.password, password)) {
      api.send_error("Wrong login or password");
      return;
    }

    string token = auth::new_token(login);
    boost::json::object response = {{"token", token},
                                    {"user", boost::json::value_from(user)}};
    api.send(response);
  }

  void me() {
    // id, role, full_name, group_number?
    User user;
    if (auth_and_load(user, ANY)) {
      api.send(boost::json::value_from(user));
    }
  }

  void show_tasks() {
    // [{id, title, subject, teacher, report?{status, grade?}}]
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }
    vector<Task> tasks = db.get_tasks_for(student);
    api.send(boost::json::value_from(tasks));
  }

  void show_task() {
    // {id, title, description, subject, teacher, group_number, report?{id, text,
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }

    size_t id = api.get_route_index();

    Task task = db.get_task(id);
    auto task_json = boost::json::value_from(task);
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
  }

  void add_report() {
    // ok: true
    api.send_error("Not implemented yet");
  }

  void report_by_id() {
    // ok: true
    api.send_error("Not implemented yet");
  }

  void grade_report() {
    // ok: true
    api.send_error("Not implemented yet");
  }

  bool auth_and_load(User &user, Role role) {
    string token = api.get_token();
    if (!auth::check_token(token)) {
      api.redirect_to("/login");
      return false;
    }

    string login = auth::login_by_token(token);

    if (!db.get_user_by_login(&login)) {
      api.send_error("Error while checking user.");
      return false;
    }

    if (!(role | user.role)) {
      api.send_error("Not permitted.");
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

  string conn = db_connection();
  Database db(conn);
  if (db.fail()) {
    api.send_error("Ooops :(");
    return 0;
  }

  Ecampus ecampus(db, api);
  ecampus.handle();
}
