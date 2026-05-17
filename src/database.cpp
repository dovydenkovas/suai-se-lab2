#include <optional>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>
#include <pqxx/pqxx>

#include "database.h"
#include "entities.h"

using std::optional;
using std::string;
using std::vector;

Database::Database(string connection)
    : conn{connection}, txn{conn}, failed{false} {}

void Database::prepare_statements() {}

optional<User> Database::get_student(size_t id) {
  // pqxx::work txn(conn);
  const std::string sql =
      "SELECT s.student_id, s.full_name, s.user_login, s.user_password, "
      "sg.group_number "
      "FROM student s JOIN study_group sg ON s.group_id = sg.group_id "
      "WHERE student_id = $1";

  pqxx::params p;
  p.append(id);
  pqxx::result res = txn.exec(sql, p);

  if (res.empty()) {
    txn.commit();
    return {};
  }

  const auto &row = res.front();
  User student;
  student.id = row["student_id"].as<size_t>();
  student.full_name = row["full_name"].as<string>();
  student.login = row["user_login"].as<string>();
  student.password = row["user_password"].as<string>();
  student.role = STUDENT;
  student.group_number =
      row["group_number"].is_null()
          ? optional<string>{}
          : optional<string>(row["group_number"].as<string>());

  BOOST_LOG_TRIVIAL(trace) << "Find student with id: " << student.as_json();
  txn.commit();
  return {student};
}

optional<User> Database::get_teacher(size_t id) {
  // pqxx::work txn(conn);
  const std::string sql =
      "SELECT teacher_id, full_name, user_login, user_password FROM teacher "
      "WHERE teacher_id = $1";

  pqxx::params p;
  p.append(id);
  pqxx::result res = txn.exec(sql, p);

  if (res.empty()) {
    txn.commit();
    return {};
  }

  const auto &row = res.front();
  User teacher;
  teacher.id = row["teacher_id"].as<size_t>();
  teacher.full_name = row["full_name"].as<string>();
  teacher.login = row["user_login"].as<string>();
  teacher.password = row["user_password"].as<string>();
  teacher.role = TEACHER;
  teacher.group_number = {};

  BOOST_LOG_TRIVIAL(trace) << "Find teacher with id: " << teacher.as_json();
  txn.commit();
  return {teacher};
}

optional<size_t> Database::get_group_id(string name) {
  const std::string sql = "SELECT group_id FROM study_group "
                          "WHERE group_number = $1";

  pqxx::params p;
  p.append(name);
  pqxx::result res = txn.exec(sql, p);

  if (res.empty()) {
    return {};
  }

  size_t gid = res.front()["group_id"].as<size_t>();
  return {gid};
}

optional<size_t> Database::get_subject_id(string name) {
  const std::string sql = "SELECT subject_id FROM subject WHERE name = $1";

  pqxx::params p;
  p.append(name);
  pqxx::result res = txn.exec(sql, p);

  if (res.empty()) {
    return {};
  }

  BOOST_LOG_TRIVIAL(trace) << "Find subject with name: " << name;
  size_t sid = res.front()["subject_id"].as<size_t>();
  return {sid};
}

optional<User> Database::get_user_by_login(string login) {

  const std::string sql =
      "SELECT teacher_id, full_name, user_login, user_password FROM teacher "
      "WHERE user_login = $1";

  pqxx::params p;
  p.append(login);
  pqxx::result res = txn.exec(sql, p);

  if (!res.empty()) {
    const auto &row = res.front();
    User teacher;
    teacher.id = row["teacher_id"].as<size_t>();
    teacher.full_name = row["full_name"].as<string>();
    teacher.login = row["user_login"].as<string>();
    teacher.password = row["user_password"].as<string>();
    teacher.role = TEACHER;
    teacher.group_number = {};

    BOOST_LOG_TRIVIAL(trace) << "Find teacher with id: " << teacher.as_json();
    return {teacher};
  }

  const std::string sql_s =
      "SELECT student_id, full_name, user_login, user_password, "
      "sg.group_number as group "
      "FROM student s JOIN study_group sg ON s.group_id = sg.group_id "
      "WHERE s.user_login = $1";

  pqxx::result res_s = txn.exec(sql_s, p);

  for (const auto &row : res_s) {
    User student;
    student.id = row["student_id"].as<size_t>();
    student.group_number = row["group"].is_null()
                               ? optional<string>{}
                               : optional<string>(row["group"].as<string>());
    student.login = row["user_login"].as<string>();
    student.full_name = row["full_name"].as<string>();
    student.password = row["user_password"].as<string>();
    student.role = STUDENT;

    BOOST_LOG_TRIVIAL(trace) << "Find student with id: " << student.as_json();
    txn.commit();
    return {student};
  }

  BOOST_LOG_TRIVIAL(trace) << "No teachers or students with login: " << login;
  return {};
}

vector<Task> Database::get_tasks_for(const User &student) {
  if (student.role != STUDENT)
    return {};

  vector<Task> tasks;

  pqxx::params p;
  p.append(student.group_number);
  const std::string sql =
      "SELECT task_id, sg.group_number as group_number, ss.teacher_id, title, "
      "description, ss.name as subject_name FROM task "
      "JOIN study_group sg ON task.group_id = sg.group_id "
      "JOIN subject ss ON ss.teacher_id = task.teacher_id "
      "WHERE group_number = $1";
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    BOOST_LOG_TRIVIAL(trace) << "Task: " << row["task_id"].as<size_t>();
    User teacher = *get_teacher(row["teacher_id"].as<size_t>());
    auto report = get_report(row["task_id"].as<size_t>());

    Task task{
        row["task_id"].as<size_t>(),
        row["title"].as<string>(),
        row["description"].as<string>(),
        row["subject_name"].as<string>(),
        teacher,
        row["group_number"].as<string>(),
    };

    task.report = report;

    tasks.push_back(task);
  }
  txn.commit();
  return tasks;
}

vector<Task> Database::get_tasks_from(const User &teacher) {
  if (teacher.role != TEACHER)
    return {};

  vector<Task> tasks;

  // pqxx::work txn(conn);
  pqxx::params p;
  p.append(teacher.id);
  const std::string sql =
      "SELECT task_id, sg.group_id as group_id, t.teacher_id as teacher_id, "
      "title, "
      "description, ss.name as subject_name, sg.group_number "
      "FROM task "
      "JOIN study_group sg ON task.group_id = sg.group_id "
      "JOIN teacher t ON task.teacher_id = t.teacher_id "
      "JOIN subject ss ON task.subject_id = ss.subject_id "
      "WHERE task.teacher_id = $1";
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    Task task;
    task.id = row["task_id"].as<size_t>();
    task.title = row["title"].as<string>();
    task.description = row["description"].as<string>();
    task.subject = row["subject_name"].as<string>();
    task.teacher = teacher;
    task.group_number = row["group_number"].as<string>();
    tasks.push_back(task);
  }
  txn.commit();
  return tasks;
}

optional<Task> Database::get_task(size_t id) {
  // pqxx::work txn(conn);
  pqxx::params p;
  p.append(id);
  const std::string sql =
      "SELECT task_id, task.group_id as group_id, t.teacher_id as teacher_id, "
      "title, "
      "description, ss.name as subject_name, sg.group_number "
      "FROM task "
      "JOIN study_group sg ON task.group_id = sg.group_id "
      "JOIN teacher t ON task.teacher_id = t.teacher_id "
      "JOIN subject ss ON task.subject_id = ss.subject_id "
      "WHERE task_id = $1";
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    auto teacher_opt = get_teacher(row["teacher_id"].as<size_t>());
    if (!teacher_opt)
      continue;
    User teacher = *teacher_opt;
    auto report = get_report(row["task_id"].as<size_t>());

    Task task;
    task.id = row["task_id"].as<size_t>();
    task.title = row["title"].as<string>();
    task.description = row["description"].as<string>();
    task.subject = row["subject_name"].as<string>();
    task.teacher = teacher;
    task.group_number = row["group_number"].as<string>();
    task.report = report;
    txn.commit();
    return task;
  }
  txn.commit();
  return {};
}

vector<Report> Database::get_reports_for(const User &teacher) {
  if (teacher.role != TEACHER)
    return {};

  vector<Report> reports;
  // pqxx::work txn(conn);
  pqxx::params p;
  p.append(teacher.id);

  const std::string sql =
      "SELECT r.report_id, r.task_id, r.student_id, r.text, r.status, r.grade, "
      "st.full_name   AS student_name, "
      "sub.name       AS subject_name "
      "FROM report r "
      "JOIN task tk      ON r.task_id = tk.task_id "
      "JOIN teacher tr   ON tk.teacher_id = tr.teacher_id "
      "JOIN student st   ON r.student_id = st.student_id "
      "JOIN subject sub  ON tk.subject_id = sub.subject_id "
      "WHERE tr.teacher_id = $1";

  pqxx::result res = txn.exec(sql, p);
  for (const auto &row : res) {
    Report rep;
    rep.id = row["report_id"].as<size_t>();
    rep.task_id = row["task_id"].as<size_t>();
    rep.student_id = row["student_id"].as<size_t>();
    rep.student_name = row["student_name"].as<string>();
    rep.text = row["text"].as<string>();
    rep.status =
        row["status"].as<string>() == "ACCEPTED"
            ? Report::ACCEPTED
            : (row["status"].as<string>() == "REJECTED" ? Report::REJECTED
                                                        : Report::SENT);
    rep.grade = row["grade"].is_null() ? 0 : row["grade"].as<int>();

    reports.push_back(std::move(rep));
  }

  txn.commit();
  return reports;
}

bool Database::insert_report(Report report) {
  // pqxx::work txn(conn);
  pqxx::params p;
  p.append(report.task_id);
  p.append(report.student_id);
  p.append(report.text);
  p.append(report.status == Report::SENT
               ? "SENT"
               : (report.status == Report::REJECTED ? "REJECTED" : "ACCEPTED"));
  p.append(nullptr);

  const std::string sql =
      "INSERT INTO report (task_id, student_id, text, status, grade) "
      "VALUES ($1, $2, $3, $4, $5)";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Inserted report for task " << report.task_id
                          << " student " << report.student_id;
  return true;
}

vector<Report> Database::get_reports_for(const size_t &task_id) {
  vector<Report> reports;
  // pqxx::work txn(conn);
  pqxx::params p;
  p.append(task_id);

  const std::string sql =
      "SELECT r.report_id, r.task_id, r.student_id, r.text, r.status, r.grade, "
      "st.full_name   AS student_name, "
      "sub.name       AS subject_name "
      "FROM report r "
      "JOIN task tk      ON r.task_id = tk.task_id "
      "JOIN teacher tr   ON tk.teacher_id = tr.teacher_id "
      "JOIN student st   ON r.student_id = st.student_id "
      "JOIN subject sub  ON tk.subject_id = sub.subject_id "
      "WHERE tk.task_id = $1";

  pqxx::result res = txn.exec(sql, p);
  for (const auto &row : res) {
    Report rep;
    rep.id = row["report_id"].as<size_t>();
    rep.task_id = row["task_id"].as<size_t>();
    rep.student_id = row["student_id"].as<size_t>();
    rep.student_name = row["student_name"].as<string>();
    rep.text = row["text"].as<string>();
    rep.status =
        row["status"].as<string>() == "ACCEPTED"
            ? Report::ACCEPTED
            : (row["status"].as<string>() == "REJECTED" ? Report::REJECTED
                                                        : Report::SENT);
    rep.grade = row["grade"].is_null() ? 0 : row["grade"].as<int>();

    reports.push_back(std::move(rep));
  }

  txn.commit();
  return reports;
}

bool Database::insert_task(Task task) {
  auto o_gid = get_group_id(task.group_number);
  if (!o_gid) {
    BOOST_LOG_TRIVIAL(warning)
        << "Attempt to insert task with invalid group number "
        << task.group_number;
    return false;
  }
  size_t gid = *o_gid;
  auto o_subject_id = get_subject_id(task.subject);
  if (!o_subject_id) {
    BOOST_LOG_TRIVIAL(warning)
        << "Attempt to insert task with invalid subject " << task.subject;
    return false;
  }
  size_t subject_id = *o_subject_id;

  BOOST_LOG_TRIVIAL(trace) << "Inserting task \"" << task.title << "\""
                           << "With group_id=" << gid
                           << " teacher_id=" << task.teacher.id
                           << " subject_id=" << subject_id;
  pqxx::params p;
  p.append(gid);
  p.append(task.teacher.id);
  p.append(subject_id);
  p.append(task.title);
  p.append(task.description);

  const std::string sql =
      "INSERT INTO task (group_id, teacher_id, subject_id, title, description) "
      "VALUES ($1, $2, $3, $4, $5)";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Inserted task \"" << task.title << "\"";
  return true;
}

bool Database::update_grade(Report report) {
  if (!report.id) {
    BOOST_LOG_TRIVIAL(warning) << "Attempt to update report without id";
    return false;
  }

  pqxx::params p;
  string status =
      report.status == Report::ACCEPTED
          ? "ACCEPTED"
          : (report.status == Report::REJECTED ? "REJECTED" : "SENT");
  p.append(status);
  if (report.status == Report::ACCEPTED)
    p.append(report.grade);
  else
    p.append(nullptr);
  p.append(report.id);

  const std::string sql =
      "UPDATE report "
      "SET grade = $2, "
      " status = $1   "
      "WHERE report_id = $3";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Report " << report.id << " updated: status="
                          << (report.status == Report::ACCEPTED
                                  ? "ACCEPTED"
                                  : (report.status == Report::REJECTED
                                         ? "REJECTED"
                                         : "SENT"))
                          << ", grade=" << report.grade;
  return true;
}

optional<Report> Database::get_report(size_t id) {
  pqxx::params p;
  p.append(id);

  const std::string sql =
      "SELECT r.report_id, r.task_id, r.student_id, r.text, r.status, r.grade, "
      "st.full_name   AS student_name, "
      "sub.name       AS subject_name "
      "FROM report r "
      "JOIN task tk      ON r.task_id = tk.task_id "
      "JOIN student st   ON r.student_id = st.student_id "
      "JOIN subject sub  ON tk.subject_id = sub.subject_id "
      "WHERE r.report_id = $1";

  pqxx::result res = txn.exec(sql, p);
  if (res.empty()) {
    txn.commit();
    return {};
  }

  const auto &row = res.front();
  Report rep;
  rep.id = row["report_id"].as<size_t>();
  rep.task_id = row["task_id"].as<size_t>();
  rep.student_id = row["student_id"].as<size_t>();
  rep.text = row["text"].as<string>();
  rep.status = row["status"].as<string>() == "ACCEPTED" ? Report::ACCEPTED
                                                        : Report::SENT;
  rep.grade = row["grade"].is_null() ? 0 : row["grade"].as<int>();
  rep.student_name = row["student_name"].as<string>();

  txn.commit();
  return rep;
}

bool Database::fail() { return failed; }
