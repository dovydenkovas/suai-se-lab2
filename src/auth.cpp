#include <boost/json/object.hpp>
#include <string>
#include "auth.h"

using std::string;

bool auth::check_password(string password, string hash) {
    return password == hash;
}

string new_token(string login) {
    return login;
}

bool auth::check_token(string token) {
    return !token.empty();
}

string auth::login_by_token(string token) {
    return token;
}

void auth::remove_token(string token) {
    // ...
}
