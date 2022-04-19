#include <regex>
#include <string>
#include <iostream>
#include <folly/String.h>
#include <folly/FBString.h>
#include <folly/FBVector.h>

int main(int argc, char** argv) {
    const folly::fbstring code{R"(println("hello world!")
    )"};

    folly::fbvector<folly::StringPiece> lines;
    folly::split("\n", code, lines);

    for (auto & line : lines) {
        // std::cout << line << std::endl;
        line = folly::trimWhitespace(line);

        if (line.empty() || line.startsWith('#')) {
            continue;
        }

        auto reg = std::regex{R"(println\(".*"\))"};
        std::smatch match_result;
        auto t = line.toString();
        if (std::regex_match(t, match_result, reg)) {
            std::cout << "---------------------" << std::endl;
            std::cout << match_result[0].str() << std::endl;
            auto s = match_result[0].str();
            for (auto it = s.begin(); it != s.end(); ++ it) {
            }
        }
    }
    return 0;
}
