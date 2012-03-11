#include "parser.h"
#include "ast.h"
#include <assert.h> // assert
#include <stdlib.h> // NULL

namespace candor {
namespace internal {

AstNode* Parser::Execute() {
  AstNode* stmt;
  while ((stmt = ParseStatement()) != NULL) {
    ast()->children()->Push(stmt);
  }

  // If parsing was successful - reset any errors
  if (Peek()->is(kEnd)) {
    SetError(NULL);
  }

  return ast();
}


AstNode* Parser::ParseStatement() {
  Position pos(this);
  AstNode* result = NULL;

  // Skip cr's before statement
  // needed for {\n blocks \n}
  while (Peek()->is(kCr)) Skip();

  switch (Peek()->type()) {
   case kReturn:
    Skip();
    {
      result = new AstNode(AstNode::kReturn);
      AstNode* value = ParseExpression();
      if (value == NULL) {
        value = new AstNode(AstNode::kNil);
        value->value("nil");
        value->length(3);
      }
      result->children()->Push(value);
    }
    break;
   case kBreak:
    Skip();
    result = new AstNode(AstNode::kBreak);
    break;
   case kIf:
    Skip();
    {
      if (!Peek()->is(kParenOpen)) {
        SetError("Expected '(' before if's condition");
        return NULL;
      }
      Skip();

      AstNode* cond = ParseExpression();
      if (cond == NULL) {
        SetError("Expected if's condition");
        return NULL;
      }

      if (!Peek()->is(kParenClose)) {
        SetError("Expected ')' after if's condition");
        return NULL;
      }
      Skip();

      AstNode* body = ParseBlock(NULL);
      AstNode* elseBody = NULL;

      if (body == NULL) {
        body = ParseStatement();
      } else {
        if (Peek()->is(kElse)) {
          Skip();
          elseBody = ParseBlock(NULL);
        }
      }

      if (body == NULL) {
        SetError("Expected if's body");
        return NULL;
      }

      result = new AstNode(AstNode::kIf);
      result->children()->Push(cond);
      result->children()->Push(body);
      if (elseBody != NULL) result->children()->Push(elseBody);
    }
    break;
   case kWhile:
    Skip();
    {
      if (!Peek()->is(kParenOpen)) {
        SetError("Expected '(' before while's condition");
        return NULL;
      }
      Skip();

      AstNode* cond = ParseExpression();
      if (cond == NULL) {
        SetError("Expected while's condition");
        return NULL;
      }
      if (!Peek()->is(kParenClose)) {
        SetError("Expected ')' after while's condition");
        return NULL;
      }
      Skip();

      AstNode* body = ParseBlock(NULL);
      if (body == NULL) {
        return NULL;
      }

      result = new AstNode(AstNode::kWhile);
      result->children()->Push(cond);
      result->children()->Push(body);
    }
    break;
   case kBraceOpen:
    result = ParseBlock(NULL);
    break;
   default:
    result = ParseExpression();
    break;
  }

  // Consume kCr or kBraceClose
  if (!Peek()->is(kEnd) && !Peek()->is(kCr) && !Peek()->is(kBraceClose)) {
    SetError("Expected CR, EOF, or '}' after statement");
    return NULL;
  }
  if (Peek()->is(kCr)) Skip();

  return pos.Commit(result);
}

#define BINOP_PRI1\
    case kLOr:\
    case kLAnd:

#define BINOP_PRI2\
    case kEq:\
    case kNe:\
    case kStrictEq:\
    case kStrictNe:

#define BINOP_PRI3\
    case kLt:\
    case kGt:\
    case kLe:\
    case kGe:

#define BINOP_PRI4\
    case kBOr:\
    case kBAnd:\
    case kBXor:

#define BINOP_PRI5\
    case kAdd:\
    case kSub:

#define BINOP_PRI6\
    case kMul:\
    case kDiv:

#define BINOP_SWITCH(type, result, priority, K)\
    type = Peek()->type();\
    switch (type) {\
     K\
      result = ParseBinOp(type, result, priority);\
     default:\
      break;\
    }\
    if (result == NULL) {\
      SetError("Failed to parse binary operation");\
      return result;\
    }


AstNode* Parser::ParseExpression(int priority) {
  AstNode* result = NULL;
  AstNode* member = NULL;

  Position pos(this);

  // Parse prefix unops and block expression
  switch (Peek()->type()) {
   case kInc:
   case kDec:
   case kNot:
   case kAdd:
   case kSub:
    member = ParsePrefixUnOp(Peek()->type());
    break;
   case kBraceOpen:
    member = ParseObjectLiteral();
    break;
   case kArrayOpen:
    member = ParseArrayLiteral();
    break;
   case kTypeof:
   case kSizeof:
   case kKeysof:
    {
      Position pos(this);

      TokenType type = Peek()->type();
      Skip();

      AstNode* expr = ParseExpression(7);
      if (expr == NULL) {
        SetError("Expected body of prefix operation");
        return NULL;
      }

      member = new AstNode(AstNode::ConvertType(type));
      member->children()->Push(expr);

      pos.Commit(member);
      break;
    }
   default:
    member = ParseMember();
    break;
  }

  switch (Peek()->type()) {
   case kAssign:
    if (member == NULL) {
      SetError("Expected lhs before '='");
      return NULL;
    }

    // member "=" expr
    {
      Skip();
      AstNode* value = ParseExpression();
      if (value == NULL) {
        SetError("Expected rhs after '='");
        return NULL;
      }
      result = new AstNode(AstNode::kAssign);
      result->children()->Push(member);
      result->children()->Push(value);
    }
    break;
   default:
    result = member;
    break;
  }

  if (result == NULL) {
    return result;
  }

  // Parse postfixes
  TokenType type = Peek()->type();
  switch (type) {
   case kInc:
    Skip();
    result = new UnOp(UnOp::kPostInc, result);
    break;
   case kDec:
    Skip();
    result = new UnOp(UnOp::kPostDec, result);
    break;
   default:
    break;
  }

  // Parse binops ordered by priority
  AstNode* initial;
  do {
    initial = result;
    switch (priority) {
     default:
     case 1:
      BINOP_SWITCH(type, result, 1, BINOP_PRI1)
     case 2:
      BINOP_SWITCH(type, result, 2, BINOP_PRI2)
     case 3:
      BINOP_SWITCH(type, result, 3, BINOP_PRI3)
     case 4:
      BINOP_SWITCH(type, result, 4, BINOP_PRI4)
     case 5:
      BINOP_SWITCH(type, result, 5, BINOP_PRI5)
     case 6:
      BINOP_SWITCH(type, result, 6, BINOP_PRI6)
     case 7:
      break;
      // Do not parse binary operations
    }
  } while (initial != result);

  return pos.Commit(result);
}


AstNode* Parser::ParsePrefixUnOp(TokenType type) {
  Position pos(this);

  // Consume prefix token
  Skip();

  AstNode* expr;
  {
    NegateSign n(this, type);

    expr = ParseExpression(7);
  }

  if (expr == NULL) {
    SetError("Expected expression after unary operation");
    return NULL;
  }

  return pos.Commit(new UnOp(UnOp::ConvertPrefixType(NegateType(type)), expr));
}


AstNode* Parser::ParseBinOp(TokenType type, AstNode* lhs, int priority) {
  Position pos(this);

  // Consume binop token
  Skip();

  AstNode* rhs;
  {
    NegateSign n(this, type);

    rhs = ParseExpression(priority);
  }

  if (rhs == NULL) {
    SetError("Expected rhs for binary operation");
    return NULL;
  }

  AstNode* result = new BinOp(BinOp::ConvertType(NegateType(type)), lhs, rhs);

  return pos.Commit(result);
}


AstNode* Parser::ParsePrimary() {
  Position pos(this);
  Lexer::Token* token = Peek();
  AstNode* result = NULL;

  switch (token->type()) {
   case kName:
   case kNumber:
   case kString:
   case kTrue:
   case kFalse:
   case kNil:
    result = new AstNode(AstNode::ConvertType(token->type()));
    result->FromToken(token);
    Skip();
    break;
   case kParenOpen:
    Skip();
    result = ParseExpression();
    if (!Peek()->is(kParenClose)) {
      SetError("Expected closing paren for primary expression");
      return NULL;
    } else {
      Skip();
    }

    // Check if we haven't parsed function's declaration by occasion
    if (Peek()->is(kBraceOpen)) {
      SetError("Unexpected '{' after expression in parens");
      return NULL;
    }
    break;
   default:
    result = NULL;
    break;
  }

  return pos.Commit(result);
}


AstNode* Parser::ParseMember() {
  Position pos(this);
  AstNode* result = ParsePrimary();

  while (!Peek()->is(kEnd) && !Peek()->is(kCr)) {
    if (Peek()->is(kParenOpen)) {
      // Calls and function declarations
      FunctionLiteral* fn = new FunctionLiteral(result, Peek()->offset());
      result = NULL;
      Skip();

      while (!Peek()->is(kParenClose) && !Peek()->is(kEnd)) {
        AstNode* expr = ParseExpression();
        if (expr == NULL) break;
        fn->args()->Push(expr);

        // Skip commas
        if (Peek()->is(kComma)) Skip();
      }
      if (!Peek()->is(kParenClose)) {
        SetError("Failed to parse function's arguments");
        break;
      }
      Skip();

      // Optional body (for function declaration)
      ParseBlock(reinterpret_cast<AstNode*>(fn));
      if (!fn->CheckDeclaration()) {
        SetError("Incorrect function declaration or call");
        break;
      }

      result = fn->End(Peek()->offset());
    } else {
      if (result == NULL) {
        SetError("Unexpected '.' or '['");
        break;
      }

      AstNode* next = NULL;
      if (Peek()->is(kDot)) {
        // a.b
        Skip();
        next = ParsePrimary();
        if (next != NULL && next->is(AstNode::kName)) {
          next->type(AstNode::kProperty);
        }
      } else if (Peek()->is(kArrayOpen)) {
        // a["prop-expr"]
        Skip();
        next = ParseExpression();
        if (Peek()->is(kArrayClose)) {
          Skip();
        } else {
          next = NULL;
        }
      }

      if (next == NULL) break;

      result = Wrap(AstNode::kMember, result);
      result->children()->Push(next);
    }
  }

  return pos.Commit(result);
}


AstNode* Parser::ParseObjectLiteral() {
  Position pos(this);

  // Skip '{'
  if (!Peek()->is(kBraceOpen)) {
    SetError("Expected '{'");
    return NULL;
  }
  Skip();

  ObjectLiteral* result = new ObjectLiteral();

  while (!Peek()->is(kBraceClose) && !Peek()->is(kEnd)) {
    AstNode* key;
    if (Peek()->is(kString) || Peek()->is(kName)) {
      key = (new AstNode(AstNode::kProperty))->FromToken(Peek());
      Skip();
    } else if (Peek()->is(kNumber)) {
      key = (new AstNode(AstNode::kProperty))->FromToken(Peek());
      Skip();
    } else {
      SetError("Expected string or number as object literal's key");
      return NULL;
    }

    // Skip ':'
    if (!Peek()->is(kColon)) {
      SetError("Expected colon after object literal's key");
      return NULL;
    }
    Skip();

    // Parse expression
    AstNode* value = ParseExpression();
    if (value == NULL) {
      SetError("Expected expression after colon");
      return NULL;
    }

    result->keys()->Push(key);
    result->values()->Push(value);

    // Skip ',' or exit loop on '}'
    if (Peek()->is(kComma)) {
      Skip();
    } else if (!Peek()->is(kBraceClose)) {
      SetError("Expected '}' or ','");
      return NULL;
    }
  }

  // Skip '}'
  if (!Peek()->is(kBraceClose)) {
    SetError("Expected '}'");
    return NULL;
  }
  Skip();

  return pos.Commit(result);
}


AstNode* Parser::ParseArrayLiteral() {
  Position pos(this);

  // Skip '['
  if (!Peek()->is(kArrayOpen)) {
    SetError("Expected '['");
    return NULL;
  }
  Skip();

  AstNode* result = new AstNode(AstNode::kArrayLiteral);

  while (!Peek()->is(kArrayClose) && !Peek()->is(kEnd)) {
    // Parse expression
    AstNode* value = ParseExpression();
    if (value == NULL) {
      SetError("Expected expression after array literal's start");
      return NULL;
    }

    result->children()->Push(value);

    // Skip ',' or exit loop on ']'
    if (Peek()->is(kComma)) {
      Skip();
    } else if (!Peek()->is(kArrayClose)) {
      SetError("Expected ']' or ','");
      return NULL;
    }
  }

  // Skip ']'
  if (!Peek()->is(kArrayClose)) {
    SetError("Expected ']'");
    return NULL;
  }
  Skip();

  return pos.Commit(result);
}


AstNode* Parser::ParseBlock(AstNode* block) {
  if (!Peek()->is(kBraceOpen)) {
    SetError("Expected '{'");
    return NULL;
  }

  bool fn = block != NULL;

  Position pos(this);

  Skip();

  AstNode* result = fn ? block : new AstNode(AstNode::kBlock);

  while (!Peek()->is(kEnd) && !Peek()->is(kBraceClose)) {
    AstNode* stmt = ParseStatement();
    if (stmt == NULL) {
      SetError("Expected statement after '{'");
      break;
    }
    result->children()->Push(stmt);
  }
  if (!Peek()->is(kBraceClose)) return NULL;
  Skip();

  // Block should not be empty
  if (result->children()->length() == 0) {
    result->children()->Push(new AstNode(AstNode::kNop));
  }

  return pos.Commit(result);
}


void Parser::Print(char* buffer, uint32_t size) {
  PrintBuffer p(buffer, size);
  ast()->PrintChildren(&p, ast()->children());
  p.Finalize();
}

} // namespace internal
} // namespace candor
