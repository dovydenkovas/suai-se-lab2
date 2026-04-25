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

Database::Database(string connection) : conn{connection}, failed{false} {}

void Database::prepare_statements() {}

optional<User> Database::get_user_by_login(string login) {
  const std::string sql =
      "SELECT teacher_id, full_name, user_login, user_password FROM teacher "
      "WHERE user_login = $1";

  pqxx::params p;
  p.append(login);
  pqxx::nontransaction txn{conn};
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    User teacher;
    teacher.id = row["teacher_id"].get<size_t>().value();
    teacher.full_name = row["full_name"].get<string>().value();
    teacher.login = row["user_login"].get<string>().value();
    teacher.password = row["user_password"].get<string>().value();
    teacher.role = TEACHER;

    BOOST_LOG_TRIVIAL(trace) << "Find teacher with id: " << teacher.as_json();
    return {teacher};
  }

  const std::string sql_s =
      "SELECT student_id, full_name, user_login, user_password, "
      "sg.group_number "
      "FROM student s JOIN study_group sg ON s.group_id = sg.group_id   "
      "WHERE user_login = $1";

  pqxx::result res_s = txn.exec(sql_s, p);

  for (const auto &row : res_s) {
    User student;
    student.id = row["student_id"].get<size_t>().value();
    student.group_number = row["group_number"].get<string>().value();
    student.login = row["user_login"].get<string>().value();
    student.full_name = row["full_name"].get<string>().value();
    student.password = row["user_password"].get<string>().value();
    student.role = STUDENT;

    BOOST_LOG_TRIVIAL(trace) << "Find student with id: " << student.as_json();
    return {student};
  }

  return {};
}

vector<Task> Database::get_tasks_for(const User &student) {
  if (student.role != STUDENT)
    return {};

  vector<Task> tasks;

  pqxx::work txn(conn);
  pqxx::params p;
  p.append(student.group_number);
  const std::string sql = "SELECT task_id, group_id, teacher_id, title, "
                          "description, ss.name as subject_name FROM task "
                          "JOIN study_group sg ON group_id = sg.group_id "
                          "JOIN teacher as t ON teacher_id = t.teacher_id "
                          "JOIN subject ss ON ss.teacher_id = t.teacher_ud "
                          "WHERE group_number = $1";
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    Task task{
        row["task_id"].as<size_t>(),    row["group_number"].as<string>(),
        row["teacher_id"].as<size_t>(), row["subject_name"].as<string>(),
        row["title"].as<string>(),      row["description"].as<string>(),
    };
    tasks.push_back(task);
  }
  txn.commit();
  return tasks;
}

vector<Task> Database::get_tasks_from(const User &teacher) {
  if (teacher.role != TEACHER)
    return {};

  vector<Task> tasks;

  pqxx::work txn(conn);
  pqxx::params p;
  p.append(teacher.id);
  const std::string sql = "SELECT task_id, group_id, teacher_id, title, "
                          "description, ss.name as subject_name FROM task "
                          "JOIN study_group sg ON group_id = sg.group_id "
                          "JOIN teacher as t ON teacher_id = t.teacher_id "
                          "JOIN subject ss ON ss.teacher_id = t.teacher_ud "
                          "WHERE teacher_id = $1";
  pqxx::result res = txn.exec(sql, p);

  for (const auto &row : res) {
    Task task{
        row["task_id"].as<size_t>(),    row["group_number"].as<string>(),
        row["teacher_id"].as<size_t>(), row["subject_name"].as<string>(),
        row["title"].as<string>(),      row["description"].as<string>(),
    };
    tasks.push_back(task);
  }
  txn.commit();
  return tasks;
}

optional<Task> Database::get_task(size_t id) {
  pqxx::work txn(conn);
  pqxx::params p;
  p.append(id);
  const std::string sql = "SELECT task_id, group_id, teacher_id, title, "
                          "description, ss.name as subject_name FROM task "
                          "JOIN study_group sg ON group_id = sg.group_id "
                          "JOIN teacher as t ON teacher_id = t.teacher_id "
                          "JOIN subject ss ON ss.teacher_id = t.teacher_ud "
                          "WHERE task_id = $1";
  pqxx::result res = txn.exec(sql, id);

  for (const auto &row : res) {
    Task task{
        row["task_id"].as<size_t>(),    row["group_number"].as<string>(),
        row["teacher_id"].as<size_t>(), row["subject_name"].as<string>(),
        row["title"].as<string>(),      row["description"].as<string>(),
    };
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
  pqxx::work txn(conn);
  pqxx::params p;
  p.append(teacher.id);

  const std::string sql =
      "SELECT r.report_id, r.task_id, r.student_id, r.text, r.status, r.grade, "
      "st.full_name   AS student_name, "
      "t.title        AS task_title, "
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
    rep.text = row["text"].as<string>();
    rep.status = row["status"].as<string>() == "ACCEPTED" ? Report::ACCEPTED : Report::SENT;
    rep.grade = row["grade"].is_null()
                    ? 0
                    : row["grade"].as<int>();

    // Дополнительные «читаемые» поля – не хранятся в таблице report,
    // но удобно возвращать клиенту.
    // rep.student_name = row["student_name"].as<string>();
    // rep.task_title = row["task_title"].as<string>();
    // rep.subject_name = row["subject_name"].as<string>();

    reports.push_back(std::move(rep));
  }

  txn.commit();
  return reports;
}

void Database::insert_report(Report report) {
  pqxx::work txn(conn);
  pqxx::params p;
  p.append(report.task_id);
  p.append(report.student_id);
  p.append(report.text);
  p.append(report.status); // ENUM как строка
  if (report.grade.has_value())
    p.append(report.grade.value());
  else
    p.append(nullptr); // NULL в поле grade

  const std::string sql =
      "INSERT INTO report (task_id, student_id, text, status, grade) "
      "VALUES ($1, $2, $3, $4, $5)";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Inserted report for task " << report.task_id
                          << " student " << report.student_id;
}

void Database::insert_task(Task task) {
  pqxx::work txn(conn);
  pqxx::params p;
  // task.group_number → нужно получить group_id
  // Предполагается, что в структуре Task уже хранится group_id и subject_id.
  p.append(task.group_id);   // int
  p.append(task.teacher_id); // int
  p.append(task.subject_id); // int
  p.append(task.title);
  p.append(task.description);

  const std::string sql =
      "INSERT INTO task (group_id, teacher_id, subject_id, title, description) "
      "VALUES ($1, $2, $3, $4, $5)";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Inserted task \"" << task.title << "\"";
}

void Database::update_grade(Report report) {
  if (!report.id) {
    BOOST_LOG_TRIVIAL(warning) << "Attempt to update report without id";
    return;
  }

  pqxx::work txn(conn);
  pqxx::params p;
  p.append(report.status); // NEW status
  // Если статус ACCEPTED – указываем grade, иначе NULL
  if (report.status == "ACCEPTED" && report.grade.has_value())
    p.append(report.grade.value());
  else
    p.append(nullptr);
  p.append(report.id.value());

  const std::string sql =
      "UPDATE report "
      "SET status = $1, "
      "    grade  = CASE WHEN $1 = 'ACCEPTED' THEN $2 ELSE NULL END "
      "WHERE report_id = $3";

  txn.exec(sql, p);
  txn.commit();

  BOOST_LOG_TRIVIAL(info) << "Report " << report.id.value()
                          << " updated: status=" << report.status << ", grade="
                          << (report.grade ? std::to_string(*report.grade)
                                           : "NULL");
}

optional<Report> Database::get_report(size_t id) {
  pqxx::work txn(conn);
  pqxx::params p;
  p.append(id);

  const std::string sql =
      "SELECT r.report_id, r.task_id, r.student_id, r.text, r.status, r.grade, "
      "st.full_name   AS student_name, "
      "t.title        AS task_title, "
      "sub.name       AS subject_name "
      "FROM report r "
      "JOIN task tk      ON r.task_id = tk.task_id "
      "JOIN student st   ON r.student_id = st.student_id "
      "JOIN subject sub  ON tk.subject_id = sub.subject_id "
      "WHERE r.report_id = $1";

  pqxx::result res = txn.exec(sql, p);
  if (!res.empty()) {
    const auto &row = res.front();
    Report rep;
    rep.id = row["report_id"].as<size_t>();
    rep.task_id = row["task_id"].as<size_t>();
    rep.student_id = row["student_id"].as<size_t>();
    rep.text = row["text"].as<string>();
    rep.status = row["status"].as<string>();
    rep.grade = row["grade"].is_null()
                    ? std::nullopt
                    : std::optional<int>{row["grade"].as<int>()};

    rep.student_name = row["student_name"].as<string>();
    rep.task_title = row["task_title"].as<string>();
    rep.subject_name = row["subject_name"].as<string>();

    txn.commit();
    return rep;
  }

  txn.commit();
  return {};
}

bool Database::fail() { return failed; }
