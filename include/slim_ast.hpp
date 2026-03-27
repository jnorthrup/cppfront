// slim_ast.hpp — Stub for combinator stack compatibility
#ifndef CPP2_SLIM_AST_HPP
#define CPP2_SLIM_AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace cpp2::ast {

enum class NodeKind {
    Expression, Statement, Declaration,
    Identifier, Literal, FunctionDecl, TypeDecl,
    BinaryOp, UnaryOp, Call, Return
};

struct Node {
    virtual ~Node() = default;
    NodeKind kind = NodeKind::Expression;
};

struct Expression : Node {};
struct Statement : Node {};
struct Declaration : Node {};

struct Identifier : Expression {
    std::string name;
};

struct Literal : Expression {
    std::string value;
};

struct FunctionDecl : Declaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration>> params;
    std::unique_ptr<Expression> return_type;
    std::unique_ptr<Statement> body;
};

struct TypeDecl : Declaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration>> members;
};

}
#endif
