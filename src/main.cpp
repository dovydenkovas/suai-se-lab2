#include <fstream>
#include <iostream>

using std::cout;
using std::ifstream;

int main() {
    cout << "Content-Type: text/html\r\n\r\n";
    ifstream f("/var/www/ecampus/html/index.html");
    if (!f) {
        cout << "<html><body><h1>500 Internal Server Error</h1><p>Cannot open page.html</p></body></html>";
        return 1;
    }
    cout << f.rdbuf();
    return 0;
}
