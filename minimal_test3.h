#include <vector>
#include <string>

#ifndef MINIMAL_TEST3_H_CPP2
#define MINIMAL_TEST3_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"

#line 1 "/tmp/minimal_test3.h2"

#line 4 "/tmp/minimal_test3.h2"
class test_type;
    

//=== Cpp2 type definitions and function declarations ===========================

#line 1 "/tmp/minimal_test3.h2"

#line 4 "/tmp/minimal_test3.h2"
class test_type {
    private: std::vector<std::string>* data; 

    public: test_type(std::vector<std::string>& d);
#line 7 "/tmp/minimal_test3.h2"
    public: auto operator=(std::vector<std::string>& d) -> test_type& ;

#line 11 "/tmp/minimal_test3.h2"
    public: auto add_value(cpp2::impl::in<cpp2::i32> pos) & -> void;
    public: test_type(test_type const&) = delete; /* No 'that' constructor, suppress copy */
    public: auto operator=(test_type const&) -> void = delete;


#line 18 "/tmp/minimal_test3.h2"
};


//=== Cpp2 function definitions =================================================

#line 1 "/tmp/minimal_test3.h2"

#line 7 "/tmp/minimal_test3.h2"
    test_type::test_type(std::vector<std::string>& d)
        : data{ &d }{

#line 9 "/tmp/minimal_test3.h2"
    }
#line 7 "/tmp/minimal_test3.h2"
    auto test_type::operator=(std::vector<std::string>& d) -> test_type& {
        data = &d;
        return *this;

#line 9 "/tmp/minimal_test3.h2"
    }

#line 11 "/tmp/minimal_test3.h2"
    auto test_type::add_value(cpp2::impl::in<cpp2::i32> pos) & -> void{
        if (pos != 0) {
            static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(data)), 
                std::string("hello ") + std::to_string(pos)
            ));
        }
    }
#endif

