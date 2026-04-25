#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <string>

#include "apihandler.h"
#include "database.h"
#include "ecampus.h"


using std::string;

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
