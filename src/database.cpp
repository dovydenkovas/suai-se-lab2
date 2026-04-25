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

optional<User> Database::get_user(size_t id) {
    if (id == 1) {
        return User {1, "teacher", "paN8aiEIonqJE", "Иван Иванович Иванов", TEACHER, {}};
    } else if (id  == 2) {
       return User {2, "student", "paN8aiEIonqJE", "Петр Петрович Петров", STUDENT, {}};
    } else {
        return {};
    }
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

vector<Task> Database::get_tasks_from(const User &teacher) {
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
            return Task{0, "Z3431", 0, "Математика", "Контрольная работа", " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam imperdiet varius tincidunt. Cras iaculis consectetur nunc, sodales viverra mauris bibendum ac. Pellentesque quis faucibus lectus. Proin ac nibh placerat, tincidunt ipsum eget, vestibulum nisi. Nam quis convallis lorem. Mauris aliquet in lorem ut lacinia. Ut eu libero at nisi elementum dapibus ac sed felis. Phasellus auctor justo massa, sit amet dictum justo tempus a. Mauris mattis odio non mi congue, eget consectetur lacus venenatis. Praesent consectetur non quam eget finibus.           Morbi vitae gravida augue. Proin venenatis risus sit amet purus convallis rhoncus. Mauris rhoncus ligula leo, nec gravida orci ornare nec. Aliquam consequat metus ipsum, quis posuere turpis pulvinar vitae. Quisque porttitor id tellus non hendrerit. Donec nec mi in ex porta luctus. Quisque id libero id lacus porta mattis vel at augue. In ullamcorper tincidunt nisl non pharetra. Vivamus placerat rhoncus tortor, in sagittis lectus facilisis dictum. Mauris mollis elit a porta convallis. Praesent a felis tincidunt, hendrerit ipsum eget, finibus lectus. Pellentesque gravida luctus libero, eu mollis urna imperdiet ut. Ut diam augue, viverra nec convallis in, malesuada vel mauris. In hac habitasse platea dictumst. Pellentesque non euismod ipsum, sed vehicula dolor. Aliquam eu ligula at magna fringilla sollicitudin sed nec arcu. "};
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

vector<Report> Database::get_reports_for(const User &teacher) {
  return {};
}

void Database::insert_report(Report report) {}
void Database::insert_task(Task task) {}
void Database::update_grade(Report report) {}
optional<Report> Database::get_report(size_t id) {
  return {};
}


bool Database::fail() { return failed; }
