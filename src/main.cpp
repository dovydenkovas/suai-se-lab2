#include <algorithm>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

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
      BOOST_LOG_TRIVIAL(debug) << "login page";
      login();
      break;
    case Request::ME:
      BOOST_LOG_TRIVIAL(debug) << "me page";
      me();
      break;
    case TASKS:
      BOOST_LOG_TRIVIAL(debug) << "tasks page";
      show_tasks();
      break;
    case Request::TASK_BY_ID:
      BOOST_LOG_TRIVIAL(debug) << "task by id page";
      show_task();
      break;
    case Request::REPORTS:
      BOOST_LOG_TRIVIAL(debug) << "reports page";
      show_reports();
      break;
    case Request::ADD_REPORT:
      BOOST_LOG_TRIVIAL(debug) << "add report page";
      add_report();
      break;
    case Request::REPORTS_BY_ID:
      BOOST_LOG_TRIVIAL(debug) << "get report page";
      report_by_id();
      break;
    case Request::GRADE_REPORT:
      BOOST_LOG_TRIVIAL(debug) << "grade report page";
      grade_report();
      break;
    case Request::SHOW_TEACHER_TASKS:
      BOOST_LOG_TRIVIAL(debug) << "show teacher tasks";
      show_teacher_tasks();
      break;
    case Request::SHOW_TEACHER_TASK:
      BOOST_LOG_TRIVIAL(debug) << "show teacher task";
      show_teacher_task();
      break;
    case Request::ADD_TASK:
      BOOST_LOG_TRIVIAL(debug) << "add task";
      add_task();
      break;
    default:
      BOOST_LOG_TRIVIAL(error) << "Unknown request";
      api.send_error(500);
    }
  }

private:
  /**
   * Role: Both     Method: POST      Endpoint: /api/auth/login
   * Headers: -     Request: login, password
   * Response: user{id, role, full_name, group_number?}
   * Discription: login page
   */
  void login() {
    string login = api.get("login");
    string password = api.get("password");

    auto u = db.get_user_by_login(login);
    if (!u.has_value()) {
      BOOST_LOG_TRIVIAL(debug) << "Wrong login name";
      api.send_error(403);
      return;
    }

    User user = u.value();
    if (!auth::check_password(password, user.password)) {
      BOOST_LOG_TRIVIAL(debug) << "Wrong password";
      api.send_error(403);
      return;
    }

    string token = auth::new_token(login);
    boost::json::object response = {{"token", token}, {"user", user.as_json()}};
    api.send(response);
  }

  /**
   * Role: Both         Method: GET      Endpoint: /api/me
   * Headers: token     Request: -
   * Response: id, role, full_name, group_number?
   * Discription: home page
   */
  void me() {
    User user;
    if (auth_and_load(user, ANY)) {
      api.send(user.as_json());
    }
  }

  /**
   * Role: Student      Method: GET      Endpoint: /api/tasks
   * Headers: token     Request: -
   * Response: [{id, title, subject, teacher, report?{status, grade?}]
   * Discription: student's task list.
   */
  void show_tasks() {
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

  /**
   * Role: Student      Method: GET      Endpoint: /api/tasks/{task_id}
   * Headers: token     Request: -
   * Response: {id, title, description, subject, teacher, group_number,
   * report?{id, text, status, grade?}}
   * Discription: task page.
   */
  void show_task() {
    User student;
    if (!auth_and_load(student, STUDENT)) {
      return;
    }
    size_t id = api.get_route_index();
    auto t = db.get_task(id);
    if (!t.has_value()) {
      BOOST_LOG_TRIVIAL(debug) << "Wrong task id";
      api.send_error(404);
    }
    Task task = t.value();
    auto task_json = task.as_json();
    task_json["descriprion"] = task.description;
    task_json["group_number"] = task.group_number;

    api.send(task_json);
  }

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/reports
   * Headers: token     Request: -
   * Response: [{id, task, subject, student, group, status, grade?}]
   * Discription: list of reports for teacher.
   */
  void show_reports() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    vector<Report> reports = db.get_reports_for(teacher);
    boost::json::array res;
    for (auto r : reports) {
      auto report = r.as_json();
      report["student"] = r.student_id;
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
    api.send(res);
  }

  /**
   * Role: Student      Method: POST      Endpoint: /api/reports
   * Headers: token     Request: task_id, text
   * Response: ok: true
   * Discription: add report.
   */
  void add_report() {
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
      BOOST_LOG_TRIVIAL(debug) << "Student has no access to this task";
      api.send(406);
      return;
    }
    Task task = *t_it;

    if (task.report && task.report->status == Report::ACCEPTED) {
      BOOST_LOG_TRIVIAL(debug) << "Report was already accepted";
      api.send(406);
      return;
    }

    Report report = Report{0, task_id, student.id, text, 0, Report::SENT};
    db.insert_report(report);
    api.send_ok();
  }

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/reports/{report_id}
   * Headers: token     Request: -
   * Response: {id, task{title, description}, subject, student, group, text,
   * status, grade?} Discription: report page.
   */
  void report_by_id() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    auto r = db.get_report(api.get_route_index());
    if (!r) {
      BOOST_LOG_TRIVIAL(warning)
          << "There are no report with index " << api.get_route_index();
      api.send_error(418);
      return;
    }

    Report report = *r;
    Task task = *db.get_task(report.task_id);

    if (task.teacher != teacher.id) {
      BOOST_LOG_TRIVIAL(warning)
          << "Teacher " << teacher.id << " want to access to task "
          << task.teacher << ", but has no perms.";
      api.send_error(406);
      return;
    }

    User student = *db.get_user(report.student_id);

    auto jo = report.as_json();
    jo["text"] = report.text;
    jo["task"] = {{"title", task.title}, {"description", task.description}};
    jo["subject"] = task.subject;
    jo["student"] = {{"id", report.student_id},
                     {"full_name", student.full_name}};
    jo["group"] = task.group_number;
    api.send(jo);
  }

  /**
   * Role: Teacher      Method: POST      Endpoint: /api/reports/{report_id}
   * Headers: token     Request: status, grade?
   * Response: {ok: true}
   * Discription: update report info.
   */
  void grade_report() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    size_t report_id = api.get_route_index();
    auto r = db.get_report(report_id);
    if (!report_id) {
      BOOST_LOG_TRIVIAL(debug) << "Report " << report_id << " not found.";
      api.send_error(404);
      return;
    }
    Report report = *r;

    auto status = api.get("status");
    if (status == "ACCEPTED") {
      report.status = Report::ACCEPTED;
      report.grade = api.get_int("grade");
    } else {
      report.status = Report::SENT;
    }

    db.update_grade(report);
    api.send_ok();
  }

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/teacher/tasks
   * Headers: token     Request: -
   * Response: [{ task_id, title, subject_name, group_number, description }]
   * Discription: list of tasks for teacher
   */
  void show_teacher_tasks() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }
    vector<Task> tasks = db.get_tasks_from(teacher);
    boost::json::array res;
    for (auto task : tasks) {
      boost::json::object jo;
      jo["task_id"] = task.id;
      jo["title"] = task.title;
      jo["subject_name"] = task.subject;
      jo["group_number"] = task.group_number;
      jo["description"] = task.description;
      res.push_back(jo);
    }
    api.send(res);
  }

  /**
   * Role: Teacher      Method: GET      Endpoint: /api/teacher/tasks/{task_id}
   * Headers: token     Request: -
   * Response: { task_id, title, subject_name, group_number, description }
   * Discription: task page for teacher.
   */
  void show_teacher_task() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    size_t task_id = api.get_route_index();
    auto t = db.get_task(task_id);

    if (!t) {
      BOOST_LOG_TRIVIAL(debug) << "No task " << task_id;
      api.send(404);
      return;
    }
    auto task = *t;
    if (task.teacher != teacher.id) {
      BOOST_LOG_TRIVIAL(debug)
          << "User " << teacher.id << " try to access teacher page of "
          << task_id << " task.";
      api.send_error(406);
      return;
    }
    boost::json::object jo;
    jo["task_id"] = task.id;
    jo["title"] = task.title;
    jo["subject_name"] = task.subject;
    jo["group_number"] = task.group_number;
    jo["description"] = task.description;
    api.send(jo);
  }

  /**
   * Role: Teacher      Method: POST       Endpoint: /api/teacher/tasks
   * Headers: token     Request: title, description, group_number, subject_name
   * Response: {ok: true}
   * Discription: add new task.
   */
  void add_task() {
    User teacher;
    if (!auth_and_load(teacher, TEACHER)) {
      return;
    }

    Task task{0,
              api.get("group_number"),
              teacher.id,
              api.get("subject"),
              api.get("title"),
              api.get("description"),
              {}};

    db.insert_task(task);
    api.send_ok();
  }

  /**
   * Check login token and load user from database.
   */
  bool auth_and_load(User &user, Role role) {
    BOOST_LOG_TRIVIAL(trace) << "Check login";
    string token = api.get_token();
    if (!auth::check_token(token)) {
      BOOST_LOG_TRIVIAL(debug) << "Token " << token << " not registered.";
      api.send_error(401);
      return false;
    }

    string login = auth::login_by_token(token);
    auto u = db.get_user_by_login(login);
    if (!u) {
      BOOST_LOG_TRIVIAL(error)
          << "There are token to not existed user: " << token << " for "
          << login;
      api.send_error(500);
      return false;
    }
    user = *u;

    if (!(role | user.role)) {
      BOOST_LOG_TRIVIAL(debug) << "User " << login << " has role " << user.role
                               << " but expected " << role;
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

void init_logging() {
  boost::log::add_file_log(
      boost::log::keywords::file_name = "/srv/ecampus-log_%N.log",
      boost::log::keywords::open_mode = std::ios_base::app,
      boost::log::keywords::rotation_size = 5 * 1024 * 1024,
      boost::log::keywords::max_size = 3 * 5 * 1024 * 1024,
      boost::log::keywords::format = "[%TimeStamp%] <%Severity%> %Message%");
  boost::log::add_common_attributes();
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::trace);
}

int main() {
  init_logging();
  BOOST_LOG_TRIVIAL(trace) << "BEGIN";
  ApiHandler api;
  try {
    string conn = db_connection();
    Database db(conn);
    if (db.fail()) {
      BOOST_LOG_TRIVIAL(error) << "Unable to connect database.";
      api.send_error(500);
      return 0;
    }

    Ecampus ecampus(db, api);
    ecampus.handle();
  } catch (std::exception &e) {
    BOOST_LOG_TRIVIAL(error) << "Unexpected global exception: " << e.what();
    api.send_error(500);
    return 0;
  }
}
