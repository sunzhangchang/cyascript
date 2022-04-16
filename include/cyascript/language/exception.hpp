#pragma once

#include <stdexcept>
#include <fmt/format.h>
#include <folly/FBString.h>
#include "language/topper.hpp"

namespace cyascript {
    /**
     * Errors generated during parsing or evaluation
     */
    struct Eval_Error: public std::runtime_error {
        string reason;
        File_Position start_position;
        File_Position end_position;
        const char *filename;

        Eval_Error(const string &why, const File_Position &where, const char *fname)
            : std::runtime_error{fmt::format("[Error] {} \" {} at ({}, {})", why, ((string{fname} != "__EVAL__") ? (fmt::format("in '{}'", fname)) : ("during evaluation")), where.line, where.column)},
            reason(why), start_position(where), end_position(where), filename(fname) {
        }

        Eval_Error(const string &why, const TokenPtr &where)
            : Eval_Error(why, where->start, where->filename) {
        }

        virtual ~Eval_Error() throw() {}
    };
} // namespace cyascript

