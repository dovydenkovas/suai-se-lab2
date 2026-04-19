#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>

#include <boost/json.hpp>

#include <cstdlib>

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
    if (!auth::check_password(user.password, password)) {
      api.send_error(403);
      return;
    }

    string token = auth::new_token(login);
    boost::json::object response = {{"token", token},
                                    {"user", user.as_json()}};
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
    for (auto task: tasks) {
        res.push_back(task.as_json());
    }
    boost::json::object t;
    t["tasks"] = res;
    api.send(t);
  }

  void show_task() {
    // {id, title, description, subject, teacher, group_number, report?{id, text,
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
  }

  void add_report() {
    // ok: true
    api.send_error(418);
  }

  void report_by_id() {
    // ok: true
    api.send_error(418);
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
  } catch(std::exception &e) {
      api.send_error(500);
      return 0;
  }
}
