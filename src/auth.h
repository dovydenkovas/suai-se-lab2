#pragma once
#include <string>

using std::string;

namespace auth {
    bool check_password(const string &password, const string &hash);
    string hash_password(const string &password);
    string new_token(const string &login);
    bool check_token(const string &token);
    string login_by_token(const string &token);
    void remove_token(const string &token);
    size_t remove_expired();
}
