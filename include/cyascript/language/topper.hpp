#pragma once

#include <memory>
#include <utility>
#include <folly/Function.h>
#include <folly/FBString.h>
#include <folly/FBVector.h>
#include "language/topper.hpp"

namespace cyascript {
    using string = folly::fbstring;

    /**
     * Convenience type for file positions
     */
    struct File_Position {
        int line = 0, column = 0;
        File_Position() = default;
        File_Position(int file_line, int file_col)
            : line{file_line}, column{file_col} {
        }
    };

    enum Token_Type {
        Error, Int, Float, Id, Char, Str, Eol, Fun_Call, Inplace_Fun_Call, Arg_List, Variable, Equation, Var_Decl,
        Expression, Comparison, Additive, Multiplicative, Negate, Not, Array_Call, Dot_Access, Quoted_String, Single_Quoted_String,
        Lambda, Block, Def, While, If, For, Inline_Array, Inline_Map, Return, File, Prefix, Break, Map_Pair, Value_Range,
        Inline_Range, Annotation, Try, Catch, Finally, Method, Attr_Decl
    };

    using TokenPtr = std::shared_ptr<struct Token>;

    /**
     * The struct that doubles as both a parser token and an AST node
     */
    struct Token {
        string text;
        Token_Type identifier;
        const char *filename;
        File_Position start, end;
        bool is_cached;
        // Boxed_Value cached_value;

        folly::fbvector<TokenPtr> children;
        TokenPtr annotation;

        Token(const string &token_text, Token_Type id, const char *fname)
            : text{token_text}, identifier{id}, filename{fname}, is_cached{false} {
        }

        Token(const string &token_text, Token_Type id, const char *fname, int start_line, int start_col, int end_line, int end_col)
            : Token(token_text, id, fname) {
            start = {start_line, start_col};
            end = {end_line, end_col};
        }
    };

    namespace {
        /**
         * Helper lookup to get the name of each node type
         */
        const char * token_type_to_string(Token_Type tokentype) {
            static const char *token_types[] = {"Internal Parser Error", "Int", "Float", "Id", "Char", "Str", "Eol", "Fun_Call", "Inplace_Fun_Call", "Arg_List", "Variable", "Equation", "Var_Decl",
                "Expression", "Comparison", "Additive", "Multiplicative", "Negate", "Not", "Array_Call", "Dot_Access", "Quoted_String", "Single_Quoted_String",
                "Lambda", "Block", "Def", "While", "If", "For", "Inline_Array", "Inline_Map", "Return", "File", "Prefix", "Break", "Map_Pair", "Value_Range",
                "Inline_Range", "Annotation", "Try", "Catch", "Finally", "Method", "Attr_Decl"};

            return token_types[tokentype];
        }
    }

} // namespace cyascript

#include "exception.hpp"
