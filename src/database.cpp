#include <optional>
#include <string>
#include <vector>

#include "database.h"
#include "entities.h"

using std::optional;
using std::string;
using std::vector;

#include <pqxx/pqxx>

Database::Database(string connection) /* : conn{connection}*/ {
  failed = false;
  // if (!conn.is_open()) {
    // failed = true;
    // return;
  // }
  // prepare_statements();
}

void Database::prepare_statements() {
  // pqxx::work txn(conn);
  // txn.conn().prepare("find_student",
  //                    "SELECT s.student_id, s.full_name, s.user_password, "
  //                    "g.group_number FROM student s JOIN study_group g ON "
  //                    "s.group_id=g.group_id WHERE s.user_login = $1");
  // txn.conn().prepare("find_teacher",
  //                    "SELECT teacher_id, full_name, user_password FROM teacher "
  //                    "WHERE user_login = $1");
  // // Для student_tasks используем group_number
  // txn.conn().prepare(
  //     "student_tasks",
  //     "SELECT t.task_id, t.title, s.name as subject_name, tr.full_name as "
  //     "teacher_name "
  //     "FROM task t JOIN subject s ON t.subject_id=s.subject_id JOIN teacher tr "
  //     "ON t.teacher_id=tr.teacher_id "
  //     "JOIN study_group g ON t.group_id=g.group_id WHERE g.group_number = $1");
  // txn.conn().prepare(
  //     "teacher_tasks",
  //     "SELECT t.task_id, t.title, s.name as subject_name, tr.full_name as "
  //     "teacher_name "
  //     "FROM task t JOIN subject s ON t.subject_id=s.subject_id JOIN teacher tr "
  //     "ON t.teacher_id=tr.teacher_id WHERE t.teacher_id = $1");
  // txn.conn().prepare(
  //     "get_report_for_student",
  //     "SELECT r.report_id, r.text, r.status::text as status, r.grade FROM "
  //     "report r WHERE r.task_id=$1 AND r.student_id=$2");
  // txn.conn().prepare(
  //     "task_by_id",
  //     "SELECT t.title, t.description, s.name as subject_name, tr.full_name as "
  //     "teacher_name, g.group_number "
  //     "FROM task t JOIN subject s ON t.subject_id=s.subject_id JOIN teacher tr "
  //     "ON t.teacher_id=tr.teacher_id JOIN study_group g ON "
  //     "t.group_id=g.group_id WHERE t.task_id=$1");
  // txn.conn().prepare(
  //     "insert_report",
  //     "INSERT INTO report(task_id, student_id, text) VALUES($1,$2,$3)");
  // txn.conn().prepare(
  //     "teacher_reports",
  //     "SELECT r.report_id, t.title as task_title, s.name as subject_name, "
  //     "st.full_name as student_name, g.group_number, r.status::text as status, "
  //     "r.grade "
  //     "FROM report r JOIN task t ON r.task_id=t.task_id JOIN subject s ON "
  //     "t.subject_id=s.subject_id JOIN student st ON r.student_id=st.student_id "
  //     "JOIN study_group g ON st.group_id=g.group_id WHERE t.teacher_id=$1");
  // txn.conn().prepare(
  //     "student_reports",
  //     "SELECT r.report_id, t.title as task_title, s.name as subject_name, "
  //     "st.full_name as student_name, g.group_number, r.status::text as status, "
  //     "r.grade "
  //     "FROM report r JOIN task t ON r.task_id=t.task_id JOIN subject s ON "
  //     "t.subject_id=s.subject_id JOIN student st ON r.student_id=st.student_id "
  //     "JOIN study_group g ON st.group_id=g.group_id WHERE r.student_id=$1");
  // txn.conn().prepare(
  //     "report_by_id",
  //     "SELECT r.text, r.status::text as status, r.grade, t.title as "
  //     "task_title, t.description as task_description, s.name as subject_name, "
  //     "st.full_name as student_name, g.group_number, t.teacher_id "
  //     "FROM report r JOIN task t ON r.task_id=t.task_id JOIN subject s ON "
  //     "t.subject_id=s.subject_id JOIN student st ON r.student_id=st.student_id "
  //     "JOIN study_group g ON st.group_id=g.group_id WHERE r.report_id=$1");
  // txn.conn().prepare("report_owner_check",
  //                    "SELECT t.teacher_id FROM report r JOIN task t ON "
  //                    "r.task_id=t.task_id WHERE r.report_id=$1");
  // txn.conn().prepare("update_report_status_grade",
  //                    "UPDATE report SET status = $1::report_status, grade = $2 "
  //                    "WHERE report_id = $3");
  // txn.conn().prepare("update_report_status",
  //                    "UPDATE report SET status = $1::report_status, grade = "
  //                    "NULL WHERE report_id = $2");
  // txn.commit();
}

optional<User> Database::get_user_by_login(string login) {
    if (login == "teacher") {
        return User {1, login, "paN8aiEIonqJE", "Иван Иванович Иванов", TEACHER, {}};
    } else if (login == "student") {
       return User {2, login, "paN8aiEIonqJE", "Петр Петрович Петров", STUDENT, {}};
    } else {
        return {};
    }
  // pqxx::work txn(conn);
  // pqxx::result res = txn.exec_prepared("find_student", login);
  // if (!res.empty()) {
  //   return User{
  //       res[0]["student_id"].as<int>(), res[0]["full_name"].as<string>(),
  //       res[0]["user_password"].as<string>(), res[0]["group_number"].c_str()};
  // }
  // txn.commit();
  // throw std::runtime_error("User not found");
}

vector<Task> Database::get_tasks_for(const User &student) {
    return {get_task(0).value()};

//   pqxx::work txn(conn);
//   pqxx::result res = txn.exec("student_tasks", student.group_number);
//   vector<Task> tasks;
//   for (const auto &row : res) {
//     Task task{row["task_id"].as<size_t>(), row["title"].as<string>(),
//               Subject{row["subject_name"].as<std::string>()},
//               Teacher{row["teacher_name"].as<std::string>()}};
//     tasks.push_back(task);
//   }
//   txn.commit();
//   return tasks;
}

optional<Task> Database::get_task(size_t id) {
    switch (id) {
        case 0:
            return Task{0, "Z3431", "Иван Иванович Иванов", "Математика", "Контрольная работа", " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam imperdiet varius tincidunt. Cras iaculis consectetur nunc, sodales viverra mauris bibendum ac. Pellentesque quis faucibus lectus. Proin ac nibh placerat, tincidunt ipsum eget, vestibulum nisi. Nam quis convallis lorem. Mauris aliquet in lorem ut lacinia. Ut eu libero at nisi elementum dapibus ac sed felis. Phasellus auctor justo massa, sit amet dictum justo tempus a. Mauris mattis odio non mi congue, eget consectetur lacus venenatis. Praesent consectetur non quam eget finibus.           Morbi vitae gravida augue. Proin venenatis risus sit amet purus convallis rhoncus. Mauris rhoncus ligula leo, nec gravida orci ornare nec. Aliquam consequat metus ipsum, quis posuere turpis pulvinar vitae. Quisque porttitor id tellus non hendrerit. Donec nec mi in ex porta luctus. Quisque id libero id lacus porta mattis vel at augue. In ullamcorper tincidunt nisl non pharetra. Vivamus placerat rhoncus tortor, in sagittis lectus facilisis dictum. Mauris mollis elit a porta convallis. Praesent a felis tincidunt, hendrerit ipsum eget, finibus lectus. Pellentesque gravida luctus libero, eu mollis urna imperdiet ut. Ut diam augue, viverra nec convallis in, malesuada vel mauris. In hac habitasse platea dictumst. Pellentesque non euismod ipsum, sed vehicula dolor. Aliquam eu ligula at magna fringilla sollicitudin sed nec arcu. "};
        default:
           return {};
    }

//   pqxx::work txn(conn);
//   pqxx::result res = txn.exec("task_by_id", id);
//   if (!res.empty()) {
//     return Task{id,
//                 res[0]["title"].as<string>(),
//                 res[0]["description"].as<string>(),
//                 Subject{res[0]["subject_name"].as<string>()},
//                 Teacher{res[0]["teacher_name"].as<string>()},
//                 res[0]["group_number"].as<int>()};
//   }
//   txn.commit();
//   throw std::runtime_error("Task not found");
}

bool Database::fail() { return failed; }
