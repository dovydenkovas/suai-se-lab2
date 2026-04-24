#include <algorithm>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <crypt.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>

#include "auth.h"

using std::optional;
using std::string;
using std::vector;

struct Record {
  string login;
  string token;
  std::chrono::system_clock::time_point expires;
};

const std::chrono::seconds TTL = std::chrono::seconds(24 * 3600);
const char DELIMETER = ';';
const string tokens_path = "/tmp/tokens.db";
Record auth_cache;

bool auth::check_password(const string &password, const string &hash) {
  BOOST_LOG_TRIVIAL(trace) << "Password: " << password
                           << "Hash: " << auth::hash_password(password);
  return auth::hash_password(password) == hash.c_str();
}

string auth::hash_password(const string &password) {
  const char *salt = "passwd_salt";
  return crypt(password.c_str(), salt);
}

optional<Record> parse_line(const string &line) {
  std::istringstream ss(line);
  string token, login, epoch_str;

  if (!getline(ss, token, DELIMETER))
    return {};
  if (!getline(ss, login, DELIMETER))
    return {};
  if (!getline(ss, epoch_str))
    return {};

  try {
    time_t epoch = static_cast<time_t>(stoll(epoch_str));
    Record r{token, login, std::chrono::system_clock::from_time_t(epoch)};
    return r;
  } catch (...) {
    return {};
  }
}

string serialize(const Record &rec) {
  time_t epoch = std::chrono::system_clock::to_time_t(rec.expires);
  return rec.token + DELIMETER + rec.login + DELIMETER +
         std::to_string(static_cast<long long>(epoch));
}

vector<Record> load_all() {
  vector<Record> out;
  std::ifstream in(tokens_path);
  if (!in)
    return out;

  string line;
  while (getline(in, line)) {
    if (auto opt = parse_line(line))
      out.push_back(*opt);
  }
  return out;
}

void store_all(const vector<Record> &vec) {
  string tmp = tokens_path + ".tmp";
  std::ofstream out(tmp, std::ios::trunc);
  if (!out)
    return;

  for (const auto &r : vec)
    out << serialize(r) << '\n';
  out.close();

  remove(tokens_path.c_str());
  rename(tmp.c_str(), tokens_path.c_str());
}

string make_token() {
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 255);
  unsigned char buf[32];
  for (auto &b : buf)
    b = static_cast<unsigned char>(dist(rd));

  std::ostringstream oss;
  for (unsigned char b : buf)
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
  return oss.str();
}

string auth::new_token(const string &login) {
  Record rec;
  rec.token = make_token();
  rec.login = login;
  rec.expires = std::chrono::system_clock::now() + TTL;

  auto records = load_all();
  records.push_back(std::move(rec));
  store_all(records);
  return records.back().token;
}

bool auth::check_token(const string &token) {
  auto records = load_all();
  auto now = std::chrono::system_clock::now();

  for (const auto &r : records) {
    if (r.token == token && r.expires > now)
      return true;
  }
  return false;
}

string auth::login_by_token(const string &token) {
  auto records = load_all();
  auto now = std::chrono::system_clock::now();

  for (const auto &r : records) {
    if (r.token == token && r.expires > now)
      return r.login;
  }
  return {};
}

void auth::remove_token(const string &token) {
  auto records = load_all();
  auto it = std::remove_if(records.begin(), records.end(),
                           [&](const Record &r) { return r.token == token; });
  if (it != records.end()) {
    records.erase(it, records.end());
    store_all(records);
  }
}

size_t auth::remove_expired() {
  auto records = load_all();
  auto now = std::chrono::system_clock::now();

  size_t before = records.size();

  records.erase(remove_if(records.begin(), records.end(),
                          [&](const Record &r) { return r.expires <= now; }),
                records.end());

  size_t after = records.size();
  if (after != before)
    store_all(records);

  return before - after;
}
