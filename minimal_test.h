#include <vector>

#ifndef MINIMAL_TEST_H_CPP2
#define MINIMAL_TEST_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"

#line 1 "/tmp/minimal_test.h2"

#line 3 "/tmp/minimal_test.h2"
class test_type;
    

//=== Cpp2 type definitions and function declarations ===========================

#line 1 "/tmp/minimal_test.h2"

#line 3 "/tmp/minimal_test.h2"
class test_type {
    private: std::vector<int>* data; 

    public: test_type(std::vector<int>& d);
#line 6 "/tmp/minimal_test.h2"
    public: auto operator=(std::vector<int>& d) -> test_type& ;

#line 10 "/tmp/minimal_test.h2"
    public: auto add_value() & -> void;
    public: test_type(test_type const&) = delete; /* No 'that' constructor, suppress copy */
    public: auto operator=(test_type const&) -> void = delete;


#line 13 "/tmp/minimal_test.h2"
};


//=== Cpp2 function definitions =================================================

#line 1 "/tmp/minimal_test.h2"

#line 6 "/tmp/minimal_test.h2"
    test_type::test_type(std::vector<int>& d)
        : data{ &d }{

#line 8 "/tmp/minimal_test.h2"
    }
#line 6 "/tmp/minimal_test.h2"
    auto test_type::operator=(std::vector<int>& d) -> test_type& {
        data = &d;
        return *this;

#line 8 "/tmp/minimal_test.h2"
    }

#line 10 "/tmp/minimal_test.h2"
    auto test_type::add_value() & -> void{
        static_cast<void>(CPP2_UFCS(emplace_back)((*cpp2::impl::assert_not_null(data)), 42));
    }
#endif

