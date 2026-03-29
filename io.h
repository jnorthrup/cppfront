#line 13 "source/io.h2"
#include "common.h"
#include <fstream>
#include <cctype>

#ifndef IO_H_CPP2
#define IO_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"

#line 1 "source/io.h2"

#line 17 "source/io.h2"
namespace cpp2 {

#line 68 "source/io.h2"
class is_preprocessor_ret;
    

#line 254 "source/io.h2"
class pre_if_depth_info;

#line 285 "source/io.h2"
class braces_tracker;

#line 393 "source/io.h2"
}


//=== Cpp2 type definitions and function declarations ===========================

#line 1 "source/io.h2"

//  Copyright 2022-2026 Herb Sutter
//  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//  
//  Part of the Cppfront Project, under the Apache License v2.0 with LLVM Exceptions.
//  See https://github.com/hsutter/cppfront/blob/main/LICENSE for license information.


//===========================================================================
//  Source loader
//===========================================================================

#line 17 "source/io.h2"
namespace cpp2 {

//-----------------------------------------------------------------------
//  move_next: advances i as long as p(line[i]) is true or the end of line
//
//  line    current line being processed
//  i       current index
//  p       predicate to apply
//
template<typename P> [[nodiscard]] auto move_next(cpp2::impl::in<std::string> line, cpp2::i32& i, P const& p) -> bool;

#line 43 "source/io.h2"
//-----------------------------------------------------------------------
//  peek_first_non_whitespace: returns the first non-whitespace char in line
//
//  line    current line being processed
//
[[nodiscard]] auto peek_first_non_whitespace(cpp2::impl::in<std::string> line) -> char;

#line 61 "source/io.h2"
//-----------------------------------------------------------------------
//  is_preprocessor: returns whether this is a preprocessor line starting
//  with #, and whether it will be followed by another preprocessor line
//
//  line        current line being processed
//  first_line  whether this is supposed to be the first line (start with #)
//
class is_preprocessor_ret {
    public: bool is_preprocessor {false}; 
    public: bool has_continuation {false}; 
    public: is_preprocessor_ret(auto&& is_preprocessor_, auto&& has_continuation_)
CPP2_REQUIRES_ (std::is_convertible_v<CPP2_TYPEOF(is_preprocessor_), std::add_const_t<bool>&> && std::is_convertible_v<CPP2_TYPEOF(has_continuation_), std::add_const_t<bool>&>) ;
public: is_preprocessor_ret();

#line 71 "source/io.h2"
};

[[nodiscard]] auto is_preprocessor(
    cpp2::impl::in<std::string> line, 
    cpp2::impl::in<bool> first_line
) -> is_preprocessor_ret;

#line 92 "source/io.h2"
//-----------------------------------------------------------------------
//  starts_with_import: returns whether the line starts with "import"
//
//  line    current line being processed
//
[[nodiscard]] auto starts_with_import(cpp2::impl::in<std::string> line) -> bool;

#line 118 "source/io.h2"
//-----------------------------------------------------------------------
//  starts_with_whitespace_slash_slash: is this a "// comment" line
//
//  line    current line being processed
//
[[nodiscard]] auto starts_with_whitespace_slash_slash(cpp2::impl::in<std::string> line) -> bool;

#line 140 "source/io.h2"
//-----------------------------------------------------------------------
//  starts_with_whitespace_slash_star_and_no_star_slash: is this a "/* comment" line
//
//  line    current line being processed
//
[[nodiscard]] auto starts_with_whitespace_slash_star_and_no_star_slash(cpp2::impl::in<std::string> line) -> bool;

#line 168 "source/io.h2"
//-----------------------------------------------------------------------
//  starts_with_operator: returns whether the line starts with the string "operator"
//  followed by the symbols of an operator
//
//  line    current line being processed
//
[[nodiscard]] auto starts_with_operator(cpp2::impl::in<std::string_view> s) -> cpp2::i32;

#line 251 "source/io.h2"
//-----------------------------------------------------------------------
//  braces_tracker: to track brace depth
//
class pre_if_depth_info {
    private: cpp2::i32 if_net_braces {0}; 
    private: bool found_else {false}; 
    private: cpp2::i32 else_net_braces {0}; 

    public: auto found_open_brace() & -> void;

#line 264 "source/io.h2"
    public: auto found_close_brace() & -> void;

#line 269 "source/io.h2"
    public: [[nodiscard]] auto found_preprocessor_else_was_there_another() & -> bool;

#line 277 "source/io.h2"
    public: [[nodiscard]] auto braces_to_ignore() const& -> cpp2::i32;
    public: pre_if_depth_info() = default;
    public: pre_if_depth_info(pre_if_depth_info const&) = delete; /* No 'that' constructor, suppress copy */
    public: auto operator=(pre_if_depth_info const&) -> void = delete;


#line 283 "source/io.h2"
};

class braces_tracker {
    private: std::vector<pre_if_depth_info> preprocessor {}; 
    private: char current_open_type {' '}; 
    private: std::vector<lineno_t> open_braces {}; 
    private: std::vector<error_entry>* error_list; 

    public: braces_tracker(std::vector<error_entry>& errors);
#line 291 "source/io.h2"
    public: auto operator=(std::vector<error_entry>& errors) -> braces_tracker& ;

#line 295 "source/io.h2"
    public: auto found_open_brace(cpp2::impl::in<lineno_t> lineno, cpp2::impl::in<char> brace) & -> void;

#line 306 "source/io.h2"
    public: auto found_close_brace(cpp2::impl::in<source_position> pos, cpp2::impl::in<char> brace) & -> void;

#line 326 "source/io.h2"
    public: auto found_eof(cpp2::impl::in<source_position> pos) & -> void;

#line 349 "source/io.h2"
    public: [[nodiscard]] auto current_depth() const& -> cpp2::i32;

#line 353 "source/io.h2"
    public: auto found_pre_if([[maybe_unused]] cpp2::impl::in<lineno_t> unnamed_param_2) & -> void;

#line 357 "source/io.h2"
    public: auto found_pre_else(cpp2::impl::in<lineno_t> lineno) & -> void;

#line 373 "source/io.h2"
    public: auto found_pre_endif(cpp2::impl::in<lineno_t> lineno) & -> void;
    public: braces_tracker(braces_tracker const&) = delete; /* No 'that' constructor, suppress copy */
    public: auto operator=(braces_tracker const&) -> void = delete;


#line 391 "source/io.h2"
};

}  // namespace cpp2


//=== Cpp2 function definitions =================================================

#line 1 "source/io.h2"

#line 17 "source/io.h2"
namespace cpp2 {

#line 26 "source/io.h2"
template<typename P> [[nodiscard]] auto move_next(cpp2::impl::in<std::string> line, cpp2::i32& i, P const& p) -> bool
{
    while( (
        cpp2::impl::cmp_less(i,std::ssize(line)) 
        && CPP2_ASSERT_IN_BOUNDS(line, i) 
        && p(CPP2_ASSERT_IN_BOUNDS(line, i))) ) 

    {
        ++i;
    }
    return 
        cpp2::impl::cmp_less(i,std::ssize(line)) 
        && CPP2_ASSERT_IN_BOUNDS(line, i); 

}

#line 48 "source/io.h2"
[[nodiscard]] auto peek_first_non_whitespace(cpp2::impl::in<std::string> line) -> char
{
    cpp2::i32 i {0}; 

    //  find first non-whitespace character
    if (!(move_next(line, i, [](cpp2::impl::in<char> c) -> bool{return std::isspace(c) != 0; }))) {
        return '\0'; 
    }

    return CPP2_ASSERT_IN_BOUNDS(line, cpp2::move(i)); 
}

is_preprocessor_ret::is_preprocessor_ret(auto&& is_preprocessor_, auto&& has_continuation_)
requires (std::is_convertible_v<CPP2_TYPEOF(is_preprocessor_), std::add_const_t<bool>&> && std::is_convertible_v<CPP2_TYPEOF(has_continuation_), std::add_const_t<bool>&>) 
                                                                                                            : is_preprocessor{ CPP2_FORWARD(is_preprocessor_) }
                                                                                                            , has_continuation{ CPP2_FORWARD(has_continuation_) }{}
is_preprocessor_ret::is_preprocessor_ret(){}

#line 73 "source/io.h2"
[[nodiscard]] auto is_preprocessor(
    cpp2::impl::in<std::string> line, 
    cpp2::impl::in<bool> first_line
) -> is_preprocessor_ret
{
    //  see if the first non-whitespace is #
    if ((
        first_line 
        && peek_first_non_whitespace(line) != '#')) 

    {
        return { false, false }; 
    }

    //  return true iff last character is a \ continuation
    return { true, CPP2_UFCS(back)(line) == '\\' }; 
}

#line 97 "source/io.h2"
[[nodiscard]] auto starts_with_import(cpp2::impl::in<std::string> line) -> bool
{
    cpp2::i32 i {0}; 

    //  find first non-whitespace character
    if (!(move_next(line, i, [](cpp2::impl::in<char> c) -> bool{return std::isspace(c) != 0; }))) {
        return false; 
    }

    std::string_view import_keyword {"import"}; 

    // the first token must begin with 'import'
    if (!(CPP2_UFCS(starts_with)(CPP2_UFCS(substr)(std::string_view(line), i), import_keyword))) {
        return false; 
    }

    // and not be immediately followed by an _identifier-continue_
    return !(is_identifier_continue(CPP2_ASSERT_IN_BOUNDS(line, cpp2::move(i) + cpp2::unchecked_narrow<cpp2::i32>(CPP2_UFCS(size)(cpp2::move(import_keyword)))))); 
}

#line 123 "source/io.h2"
[[nodiscard]] auto starts_with_whitespace_slash_slash(cpp2::impl::in<std::string> line) -> bool
{
    cpp2::i32 i {0}; 

    //  find first non-whitespace character
    if (!(move_next(line, i, [](cpp2::impl::in<char> c) -> bool{return std::isspace(c) != 0; }))) {
        return false; 
    }

    return 
        cpp2::impl::cmp_less(i,std::ssize(line) - 1) 
        && CPP2_ASSERT_IN_BOUNDS(line, i) == '/' 
        && CPP2_ASSERT_IN_BOUNDS(line, i + 1) == '/'; 

}

#line 145 "source/io.h2"
[[nodiscard]] auto starts_with_whitespace_slash_star_and_no_star_slash(cpp2::impl::in<std::string> line) -> bool
{
    cpp2::i32 i {0}; 

    //  find first non-whitespace character
    if (!(move_next(line, i, [](cpp2::impl::in<char> c) -> bool{return std::isspace(c) != 0; }))) {
        return false; 
    }

    if ((
        cpp2::impl::cmp_less(i,std::ssize(line) - 1) 
        && CPP2_ASSERT_IN_BOUNDS(line, i) == '/' 
        && CPP2_ASSERT_IN_BOUNDS(line, i + 1) == '*')) 

    {
        return CPP2_UFCS(find)(line, "*/", cpp2::move(i)) == line.npos; 
    }
    else {
        return false; 
    }
}

#line 174 "source/io.h2"
[[nodiscard]] auto starts_with_operator(cpp2::impl::in<std::string_view> s) -> cpp2::i32
{
    if (CPP2_UFCS(starts_with)(s, "operator")) 
    {
        cpp2::i32 j {8}; 

        //  skip any spaces
        while( (
            cpp2::impl::cmp_less(j,std::ssize(s)) 
            && std::isspace(CPP2_ASSERT_IN_BOUNDS(s, j)) != 0) ) 

        {
            ++j;
        }
        if (cpp2::impl::cmp_greater_eq(j,std::ssize(s))) {
            return 0; 
        }

        char c1 {'\0'}; 
        char c2 {'\0'}; 
        char c3 {'\0'}; 
        if (cpp2::impl::cmp_less(j,std::ssize(s))) {c1 = CPP2_ASSERT_IN_BOUNDS(s, j);}
        if (cpp2::impl::cmp_less(j + 1,std::ssize(s))) {c2 = CPP2_ASSERT_IN_BOUNDS(s, j + 1); }
        if (cpp2::impl::cmp_less(j + 2,std::ssize(s))) {c3 = CPP2_ASSERT_IN_BOUNDS(s, j + 2); }

        //  Check operator symbols using if/else chain
        //  /= /  == =  ! !=  *= *  %= %  ^= ^  ~= ~
        if (c1 == '/' || c1 == '=' || c1 == '!' || c1 == '*' || c1 == '%' || c1 == '^' || c1 == '~') 
        {
            if (c2 == '=') {return j + 2; }
            return j + 1; 
        }

        //  ++ += +
        if (c1 == '+') 
        {
            if (c2 == '=' || c2 == '+') {return j + 2; }
            return j + 1; 
        }

        //  -- -= -> -
        if (c1 == '-') 
        {
            if (c2 == '=' || c2 == '-' || c2 == '>') {return j + 2; }
            return j + 1; 
        }

        //  ||= || |= |   &&= && &= &
        if (c1 == '|' || c1 == '&') 
        {
            if (c2 == c1 && c3 == '=') {return j + 3; }
            if (c2 == c1 || c2 == '=') {return j + 2; }
            return j + 1; 
        }

        //  >>= >> >= >
        if (c1 == '>') 
        {
            if (c2 == '>' && c3 == '=') {return j + 3; }
            if (c2 == '>' || c2 == '=') {return j + 2; }
            return j + 1; 
        }

        //  <<= << <=> <= <
        if (cpp2::move(c1) == '<') 
        {
            if (c2 == '<' && c3 == '=') {return j + 3; }
            if (c2 == '=' && cpp2::move(c3) == '>') {return j + 3; }
            if (c2 == '<' || c2 == '=') {return j + 2; }
            return cpp2::move(j) + 1; 
        }
    }

    return 0; 
}

#line 259 "source/io.h2"
    auto pre_if_depth_info::found_open_brace() & -> void{
        if (!(found_else)) {if_net_braces = if_net_braces + 1;}
        else           { else_net_braces = else_net_braces + 1; }
    }

#line 264 "source/io.h2"
    auto pre_if_depth_info::found_close_brace() & -> void{
        if (!(found_else)) {if_net_braces = if_net_braces - 1;}
        else           { else_net_braces = else_net_braces - 1; }
    }

#line 269 "source/io.h2"
    [[nodiscard]] auto pre_if_depth_info::found_preprocessor_else_was_there_another() & -> bool{
        if (found_else) {
            return true; 
        }
        found_else = true;
        return false; 
    }

#line 277 "source/io.h2"
    [[nodiscard]] auto pre_if_depth_info::braces_to_ignore() const& -> cpp2::i32{
        if (cpp2::impl::cmp_greater_eq(if_net_braces,0) && if_net_braces == else_net_braces) {
            return if_net_braces; 
        }
        return 0; 
    }

#line 291 "source/io.h2"
    braces_tracker::braces_tracker(std::vector<error_entry>& errors)
        : error_list{ &errors }{

#line 293 "source/io.h2"
    }
#line 291 "source/io.h2"
    auto braces_tracker::operator=(std::vector<error_entry>& errors) -> braces_tracker& {
        preprocessor = {};
        current_open_type = ' ';
        open_braces = {};
        error_list = &errors;
        return *this;

#line 293 "source/io.h2"
    }

#line 295 "source/io.h2"
    auto braces_tracker::found_open_brace(cpp2::impl::in<lineno_t> lineno, cpp2::impl::in<char> brace) & -> void{
        cpp2::impl::assert(cpp2::impl::cmp_greater(std::ssize(preprocessor),0));
        if (CPP2_UFCS(empty)(open_braces)) {
            current_open_type = brace;
        }
        if (current_open_type == brace) {
            CPP2_UFCS(push_back)(open_braces, lineno);
            CPP2_UFCS(found_open_brace)((*cpp2::impl::assert_not_null(CPP2_UFCS(back)(preprocessor))));
        }
    }

#line 306 "source/io.h2"
    auto braces_tracker::found_close_brace(cpp2::impl::in<source_position> pos, cpp2::impl::in<char> brace) & -> void{
        cpp2::impl::assert(cpp2::impl::cmp_greater(std::ssize(preprocessor),0));

        if ((current_open_type == '{' && brace == '}') 
            || (current_open_type == '(' && brace == ')')) 
        {
            if (cpp2::impl::cmp_less(std::ssize(open_braces),1)) {
                static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(error_list)), 
                    pos, 
                    "closing } does not match a prior {"
                ));
            }
            else {
                CPP2_UFCS(pop_back)(open_braces);
            }

            CPP2_UFCS(found_close_brace)((*cpp2::impl::assert_not_null(CPP2_UFCS(back)(preprocessor))));
        }
    }

#line 326 "source/io.h2"
    auto braces_tracker::found_eof(cpp2::impl::in<source_position> pos) & -> void{
        if (current_depth() != 0) {
            std::string unmatched {""}; 
            cpp2::i32 i {0}; 
            while( cpp2::impl::cmp_less(i,std::ssize(open_braces)) ) {
                if (cpp2::impl::cmp_greater(i,0) && cpp2::impl::cmp_greater(std::size(open_braces),2)) {unmatched = unmatched + ",";}
                if (cpp2::impl::cmp_greater(i,0) && i == std::ssize(open_braces) - 1) {unmatched = unmatched + " and";}
                unmatched = unmatched + " " + std::to_string(CPP2_ASSERT_IN_BOUNDS(open_braces, i));
                i = i + 1;
            }
            std::string suffix {""}; 
            if (cpp2::impl::cmp_greater(current_depth(),1)) {suffix = "s"; }
            static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(error_list)), 
                pos, 
                std::string("end of file reached with ") 
                + std::to_string(current_depth()) 
                + " missing } to match earlier { on line" 
                + cpp2::move(suffix) 
                + cpp2::move(unmatched)
            ));
        }
    }

#line 349 "source/io.h2"
    [[nodiscard]] auto braces_tracker::current_depth() const& -> cpp2::i32{
        return cpp2::unchecked_narrow<cpp2::i32>(std::ssize(open_braces)); 
    }

#line 353 "source/io.h2"
    auto braces_tracker::found_pre_if([[maybe_unused]] cpp2::impl::in<lineno_t> unnamed_param_2) & -> void{
        CPP2_UFCS(push_back)(preprocessor, {});
    }

#line 357 "source/io.h2"
    auto braces_tracker::found_pre_else(cpp2::impl::in<lineno_t> lineno) & -> void{
        if (cpp2::impl::cmp_less(std::ssize(preprocessor),2)) {
            static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(error_list)), 
                lineno, 
                "#else does not match a prior #if"
            ));
        }

        if (CPP2_UFCS(found_preprocessor_else_was_there_another)((*cpp2::impl::assert_not_null(CPP2_UFCS(back)(preprocessor))))) {
            static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(error_list)), 
                lineno, 
                "#else already encountered for this #if"
            ));
        }
    }

#line 373 "source/io.h2"
    auto braces_tracker::found_pre_endif(cpp2::impl::in<lineno_t> lineno) & -> void{
        if (cpp2::impl::cmp_less(std::ssize(preprocessor),2)) {
            static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(error_list)), 
                lineno, 
                "#endif does not match a prior #if"
            ));
        }

        cpp2::i32 i {0}; 
        while( cpp2::impl::cmp_less(i,CPP2_UFCS(braces_to_ignore)((*cpp2::impl::assert_not_null(CPP2_UFCS(back)(preprocessor))))) ) {
            char close_brace {')'}; 
            if (current_open_type == '{') {close_brace = '}'; }
            found_close_brace(source_position(), cpp2::move(close_brace));
            i = i + 1;
        }

        CPP2_UFCS(pop_back)(preprocessor);
    }

#line 393 "source/io.h2"
}

#endif
