#pragma once

#include <memory>
#include <utility>
#include <folly/Function.h>
#include <folly/FBVector.h>
#include "language/topper.hpp"

namespace cyascript {
    class CyaScript_Parser {
        std::pair<string, string> multiline_comment{"/*", "*/"};
        string singleline_comment{"//"};

        string::iterator pos, endd;

        auto size() {
            return endd - pos;
        }

        int line, column;
        const char * filename;
        folly::fbvector<TokenPtr> matched_stack;

    public:
        CyaScript_Parser() = default;

        bool nowable() {
            return pos != endd;
        }

        bool capture(string::iterator st, Token_Type token_type, int prev_line, int prev_col) {
            matched_stack.push_back(std::make_shared<Token>(string{st, pos}, token_type, filename, prev_line, prev_col, line, column));
            return true;
        }

        template<class F, class...Args>
        folly::Function<bool()> use_capture(Token_Type token_type, F && func, Args&&...args) {
            return [&]() {
                auto st = pos;
                int prev_col = column;
                int prev_line = line;
                if ((func)(std::forward<Args>(args)...)) {
                    capture(st, token_type, prev_line, prev_col);
                    return true;
                }
                return false;
            };
        }

        template<class F, class...Args>
        folly::Function<bool()> use_capture(Token_Type token_type, F && func, CyaScript_Parser * self, Args&&...args) {
            return [&]() {
                auto st = pos;
                int prev_col = column;
                int prev_line = line;
                if ((self->*func)(std::forward<Args>(args)...)) {
                    capture(st, token_type, prev_line, prev_col);
                    return true;
                }
                return false;
            };
        }
#define use_capture_(token_type, func, ...) use_capture(token_type, &CyaScript_Parser::func, this, ##__VA_ARGS__)

        bool skip_whitespaces() {
            bool res = false;
            while (nowable()) {
                if ((*pos == ' ') || (*pos == '\t')) {
                    ++ pos;
                    ++ column;
                    res = true;
                } else if (skip_comments()) {
                    res = true;
                } else {
                    break;
                }
            }
            return res;
        }

        template<class F, class...Args>
        folly::Function<bool()> use_skip_ws(F && func, Args&&...args) {
            return [&]() {
                skip_whitespaces();
                return (func)(std::forward<Args>(args)...);
            };
        }

        template<class F, class...Args>
        folly::Function<bool()> use_skip_ws(F && func, CyaScript_Parser * self, Args&&...args) {
            return [&]() {
                skip_whitespaces();
                return (self->*func)(std::forward<Args>(args)...);
            };
        }
#define use_skip_ws_(func, ...) use_skip_ws(&CyaScript_Parser::func, this, ##__VA_ARGS__)

        bool match_char(char c) {
            bool res = false;
            if (nowable() && (*pos == c)) {
                ++ pos;
                ++ column;
                res = true;
            }
            return res;
        }

        bool match_symbol(const string & str) {
            auto len = str.size();
            if (size() >= len) {
                for (int i = 0; i < len; ++ i) {
                    if (*(pos + i) != str[i]) {
                        return false;
                    }
                }
                pos = (pos + len);
                column += len;
                return true;
            }
            return false;
        }

        bool match_expression(const string & str, bool disallow_prev) {
            skip_whitespaces();
            auto st = pos;
            int prev_col = column;
            int prev_line = line;
            if (match_symbol(str)) {
                // todo: fix this.  Hacky workaround for preventing substring matches
                if (nowable() && (disallow_prev == false) && ((*pos == '+') || (*pos == '-') || (*pos == '*') || (*pos == '/') || (*pos == '=') || (*pos == '.'))) {
                    pos = st;
                    column = prev_col;
                    line = prev_line;
                    return false;
                }
                return true;
            }
            return false;
        }

        bool eol() {
            bool res = false;
            if (nowable()) {
                if ((match_symbol("\r\n") || match_char('\n'))) {
                    ++ line;
                    column = 1;
                    res = true;
                } else if (match_char(';')) {
                    res = true;
                }
            }
            return res;
        }

        bool skip_comments() {
            bool res = false;
            if (nowable()) {
                if (match_symbol(multiline_comment.first)) {
                    while (nowable()) {
                        if (match_symbol(multiline_comment.second)) {
                            res = true;
                            break;
                        } else if (!eol()) {
                            ++ column;
                            ++ pos;
                        }
                    }
                    res = true;
                } else if (match_symbol(singleline_comment)) {
                    while (nowable()) {
                        if (eol()) {
                            res = true;
                        } else {
                            ++ column;
                            ++ pos;
                        }
                    }
                    res = true;
                }
            }
            return res;
        }

        bool id() {
            bool res = false;
            if (nowable() && (isalpha(*pos) || isdigit(*pos) || (*pos == '_'))) {
                res = true;
                while (nowable() && (isalpha(*pos) || isdigit(*pos) || (*pos == '_'))) {
                    ++ pos;
                    ++ column;
                }
            }
            return res;
        }

        /**
         * Checks for a node annotation of the form "#<annotation>"
         */
        bool annotation() {
            skip_whitespaces();
            auto st = pos;
            int prev_col = column;
            int prev_line = line;
            if (match_char('#')) {
                while (match_char('#')) {
                    while (nowable()) {
                        if (eol()) {
                            break;
                        } else {
                            ++ column;
                            ++ pos;
                        }
                    }
                }
                capture(st, Token_Type::Annotation, prev_line, prev_col);
                return true;
            }
            return false;
        }

        bool keyword(string str) {
            skip_whitespaces();
            auto st = pos;
            int prev_line = line;
            int prev_col = column;
            if (match_symbol(str)) {
                if (nowable() && (isalpha(*pos) || isdigit(*pos) || (*pos == '_'))) {
                    pos = st;
                    column = prev_col;
                    line = prev_line;
                    return false;
                }
                return true;
            }
            return false;
        }

        bool equation() {
            bool res = false;
            int prev_len = matched_stack.size();

            if (expression()) {
                res = true;
                if (match_expression()) {
                    ;
                }
            }
        }

        bool arg_list() {
            bool res = false;
            int prev_len = matched_stack.size();

            if ()
        }

        /**
         * Reads a function definition
         */
        bool Fun() {
            bool res = false;
            bool is_annotation = false;
            bool is_method = false;
            TokenPtr annotation_token;

            if (annotation()) {
                while (eol()) {
                }
                annotation_token = matched_stack.back();
                matched_stack.pop_back();
                is_annotation = true;
            }

            int len = matched_stack.size();

            if (keyword("fun")) {
                res = true;

                if (!use_skip_ws(use_capture_(Token_Type::Id, id))) {
                    throw Eval_Error("Missing function name in definition", File_Position(line, column), filename);
                }

                if (use_skip_ws_(match_symbol, "::")) {
                    is_method = true;

                    if (!use_skip_ws(use_capture_(Token_Type::Id, id))) {
                        throw Eval_Error("Missing method name in definition", File_Position(line, column), filename);
                    }
                }

                if (use_skip_ws_(match_char, '(')) {
                    ;
                }
            }
        }

        bool statements() {
            ;
        }

        bool parse(string input, const char * fname) {
            pos = input.begin();
            endd = input.end();
            line = column = 1;
            filename = fname;

            if ((input.size() > 1) && (input[0] == '#') && (input[1] == '!')) {
                while ((pos != endd) && (!use_skip_ws_(eol))) {
                    ++ pos;
                }
            }

            if (statements()) {
                ;
            }
        }
#undef use_skip_ws_
    };
} // namespace cyascript
