#pragma once

#include <memory>
#include <utility>
#include <iostream>
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

        void build_matched(Token_Type type, int st) {
            if (st != (int)(matched_stack.size())) {
                TokenPtr t = std::make_shared<Token>("", type, filename, matched_stack[st]->start.line, matched_stack[st]->start.column, line, column);
                t->children.assign(matched_stack.begin() + st, matched_stack.end());
                matched_stack.erase(matched_stack.begin() + st, matched_stack.end());
                matched_stack.emplace_back(t);
            } else {
                // todo: fix the fact that a successful match that captured no tokens does't have any real start position
                matched_stack.emplace_back(std::make_shared<Token>("", type, filename, line, column, line, column));
            }
        }

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

        bool match_symbol(const string & str, bool is_capture, bool disallow_prev = false) {
            skip_whitespaces();
            auto st = pos;
            int prev_col = column;
            int prev_line = line;
            if (!is_capture) {
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
            } else {
                match_symbol(str, false, disallow_prev);
                capture(st, Token_Type::Str, prev_line, prev_col);
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
            std::cout << "id" << std::endl;
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
            std::cout << "annotation" << std::endl;
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
            std::cout << "keyword: " << str << std::endl;
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

        bool value() {
            return true;
        }

        /**
         * Reads a string of dot-notation accesses from input
         */
        bool dot_access() {
            bool res = false;

            int prev_stack_top = matched_stack.size();

            if (value()) {
                res = true;
                bool flg = false;
                while (match_symbol(".", false)) {
                    flg = true;
                    if (!value()) {
                        throw Eval_Error("Incomplete dot notation", File_Position{line, column}, filename);
                    }
                }
                if (flg) {
                    build_matched(Token_Type::Dot_Access, prev_stack_top);
                }
            }

            return res;
        }

        /**
         * Reads a string of multiplication/division/modulus from input
         */
        bool multiplicative() {
            bool res = false;

            int prev_stack_top = matched_stack.size();

            if (dot_access()) {
                res = true;
                bool flg = false;
                while (match_symbol("*", true)
                    || match_symbol("/", true)
                    || match_symbol("%", true)
                    ) {
                    flg = true;
                    if (!dot_access()) {
                        throw Eval_Error("Incomplete math expression", File_Position{line, column}, filename);
                    }
                }
                if (flg) {
                    build_matched(Token_Type::Multiplicative, prev_stack_top);
                }
            }

            return res;
        }

        /**
         * Reads a string of binary additions/subtractions from input
         */
        bool additive() {
            bool res = false;

            int prev_stack_top = matched_stack.size();

            if (multiplicative()) {
                res = true;
                bool flg = false;
                while (match_symbol("+", true, false)
                    || match_symbol("-", true, false)
                    ) {
                    flg = true;
                    if (!multiplicative()) {
                        throw Eval_Error("Incomplete math expression", File_Position{line, column}, filename);
                    }
                }
                if (flg) {
                    build_matched(Token_Type::Additive, prev_stack_top);
                }
            }

            return res;
        }

        /**
         * Reads a string of binary comparisons from input
         */
        bool comparison() {
            bool res = false;

            int prev_stack_top = matched_stack.size();

            if (additive()) {
                res = true;
                bool flg = false;
                while (match_symbol(">=", true, false)
                    || match_symbol(">", true, false)
                    || match_symbol("<=", true, false)
                    || match_symbol("<", true, false)
                    || match_symbol("==", true, false)
                    || match_symbol("!=", true, false)
                    ) {
                    flg = true;
                    if (!additive()) {
                        throw Eval_Error("Incomplete comparison expression", File_Position(line, column), filename);
                    }
                }

                if (flg) {
                    build_matched(Token_Type::Comparison, prev_stack_top);
                }
            }

            return res;
        }

        bool expression() {
            bool res = false;

            int prev_stack_top = matched_stack.size();

            if (comparison()) {
                res = true;
                bool flg = false;
                while (match_symbol("&&", true, false) || match_symbol("||", true, false)) {
                    flg = true;
                    if (!comparison()) {
                        throw Eval_Error("Incomplete expression", File_Position{line, column}, filename);
                    }
                }
                if (flg) {
                    build_matched(Token_Type::Expression, prev_stack_top);
                }
            }

            return res;
        }

        bool equation() {
            bool res = false;
            int prev_len = matched_stack.size();

            if (expression()) {
                res = true;
                if (match_symbol("=", true, true)
                    || match_symbol(":=", true, true)
                    || match_symbol("+=", true, true)
                    || match_symbol("-=", true, true)
                    || match_symbol("*=", true, true)
                    || match_symbol("/=", true, true)
                    ) {
                    if (!equation()) {
                        throw Eval_Error("Incomplete equation", matched_stack.back());
                    }
                    build_matched(Token_Type::Equation, prev_len);
                }
            }
            return false;
        }

        bool arg_list() {
            bool res = false;
            int prev_len = matched_stack.size();

            if (equation()) {
                ;
            }
        }

        /**
         * Reads a function definition
         */
        bool Fun() {
            std::cout << "Fun" << std::endl;
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

                if (!use_skip_ws(use_capture_(Token_Type::Id, id))()) {
                    throw Eval_Error("Missing function name in definition", File_Position(line, column), filename);
                }

                if (match_symbol("::", false)) {
                    is_method = true;

                    if (!use_skip_ws(use_capture_(Token_Type::Id, id))()) {
                        throw Eval_Error("Missing method name in definition", File_Position(line, column), filename);
                    }
                }

                // if (use_skip_ws_(match_char, '(')()) {
                //     ;
                // }
            }
            return res;
        }

        bool statements() {
            std::cout << "statements" << std::endl;
            bool res = false;

            bool has_more = true;
            bool saw_eol = true;

            while (has_more) {
                has_more = false;
                if (Fun()) {
                    if (!saw_eol) {
                        throw Eval_Error("Two function definitions missing line separator", matched_stack.back());
                    }
                    has_more = true;
                    res = true;
                    saw_eol = true;
                } else if (equation()) {
                    if (!saw_eol) {
                        throw Eval_Error("Two expressions missing line separator", matched_stack.back());
                    }
                    has_more = true;
                    res = true;
                    saw_eol = false;
                } else if (eol()) {
                    has_more = true;
                    res = true;
                    saw_eol = true;
                } else {
                    has_more = false;
                }
            }

            return res;
        }

        bool parse(string input, const char * fname) {
            pos = input.begin();
            endd = input.end();
            line = column = 1;
            filename = fname;

            if ((input.size() > 1) && (input[0] == '#') && (input[1] == '!')) {
                while ((pos != endd) && (!use_skip_ws_(eol)())) {
                    ++ pos;
                }
            }

            if (statements()) {
                if (nowable()) {
                    throw Eval_Error("Unparsed input", File_Position{line, column}, fname);
                } else {
                    build_matched(Token_Type::File, 0);
                    return true;
                }
            } else {
                return false;
            }
            return false;
        }
#undef use_skip_ws_
    };
} // namespace cyascript
