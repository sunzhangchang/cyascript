#include <iostream>
#include <filesystem>
#include <fmt/format.h>
#include <lyra/lyra.hpp>
#include <folly/FileUtil.h>
#include <folly/FBString.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

struct compile_cmd {
    bool show_help = false;
    folly::fbstring file_path;

    compile_cmd(lyra::cli & cli) {
        cli
            .add_argument(
                lyra::command("compile",
                    [this](const lyra::group & g) { this->compile(g); }
                )
                .help("Compile CyaScript source code")
                .add_argument(
                    lyra::help(show_help)
                )
                .add_argument(
                    lyra::arg(file_path, "file_path")
                    .required()
                    .help("CyaScript source code")
                )
            );
    }

    void compile(const lyra::group & g) {
        if (show_help) {
            cout << g << endl;
            exit(0);
        }

        std::filesystem::path path{file_path.toStdString()};
        if (!path.has_extension()) {
            path.replace_extension(".cyas");
            file_path = path.string();
        }

        if (!std::filesystem::exists(path)) {
            cerr << fmt::format("File \"{}\" does not exist.", file_path) << endl;
            exit(1);
        }

        folly::fbstring context;
        auto flg = folly::readFile(file_path.toStdString().c_str(), context);
        if (!flg) {
            cerr << fmt::format("Read file \"{}\" failed.", file_path) << endl;
            exit(1);
        } else {
            // cout << context << endl;
            ;
        }
    }
};

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
    cerr.tie(nullptr);
    bool show_help = false;
    auto cli =
        lyra::cli()
        .add_argument(
            lyra::help(show_help)
        );

    compile_cmd cpl{cli};

    auto res = cli.parse({argc, argv});

    if (!res) {
        cerr << res.message() << endl;
        exit(1);
    }

    if (show_help) {
        cout << cli << endl;
        exit(0);
    }
    return 0;
}
