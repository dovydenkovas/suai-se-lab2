#pragma once

#include <optional>
#include <string>
#include <pqxx/pqxx>
#include "entities.h"

using std::string;
using std::vector;

class Database {
private:
    // pqxx::connection conn;
    bool failed;

public:
    Database(string connection);

    std::optional<User> get_user_by_login(string login);
    vector<Task> get_tasks_for(const User &student);
    std::optional<Task> get_task(size_t id);
    bool fail();

private:
    void prepare_statements();
};
