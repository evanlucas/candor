#ifndef _SRC_PARSER_H_
#define _SRC_PARSER_H_

#include "lexer.h"
#include "ast.h"
#include "utils.h"

#include <assert.h> // assert
#include <stdlib.h> // NULL

namespace candor {
namespace internal {

// Creates AST tree from plain source code
class Parser : public Lexer, public ErrorHandler {
 public:
  enum ParserSign {
    kNormal,
    kNegated
  };

  enum ParseStatementType {
    kSkipTrailingCr,
    kLeaveTrailingCr
  };

  enum PrimaryRestriction {
    kNoKeywords,
    kAny
  };

  Parser(const char* source, uint32_t length) : Lexer(source, length) {
    ast_ = new FunctionLiteral(NULL);
    ast_->make_root();
    sign_ = kNormal;
  }

  // Used to implement lookahead
  class Position {
   public:
    Position(Lexer* lexer) : lexer_(lexer), committed_(false) {
      if (lexer_->queue()->length() == 0) {
        offset_ = lexer_->offset_;
      } else {
        offset_ = lexer_->queue()->head()->value()->offset_;
      }
    }

    ~Position() {
      if (!committed_) {
        while (lexer_->queue()->length() > 0) lexer_->queue()->Shift();
        lexer_->offset_ = offset_;
      }
    }

    inline AstNode* Commit(AstNode* result) {
      if (result != NULL) {
        committed_ = true;
      }
      return result;
    }

   private:
    Lexer* lexer_;
    uint32_t offset_;
    bool committed_;
  };

  class NegateSign {
   public:
    NegateSign(Parser* p, TokenType type) : p_(p), sign_(p->sign_) {
      if (sign_ == kNormal && type == kSub) {
        p_->sign_ = kNegated;
      } else if (sign_ == kNegated && type == kAdd) {
        p_->sign_ = kNormal;
      }
    }

    ~NegateSign() {
      p_->sign_ = sign_;
    }
   private:
    Parser* p_;
    ParserSign sign_;
  };

  inline TokenType NegateType(TokenType type) {
    if (sign_ == kNormal) return type;

    switch (type) {
     case kAdd:
      return kSub;
     case kSub:
      return kAdd;
     default:
      return type;
    }
  }

  // AST (result)
  inline AstNode* ast() {
    return ast_;
  }

  inline void SetError(const char* msg) {
    ErrorHandler::SetError(msg, Peek()->offset());
  }

  // Prints AST into buffer (debug purposes only)
  void Print(char* buffer, uint32_t size);

  AstNode* Execute();
  AstNode* ParseStatement(ParseStatementType type);
  AstNode* ParseExpression(int priority = 0);
  AstNode* ParsePrefixUnOp(TokenType type);
  AstNode* ParseBinOp(TokenType type, AstNode* lhs, int priority = 0);
  AstNode* ParsePrimary(PrimaryRestriction rest);
  AstNode* ParseMember();
  AstNode* ParseObjectLiteral();
  AstNode* ParseArrayLiteral();
  AstNode* ParseBlock(AstNode* block);

  ParserSign sign_;

  AstNode* ast_;
};

} // namespace internal
} // namespace candor

#endif // _SRC_PARSER_H_
