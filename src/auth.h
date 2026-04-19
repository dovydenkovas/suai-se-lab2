#include <string>

using std::string;

namespace auth {
    struct {
        string login;
        string token;
        string expires_at;
    } _cache;

    bool check_password(string password, string hash);
    string new_token(string login);
    bool check_token(string token);
    string login_by_token(string token);
    void remove_token(string token);
}

