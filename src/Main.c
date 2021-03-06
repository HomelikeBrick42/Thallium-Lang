#include "./Typedefs.h"
#include "./DynamicArray.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

void* Allocate(u64 size) {
    void* ptr = malloc(size);
    if (!ptr) {
        perror("Allocate failed!");
        abort();
        return NULL;
    }
    memset(ptr, 0, size);
    return ptr;
}

b8 MatchStrings(const char* a, const char* b) {
    // TODO: Intern all strings so pointer comparasion can be used
    return strcmp(a, b) == 0;
}

typedef struct Src {
    const char* Path;
    const char* Source;
    u64 Length;
} Src;

typedef struct SrcPos {
    Src* Src;
    u64 Position;
    u64 Line;
    u64 Column;
} SrcPos;

typedef enum TokenKind {
    TokenKind_EndOfFile,

    TokenKind_Name,
    TokenKind_Integer,
    TokenKind_Float,
    TokenKind_String,
    TokenKind_Keyword,

    TokenKind_LParen,
    TokenKind_RParen,
    TokenKind_LBrace,
    TokenKind_RBrace,
    TokenKind_LBracket,
    TokenKind_RBracket,
    TokenKind_Colon,
    TokenKind_Semicolon,
    TokenKind_Period,
    TokenKind_PeriodPeriod,
    TokenKind_Caret,
    TokenKind_Comma,

    TokenKind_Plus,
    TokenKind_Minus,
    TokenKind_Asterisk,
    TokenKind_Slash,
    TokenKind_Percent,
    TokenKind_Equals,
    TokenKind_ExclamationMark,
    TokenKind_Ampersand,
    TokenKind_Pipe,

    TokenKind_EqualsEquals,
    TokenKind_PlusEquals,
    TokenKind_MinusEquals,
    TokenKind_AsteriskEquals,
    TokenKind_SlashEquals,
    TokenKind_PercentEquals,
    TokenKind_ExclamationMarkEquals,

    TokenKind_AmpersandAmpersand,
    TokenKind_PipePipe,

    TokenKind_RightArrow,
} TokenKind;

const char* TokenKindNames[] = {
    [TokenKind_EndOfFile] = "EndOfFile",

    [TokenKind_Name] = "Name",
    [TokenKind_Integer] = "Integer",
    [TokenKind_Float] = "Float",
    [TokenKind_String] = "String",
    [TokenKind_Keyword] = "Keyword",

    [TokenKind_LParen] = "(",
    [TokenKind_RParen] = ")",
    [TokenKind_LBrace] = "{",
    [TokenKind_RBrace] = "}",
    [TokenKind_LBracket] = "[",
    [TokenKind_RBracket] = "]",
    [TokenKind_Colon] = ":",
    [TokenKind_Semicolon] = ";",
    [TokenKind_Period] = ".",
    [TokenKind_PeriodPeriod] = "..",
    [TokenKind_Caret] = "^",
    [TokenKind_Comma] = ",",

    [TokenKind_Plus] = "+",
    [TokenKind_Minus] = "-",
    [TokenKind_Asterisk] = "*",
    [TokenKind_Slash] = "/",
    [TokenKind_Percent] = "%",
    [TokenKind_Equals] = "=",
    [TokenKind_ExclamationMark] = "!",
    [TokenKind_Ampersand] = "&",
    [TokenKind_Pipe] = "|",

    [TokenKind_EqualsEquals] = "==",
    [TokenKind_PlusEquals] = "+=",
    [TokenKind_MinusEquals] = "-=",
    [TokenKind_AsteriskEquals] = "*=",
    [TokenKind_SlashEquals] = "/=",
    [TokenKind_PercentEquals] = "%=",
    [TokenKind_ExclamationMarkEquals] = "!=",

    [TokenKind_AmpersandAmpersand] = "&&",
    [TokenKind_PipePipe] = "||",

    [TokenKind_RightArrow] = "->",
};

typedef enum Keyword {
    Keyword_True,
    Keyword_False,
    Keyword_Null,
    Keyword_Return,
    Keyword_If,
    Keyword_Else,
    Keyword_Struct,
    Keyword_SizeOf,
    Keyword_Cast,

    Keyword_Count,
} Keyword;

const char* KeywordNames[] = {
    [Keyword_True] = "true",
    [Keyword_False] = "false",
    [Keyword_Null] = "null",
    [Keyword_Return] = "return",
    [Keyword_If] = "if",
    [Keyword_Else] = "else",
    [Keyword_Struct] = "struct",
    [Keyword_SizeOf] = "size_of",
    [Keyword_Cast] = "cast",
};

typedef struct Token {
    TokenKind Kind;
    SrcPos Pos;
    u64 Length;

    union {
        const char* Name;
        u64 Integer;
        f64 Float;
        const char* String;
        Keyword Keyword;
    };
} Token;

b8 TokenIsAssignment(Token token) {
    return
        token.Kind == TokenKind_Equals ||
        token.Kind == TokenKind_PlusEquals ||
        token.Kind == TokenKind_MinusEquals ||
        token.Kind == TokenKind_AsteriskEquals ||
        token.Kind == TokenKind_SlashEquals ||
        token.Kind == TokenKind_PercentEquals;
}

void Error(const char* message, ...) {
    __builtin_va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    putchar('\n');
    ASSERT(FALSE);
    abort();
}

typedef struct Lexer {
    Src Src;
    SrcPos Pos;
} Lexer;

void Lexer_Init(Lexer* lexer, const char* path, const char* source) {
    lexer->Src = (Src){
        .Path = path,
        .Source = source,
        .Length = strlen(source),
    };

    lexer->Pos = (SrcPos){
        .Src = &lexer->Src,
        .Position = 0,
        .Line = 1,
        .Column = 1,
    };
}

char Lexer_PeekChar(Lexer* lexer, u64 offset) {
    u64 index = lexer->Pos.Position + offset;
    if (index >= lexer->Src.Length) {
        return '\0';
    }
    return lexer->Src.Source[index];
}

char Lexer_CurrentChar(Lexer* lexer) {
    return Lexer_PeekChar(lexer, 0);
}

char Lexer_NextChar(Lexer* lexer) {
    char current = Lexer_CurrentChar(lexer);
    lexer->Pos.Position++;
    lexer->Pos.Column++;
    if (current == '\n') {
        lexer->Pos.Line++;
        lexer->Pos.Column = 1;
    }
    return current;
}

Token Lexer_NextToken(Lexer* lexer) {
Start:
    SrcPos startPos = lexer->Pos;

    switch (Lexer_CurrentChar(lexer)) {
        #define CHAR(c, k) \
            case c: { \
                Lexer_NextChar(lexer); \
                return (Token){ \
                    .Kind = k, \
                    .Pos = startPos, \
                    .Length = 1, \
                }; \
            } break

        CHAR('\0', TokenKind_EndOfFile);

        CHAR('(', TokenKind_LParen);
        CHAR(')', TokenKind_RParen);
        CHAR('{', TokenKind_LBrace);
        CHAR('}', TokenKind_RBrace);
        CHAR('[', TokenKind_LBracket);
        CHAR(']', TokenKind_RBracket);
        CHAR(':', TokenKind_Colon);
        CHAR(';', TokenKind_Semicolon);
        CHAR('^', TokenKind_Caret);
        CHAR(',', TokenKind_Comma);

        #undef CHAR

        #define CHAR2(c1, k1, c2, k2) \
            case c1: { \
                Lexer_NextChar(lexer); \
                if (Lexer_CurrentChar(lexer) == c2) { \
                    Lexer_NextChar(lexer); \
                    return (Token){ \
                        .Kind = k2, \
                        .Pos = startPos, \
                        .Length = 2, \
                    }; \
                } else { \
                    return (Token){ \
                        .Kind = k1, \
                        .Pos = startPos, \
                        .Length = 1, \
                    }; \
                } \
            } break
        
        CHAR2('.', TokenKind_Period, '.', TokenKind_PeriodPeriod);

        CHAR2('=', TokenKind_Equals, '=', TokenKind_EqualsEquals);
        CHAR2('+', TokenKind_Plus, '=', TokenKind_PlusEquals);
        CHAR2('*', TokenKind_Asterisk, '=', TokenKind_AsteriskEquals);
        CHAR2('%', TokenKind_Percent, '=', TokenKind_PercentEquals);
        CHAR2('!', TokenKind_ExclamationMark, '=', TokenKind_ExclamationMarkEquals);

        CHAR2('&', TokenKind_Ampersand, '&', TokenKind_AmpersandAmpersand);
        CHAR2('|', TokenKind_Pipe, '|', TokenKind_PipePipe);

        #undef CHAR2

        case '-': {
            Lexer_NextChar(lexer);
            if (Lexer_CurrentChar(lexer) == '=') {
                Lexer_NextChar(lexer);
                return (Token){
                    .Kind = TokenKind_MinusEquals,
                    .Pos = startPos,
                    .Length = 2,
                };
            } else if (Lexer_CurrentChar(lexer) == '>') {
                Lexer_NextChar(lexer);
                return (Token){
                    .Kind = TokenKind_RightArrow,
                    .Pos = startPos,
                    .Length = 2,
                };
            } else {
                return (Token){
                    .Kind = TokenKind_Minus,
                    .Pos = startPos,
                    .Length = 1,
                };
            }
        } break;

        case ' ': case '\t': case '\n': case '\r': {
            while (TRUE) {
                switch (Lexer_CurrentChar(lexer)) {
                    case ' ': case '\t': case '\n': case '\r': {
                        Lexer_NextChar(lexer);
                    } continue;

                    default: {
                    } break;
                }
                break;
            }
        } goto Start;

        case '/': {
            Lexer_NextChar(lexer);
            if (Lexer_CurrentChar(lexer) == '=') {
                Lexer_NextChar(lexer);
                return (Token){
                    .Kind = TokenKind_SlashEquals,
                    .Pos = startPos,
                    .Length = 2,
                };
            } else if (Lexer_CurrentChar(lexer) == '/') {
                while (Lexer_CurrentChar(lexer) != '\n' && Lexer_CurrentChar(lexer) != '\0') {
                    Lexer_NextChar(lexer);
                }
                goto Start;
            } else if (Lexer_CurrentChar(lexer) == '*') {
                Lexer_NextChar(lexer);
                u64 depth = 1;
                while (depth > 0 && Lexer_CurrentChar(lexer) != '\0') {
                    if (Lexer_CurrentChar(lexer) == '/') {
                        Lexer_NextChar(lexer);
                        if (Lexer_CurrentChar(lexer) == '*') {
                            Lexer_NextChar(lexer);
                            depth++;
                        }
                    } else if (Lexer_CurrentChar(lexer) == '*') {
                        Lexer_NextChar(lexer);
                        if (Lexer_CurrentChar(lexer) == '/') {
                            Lexer_NextChar(lexer);
                            depth--;
                        }
                    } else {
                        Lexer_NextChar(lexer);
                    }
                }

                if (Lexer_CurrentChar(lexer) == '\0') {
                    Error("Unexpected end of file in block comment!");
                    return (Token){};
                }

                goto Start;
            } else {
                return (Token){
                    .Kind = TokenKind_Slash,
                    .Pos = startPos,
                    .Length = 1,
                };
            }
        } break;

        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case '_': {
            u64 length = 0;
            char* buffer = DynamicArrayCreate(char);

            while (TRUE) {
                switch (Lexer_CurrentChar(lexer)) {
                    case '0': case '1': case '2': case '3': case '4':
			        case '5': case '6': case '7': case '8': case '9':
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F': case 'G': case 'H': case 'I': case 'J':
                    case 'K': case 'L': case 'M': case 'N': case 'O':
                    case 'P': case 'Q': case 'R': case 'S': case 'T':
                    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                    case 'a': case 'b': case 'c': case 'd': case 'e':
                    case 'f': case 'g': case 'h': case 'i': case 'j':
                    case 'k': case 'l': case 'm': case 'n': case 'o':
                    case 'p': case 'q': case 'r': case 's': case 't':
                    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                    case '_': {
                        length++;
                        DynamicArrayPush(buffer, Lexer_NextChar(lexer));
                    } continue;

                    default: {
                    } break;
                }
                break;
            }

            DynamicArrayPush(buffer, '\0');

            for (u64 i = 0; i < Keyword_Count; i++) {
                if (strcmp(buffer, KeywordNames[i]) == 0) {
                    return (Token) {
                        .Kind = TokenKind_Keyword,
                        .Pos = startPos,
                        .Length = length,
                        .Keyword = i,
                    };
                }
            }

            char* name = Allocate(DynamicArraySize(buffer));
            memcpy(name, buffer, DynamicArraySize(buffer));
            DynamicArrayDestroy(buffer);

            return (Token){
                .Kind = TokenKind_Name,
                .Pos = startPos,
                .Length = length,
                .Name = name,
            };
        } break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            static u64 CharToInt[256] = {
                ['0'] = 0,
                ['1'] = 1,
                ['2'] = 2,
                ['3'] = 3,
                ['4'] = 4,
                ['5'] = 5,
                ['6'] = 6,
                ['7'] = 7,
                ['8'] = 8,
                ['9'] = 9,
                ['a'] = 10, ['A'] = 10,
                ['b'] = 11, ['B'] = 11,
                ['c'] = 12, ['C'] = 12,
                ['d'] = 13, ['D'] = 13,
                ['e'] = 14, ['E'] = 14,
                ['f'] = 15, ['F'] = 15,
            };

            u64 length = 0;
            u64 integerValue = 0;

            u64 base = 10;
            if (Lexer_CurrentChar(lexer) == '0') {
                Lexer_NextChar(lexer);
                switch (Lexer_CurrentChar(lexer)) {
                    case 'x': {
                        Lexer_NextChar(lexer);
                        base = 16;
                    } break;

                    case 'b': {
                        Lexer_NextChar(lexer);
                        base = 2;
                    } break;

                    default: {
                        base = 10;
                    } break;
                }
            }

            while (TRUE) {
                switch (Lexer_CurrentChar(lexer)) {
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F': case 'G': case 'H': case 'I': case 'J':
                    case 'K': case 'L': case 'M': case 'N': case 'O':
                    case 'P': case 'Q': case 'R': case 'S': case 'T':
                    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                    case 'a': case 'b': case 'c': case 'd': case 'e':
                    case 'f': case 'g': case 'h': case 'i': case 'j':
                    case 'k': case 'l': case 'm': case 'n': case 'o':
                    case 'p': case 'q': case 'r': case 's': case 't':
                    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': {
                        length++;
                        u64 value = CharToInt[cast(u8) Lexer_NextChar(lexer)];

                        if (value >= base) {
                            Error("Cannot have digit bigger than base!");
                        }

                        integerValue *= base;
                        integerValue += value;
                    } continue;

                    case '_': {
                        length++;
                        Lexer_NextChar(lexer);
                    } break;

                    case '.': {
                        length++;
                        Lexer_NextChar(lexer);

                        f64 floatValue = cast(f64) integerValue;
                        u64 denominator = 1;

                        while (TRUE) {
                            switch (Lexer_CurrentChar(lexer)) {
                                case '0': case '1': case '2': case '3': case '4':
                                case '5': case '6': case '7': case '8': case '9':
                                case 'A': case 'B': case 'C': case 'D': case 'E':
                                case 'F': case 'G': case 'H': case 'I': case 'J':
                                case 'K': case 'L': case 'M': case 'N': case 'O':
                                case 'P': case 'Q': case 'R': case 'S': case 'T':
                                case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                                case 'a': case 'b': case 'c': case 'd': case 'e':
                                case 'f': case 'g': case 'h': case 'i': case 'j':
                                case 'k': case 'l': case 'm': case 'n': case 'o':
                                case 'p': case 'q': case 'r': case 's': case 't':
                                case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': {
                                    length++;
                                    u64 value = CharToInt[cast(u8) Lexer_NextChar(lexer)];

                                    if (value >= base) {
                                        Error("Cannot have digit bigger than base!");
                                    }

                                    denominator *= base;
                                    floatValue += (cast(f64) value) / (cast(f64) denominator);
                                } continue;

                                case '_': {
                                    length++;
                                    Lexer_NextChar(lexer);
                                } continue;

                                case '.': {
                                    Error("Cannot have more than one '.' in a float literal");
                                } break;

                                default: {
                                } break;
                            }
                            break;
                        }

                        return (Token){
                            .Kind = TokenKind_Float,
                            .Pos = startPos,
                            .Length = length,
                            .Float = floatValue,
                        };
                    } break;

                    default: {
                    } break;
                }
                break;
            }

            return (Token){
                .Kind = TokenKind_Integer,
                .Pos = startPos,
                .Length = length,
                .Integer = integerValue,
            };
        } break;

        case '"': {
            Lexer_NextChar(lexer);

            u64 length = 1;
            char* buffer = DynamicArrayCreate(char);

            while (TRUE) {
                switch (Lexer_CurrentChar(lexer)) {
                    case '\0': {
                        Error("Unexpected end of file in string literal!");
                    } break;

                    case '"': {
                        length++;
                        Lexer_NextChar(lexer);
                    } break;

                    default: {
                        length++;
                        DynamicArrayPush(buffer, Lexer_NextChar(lexer));
                    } continue;
                }
                break;
            }

            DynamicArrayPush(buffer, '\0');
            char* string = Allocate(DynamicArraySize(buffer));
            memcpy(string, buffer, DynamicArraySize(buffer));
            DynamicArrayDestroy(buffer);

            return (Token){
                .Kind = TokenKind_String,
                .Pos = startPos,
                .Length = length,
                .String = string,
            };
        } break;

        default: {
            Error("Unknown character '%c'", Lexer_NextChar(lexer));
        } goto Start;
    }

    ASSERT(FALSE);
    return (Token){};
}

typedef struct AstExpression AstExpression;
typedef struct AstLiteral AstLiteral;
typedef struct AstName AstName;
typedef struct AstUnaryExpression AstUnaryExpression;
typedef struct AstBinaryExpression AstBinaryExpression;
typedef struct AstField AstField;
typedef struct AstStruct AstStruct;
typedef struct AstProcedure AstProcedure;
typedef struct AstStruct AstStruct;
typedef struct AstCall AstCall;
typedef struct AstIndex AstIndex;
typedef struct AstSizeOf AstSizeOf;
typedef struct AstCast AstCast;

typedef struct AstStatement AstStatement;
typedef struct AstScope AstScope;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstAssignment AstAssignment;
typedef struct AstReturn AstReturn;
typedef struct AstIf AstIf;

typedef struct Ast Ast;
typedef struct AstType AstType;

// Expression

struct AstLiteral {
    Token Token;
};

struct AstName {
    Token Name;
};

struct AstUnaryExpression {
    Token Operator;
    AstExpression* Operand;
};

struct AstBinaryExpression {
    AstExpression* Left;
    Token Operator;
    AstExpression* Right;
};

struct AstField {
    AstExpression* Expression;
    Token Name;
};

struct AstStruct {
    AstDeclaration* Declarations;
};

typedef struct AstProcedureArgument {
    Token Name;
    AstType* Type;
} AstProcedureArgument;

struct AstProcedure {
    AstProcedureArgument* Arguments;
    AstType* ReturnType;
    AstScope* Body;
};

struct AstCall {
    AstExpression* Operand;
    AstExpression** Arguments;
};

struct AstIndex {
    AstExpression* Operand;
    AstExpression* Index;
};

struct AstSizeOf {
    AstExpression* Expression;
};

struct AstCast {
    AstType* Type;
    AstExpression* Expression;
};

typedef enum AstExpressionKind {
    AstExpressionKind_None,
    AstExpressionKind_True,
    AstExpressionKind_False,
    AstExpressionKind_Null,
    AstExpressionKind_Literal,
    AstExpressionKind_Name,
    AstExpressionKind_Unary,
    AstExpressionKind_Binary,
    AstExpressionKind_Field,
    AstExpressionKind_Struct,
    AstExpressionKind_Procedure,
    AstExpressionKind_Call,
    AstExpressionKind_Index,
    AstExpressionKind_Sizeof,
    AstExpressionKind_Cast,
} AstExpressionKind;

struct AstExpression {
    AstExpressionKind Kind;
    AstType* Type;
    b8 IsLValue;
    b8 Constant;

    union {
        AstLiteral Literal;
        AstName Name;
        AstUnaryExpression Unary;
        AstBinaryExpression Binary;
        AstField Field;
        AstStruct Struct;
        AstProcedure Procedure;
        AstCall Call;
        AstIndex Index;
        AstSizeOf SizeOf;
        AstCast Cast;
    };
};

// Statement

struct AstScope {
    AstScope* Parent;
    AstStatement** Statements;
};

struct AstDeclaration {
    Token Name;
    AstType* Type;
    AstExpression* Value;
    b8 Constant;
};

struct AstAssignment {
    AstExpression* Operand;
    Token Operator;
    AstExpression* Value;
};

struct AstReturn {
    AstExpression* Expression;
};

struct AstIf {
    AstExpression* Condition;
    AstStatement* Then;
    AstStatement* Else;
};

typedef enum AstStatementKind {
    AstStatementKind_None,
    AstStatementKind_Expression,
    AstStatementKind_Scope,
    AstStatementKind_Declaration,
    AstStatementKind_Assignment,
    AstStatementKind_Return,
    AstStatementKind_If,
} AstStatementKind;

struct AstStatement {
    AstStatementKind Kind;

    union {
        AstExpression Expression;
        AstScope Scope;
        AstDeclaration Declaration;
        AstAssignment Assignment;
        AstReturn Return;
        AstIf If;
    };
};

// Ast

typedef struct AstTypeUnknown {
    Token Name;
} AstTypeUnknown;

typedef struct AstTypePointer {
    AstType* PointerTo;
} AstTypePointer;

typedef struct AstTypeProcedure {
    AstProcedureArgument* Arguments;
    AstType* ReturnType;
} AstTypeProcedure;

typedef struct AstTypeArray {
    AstExpression* Count;
    b8 Dynamic;
    AstType* ArrayOf;
} AstTypeArray;

typedef enum AstTypeKind {
    AstTypeKind_None,
    AstTypeKind_Unknown,
    AstTypeKind_Void,
    AstTypeKind_Type,
    AstTypeKind_Integer,
    AstTypeKind_Float,
    AstTypeKind_String,
    AstTypeKind_Bool,
    AstTypeKind_Pointer,
    AstTypeKind_Procedure,
    AstTypeKind_Struct,
    AstTypeKind_Array,
} AstTypeKind;

typedef enum AstTypeCompletion {
    AstTypeCompletion_Incomplete,
    AstTypeCompletion_Completing,
    AstTypeCompletion_Complete,
} AstTypeCompletion;

struct AstType {
    AstTypeKind Kind;
    AstTypeCompletion Completion;
    u64 Size;

    union {
        AstTypeUnknown Unknown;
        AstTypePointer Pointer;
        AstTypeProcedure Procedure;
        AstStruct Struct;
        AstTypeArray Array;
        b8 Signed;
    };
};

typedef enum AstKind {
    AstKind_None,
    AstKind_Statement,
    AstKind_Type,
} AstKind;

struct Ast {
    AstKind Kind;

    union {
        AstStatement Statement;
        AstType Type;
    };
};

typedef struct Parser {
    Lexer Lexer;
    Token Current;
} Parser;

void Parser_Init(Parser* parser, const char* path, const char* source) {
    Lexer_Init(&parser->Lexer, path, source);
    parser->Current = Lexer_NextToken(&parser->Lexer);
}

Token Parser_NextToken(Parser* parser) {
    Token token = parser->Current;
    parser->Current = Lexer_NextToken(&parser->Lexer);
    return token;
}

Token Parser_ExpectToken(Parser* parser, TokenKind kind) {
    Token token = Parser_NextToken(parser);
    if (token.Kind != kind) {
        Error("Expected '%s' got '%s'", TokenKindNames[kind], TokenKindNames[token.Kind]);
    }
    return token;
}

AstExpression* Parser_ParseExpression(Parser* parser, AstScope* parentScope);
AstExpression* Parser_ParsePrimaryExpression(Parser* parser, AstScope* parentScope);
AstExpression* Parser_ParseBinaryExpression(Parser* parser, u64 presedence, AstScope* parentScope);
AstType* Parser_ParseType(Parser* parser, AstScope* parentScope);
AstStatement* Parser_ParseStatement(Parser* parser, AstScope* parentScope);
AstScope* Parser_ParseScope(Parser* parser, AstScope* parentScope);

AstExpression* Parser_ParseExpression(Parser* parser, AstScope* parentScope) {
    return Parser_ParseBinaryExpression(parser, 0, parentScope);
}

u64 Parser_GetUnaryPresedence(Token token) {
    switch (token.Kind) {
        case TokenKind_Plus:
        case TokenKind_Minus:
        case TokenKind_Caret:
        case TokenKind_Asterisk:
        case TokenKind_ExclamationMark:
            return 5;
        default:
            return 0;
    }
}

u64 Parser_GetBinaryPresedence(Token token) {
    switch (token.Kind) {
        case TokenKind_Period:
            return 6;
        case TokenKind_Asterisk:
        case TokenKind_Slash:
        case TokenKind_Percent:
        case TokenKind_Ampersand:
        case TokenKind_Pipe:
            return 4;
        case TokenKind_Plus:
        case TokenKind_Minus:
            return 3;
        case TokenKind_EqualsEquals:
        case TokenKind_ExclamationMarkEquals:
            return 2;
        case TokenKind_AmpersandAmpersand:
        case TokenKind_PipePipe:
            return 1;
        default:
            return 0;
    }
}

AstExpression* Parser_ParseProcedure(Parser* parser, AstProcedureArgument* firstArg, AstScope* parentScope) {
    AstProcedureArgument* arguments = DynamicArrayCreate(AstProcedureArgument);
    if (firstArg) {
        DynamicArrayPush(arguments, *firstArg); // TODO: Memory leak
    }

    while (parser->Current.Kind != TokenKind_RParen) {
        Parser_ExpectToken(parser, TokenKind_Comma);

        Token nameToken = Parser_ExpectToken(parser, TokenKind_Name);

        Parser_ExpectToken(parser, TokenKind_Colon);
        AstType* type = Parser_ParseType(parser, parentScope);

        DynamicArrayPush(arguments, ((AstProcedureArgument){
            .Name = nameToken,
            .Type = type,
        }));
    }

    Parser_ExpectToken(parser, TokenKind_RParen);

    AstType* returnType = NULL;
    if (parser->Current.Kind == TokenKind_RightArrow) {
        Parser_ExpectToken(parser, TokenKind_RightArrow);
        returnType = Parser_ParseType(parser, parentScope);
    }

    AstScope* body = Parser_ParseScope(parser, parentScope);

    AstExpression* expression = Allocate(sizeof(AstExpression));
    expression->Kind = AstExpressionKind_Procedure;
    expression->Procedure.Arguments = arguments;
    expression->Procedure.ReturnType = returnType;
    expression->Procedure.Body = body;

    return expression;
}

AstExpression* Parser_ParsePrimaryExpression(Parser* parser, AstScope* parentScope) {
    switch (parser->Current.Kind) {
        case TokenKind_Name: {
            Token nameToken = Parser_ExpectToken(parser, TokenKind_Name);

            AstExpression* expression = Allocate(sizeof(AstExpression));
            expression->Kind = AstExpressionKind_Name;
            expression->Name.Name = nameToken;

            return expression;
        } break;

        case TokenKind_Keyword: {
            switch (Parser_ExpectToken(parser, TokenKind_Keyword).Keyword) {
                case Keyword_True: {
                    AstExpression* expression = Allocate(sizeof(AstExpression));
                    expression->Kind = AstExpressionKind_True;
                    return expression;
                } break;

                case Keyword_False: {
                    AstExpression* expression = Allocate(sizeof(AstExpression));
                    expression->Kind = AstExpressionKind_False;
                    return expression;
                } break;

                case Keyword_Null: {
                    AstExpression* expression = Allocate(sizeof(AstExpression));
                    expression->Kind = AstExpressionKind_Null;
                    return expression;
                } break;

                case Keyword_Struct: {
                    AstScope* scope = Parser_ParseScope(parser, parentScope); // TODO: Memory leak

                    AstDeclaration* declarations = DynamicArrayCreate(AstDeclaration);
                    for (u64 i = 0; i < DynamicArrayLength(scope->Statements); i++) {
                        if (scope->Statements[i]->Kind != AstStatementKind_Declaration) {
                            Error("Expected declaration in struct");
                            return NULL;
                        }
                        DynamicArrayPush(declarations, scope->Statements[i]->Declaration);
                    }

                    AstExpression* expression = Allocate(sizeof(AstExpression));
                    expression->Kind = AstExpressionKind_Struct;
                    expression->Struct.Declarations = declarations;
                    return expression;
                } break;

                case Keyword_SizeOf: {
                    Parser_ExpectToken(parser, TokenKind_LParen);
                    AstExpression* expression = Parser_ParseExpression(parser, parentScope);
                    Parser_ExpectToken(parser, TokenKind_RParen);

                    AstExpression* sizeOf = Allocate(sizeof(AstExpression));
                    sizeOf->Kind = AstExpressionKind_Sizeof;
                    sizeOf->SizeOf.Expression = expression;
                    return sizeOf;
                } break;

                case Keyword_Cast: {
                    Parser_ExpectToken(parser, TokenKind_LParen);
                    AstType* type = Parser_ParseType(parser, parentScope);
                    Parser_ExpectToken(parser, TokenKind_RParen);
                    AstExpression* expression = Parser_ParsePrimaryExpression(parser, parentScope);

                    AstExpression* result = Allocate(sizeof(AstExpression));
                    result->Kind = AstExpressionKind_Cast;
                    result->Cast.Type = type;
                    result->Cast.Expression = expression;
                    return result;
                } break;

                default: {
                    goto Default;
                } break;
            }
        } break;

        case TokenKind_Integer:
        case TokenKind_Float:
        case TokenKind_String: {
            Token literalToken = Parser_NextToken(parser);

            AstExpression* expression = Allocate(sizeof(AstExpression));
            expression->Kind = AstExpressionKind_Literal;
            expression->Literal.Token = literalToken;

            return expression;
        } break;

        case TokenKind_LParen: {
            Parser_ExpectToken(parser, TokenKind_LParen);

            if (parser->Current.Kind == TokenKind_RParen) {
                return Parser_ParseProcedure(parser, NULL, parentScope);
            }

            AstExpression* expression = Parser_ParseExpression(parser, parentScope);

            if (parser->Current.Kind == TokenKind_Colon) { // Procedure
                if (expression->Kind != AstExpressionKind_Name) {
                    Error("Expected ':'"); // TODO: Better error
                    return NULL;
                }

                Parser_ExpectToken(parser, TokenKind_Colon);
                AstType* type = Parser_ParseType(parser, parentScope);

                return Parser_ParseProcedure(parser, &(AstProcedureArgument){
                    .Name = expression->Name.Name,
                    .Type = type,
                }, parentScope); // TODO: Pass global scope here
            } else {
                Parser_ExpectToken(parser, TokenKind_RParen);
                return expression;
            }
        } break;

        default: Default: {
            Error("Unexpected token '%s'", TokenKindNames[Parser_NextToken(parser).Kind]);
            return NULL;
        } break;
    }

    ASSERT(FALSE);
    return NULL;
}

AstExpression* Parser_ParseBinaryExpression(Parser* parser, u64 presedence, AstScope* parentScope) {
    u64 unaryPresedence = Parser_GetUnaryPresedence(parser->Current);
    AstExpression* left;
    if (unaryPresedence != 0 && unaryPresedence > presedence) {
        Token operator = Parser_NextToken(parser);
        AstExpression* operand = Parser_ParseBinaryExpression(parser, unaryPresedence, parentScope);
        
        left = Allocate(sizeof(AstExpression));
        left->Kind = AstExpressionKind_Unary;
        left->Unary.Operator = operator;
        left->Unary.Operand = operand;
    } else {
        left = Parser_ParsePrimaryExpression(parser, parentScope);
    }

    while (TRUE) {
        if (parser->Current.Kind == TokenKind_LParen) {
            Parser_ExpectToken(parser, TokenKind_LParen);
            AstExpression** arguments = DynamicArrayCreate(AstExpression*);

            b8 first = TRUE;
            while (parser->Current.Kind != TokenKind_RParen) {
                if (!first) {
                    Parser_ExpectToken(parser, TokenKind_Comma);
                } else {
                    first = FALSE;
                }

                DynamicArrayPush(arguments, Parser_ParseExpression(parser, parentScope));
            }
            Parser_ExpectToken(parser, TokenKind_RParen);

            AstExpression* expression = Allocate(sizeof(AstExpression));
            expression->Kind = AstExpressionKind_Call;
            expression->Call.Operand = left;
            expression->Call.Arguments = arguments;
            left = expression;
        } else if (parser->Current.Kind == TokenKind_LBracket) {
            Parser_ExpectToken(parser, TokenKind_LBracket);
            AstExpression* index = Parser_ParseExpression(parser, parentScope);
            Parser_ExpectToken(parser, TokenKind_RBracket);

            AstExpression* expression = Allocate(sizeof(AstExpression));
            expression->Kind = AstExpressionKind_Index;
            expression->Index.Operand = left;
            expression->Index.Index = index;
            left = expression;
        }

        u64 binaryPresedence = Parser_GetBinaryPresedence(parser->Current);
        if (binaryPresedence == 0 || binaryPresedence <= presedence) {
            break;
        }

        Token operator = Parser_NextToken(parser);

        switch (operator.Kind) {
            case TokenKind_Period: {
                Token nameToken = Parser_ExpectToken(parser, TokenKind_Name);

                AstExpression* newLeft = Allocate(sizeof(AstExpression));
                newLeft->Kind = AstExpressionKind_Field;
                newLeft->Field.Expression = left;
                newLeft->Field.Name = nameToken;

                left = newLeft;
            } break;

            default: {
                AstExpression* right = Parser_ParseBinaryExpression(parser, binaryPresedence, parentScope);

                AstExpression* newLeft = Allocate(sizeof(AstExpression));
                newLeft->Kind = AstExpressionKind_Binary;
                newLeft->Binary.Left = left;
                newLeft->Binary.Operator = operator;
                newLeft->Binary.Right = right;

                left = newLeft;
            } break;
        }
    }

    return left;
}

AstType* Parser_ParseType(Parser* parser, AstScope* parentScope) {
    switch (parser->Current.Kind) {
        case TokenKind_Name: {
            Token nameToken = Parser_ExpectToken(parser, TokenKind_Name);
            AstType* type = Allocate(sizeof(AstType));
            type->Kind = AstTypeKind_Unknown;
            type->Unknown.Name = nameToken;
            return type;
        } break;

        case TokenKind_Caret: {
            Parser_ExpectToken(parser, TokenKind_Caret);
            AstType* type = Allocate(sizeof(AstType));
            type->Kind = AstTypeKind_Pointer;
            type->Pointer.PointerTo = Parser_ParseType(parser, parentScope);
            return type;
        } break;

        case TokenKind_LParen: {
            Parser_ExpectToken(parser, TokenKind_LParen);
            AstType* type = Parser_ParseType(parser, parentScope);
            Parser_ExpectToken(parser, TokenKind_RParen);
            return type;
        } break;

        case TokenKind_LBracket: {
            Parser_ExpectToken(parser, TokenKind_LBracket);
            b8 dynamic = FALSE;
            AstExpression* count = NULL;
            if (parser->Current.Kind == TokenKind_PeriodPeriod) {
                dynamic = TRUE;
            } else if (parser->Current.Kind != TokenKind_RBracket) {
                count = Parser_ParseExpression(parser, parentScope);
            }
            Parser_ExpectToken(parser, TokenKind_RBracket);
            AstType* arrayOf = Parser_ParseType(parser, parentScope);
            
            AstType* type = Allocate(sizeof(AstType));
            type->Kind = AstTypeKind_Array;
            type->Array.Dynamic = dynamic;
            type->Array.Count = count;
            type->Array.ArrayOf = arrayOf;
            return type;
        } break;

        default: {
            ASSERT(FALSE);
            return NULL;
        } break;
    }

    ASSERT(FALSE);
    return NULL;
}

AstStatement* Parser_ParseStatement(Parser* parser, AstScope* parentScope) {
    if (parser->Current.Kind == TokenKind_Semicolon) {
        Parser_ExpectToken(parser, TokenKind_Semicolon);
        return Parser_ParseStatement(parser, parentScope);
    } else if (parser->Current.Kind == TokenKind_LBrace) {
        AstStatement* statement = Allocate(sizeof(AstStatement));
        statement->Kind = AstStatementKind_Scope;
        statement->Scope = *Parser_ParseScope(parser, parentScope); // TODO: Memory leak
        return statement;
    } else if (parser->Current.Kind == TokenKind_Keyword) {
        switch (Parser_ExpectToken(parser, TokenKind_Keyword).Keyword) {
            case Keyword_Return: {
                AstStatement* statement = Allocate(sizeof(AstStatement));
                statement->Kind = AstStatementKind_Return;
                statement->Return.Expression = Parser_ParseExpression(parser, parentScope);
                Parser_ExpectToken(parser, TokenKind_Semicolon);
                return statement;
            } break;

            case Keyword_If: {
                AstExpression* condition = Parser_ParseExpression(parser, parentScope);
                AstStatement* then = Parser_ParseStatement(parser, parentScope);

                AstStatement* else_ = NULL;
                if (parser->Current.Kind == TokenKind_Keyword && parser->Current.Keyword == Keyword_Else) {
                    Parser_ExpectToken(parser, TokenKind_Keyword);
                    else_ = Parser_ParseStatement(parser, parentScope);
                }

                AstStatement* statement = Allocate(sizeof(AstStatement));
                statement->Kind = AstStatementKind_If;
                statement->If.Condition = condition;
                statement->If.Then = then;
                statement->If.Else = else_;
                return statement;
            } break;

            default: {
                ASSERT(FALSE);
                return NULL;
            } break;
        }
    } else {
        AstExpression* expression = Parser_ParseExpression(parser, parentScope);

        if (parser->Current.Kind == TokenKind_Colon) {
            if (expression->Kind != AstExpressionKind_Name) {
                Error("':' must be preceded by a name!");
                return NULL;
            }

            Parser_ExpectToken(parser, TokenKind_Colon);

            AstType* type = NULL;
            if (parser->Current.Kind != TokenKind_Equals && parser->Current.Kind != TokenKind_Colon) {
                type = Parser_ParseType(parser, parentScope);
            }

            b8 constant = FALSE;
            AstExpression* value = NULL;
            if (parser->Current.Kind == TokenKind_Equals || parser->Current.Kind == TokenKind_Colon) {
                if (Parser_NextToken(parser).Kind == TokenKind_Colon) {
                    constant = TRUE;
                }
                value = Parser_ParseExpression(parser, parentScope);
            }

            if (!type && !value) {
                Error("Declaration must have type or value!");
                return NULL;
            }

            if (!value || (value && value->Kind != AstExpressionKind_Procedure && value->Kind != AstExpressionKind_Struct)) {
                Parser_ExpectToken(parser, TokenKind_Semicolon);
            }

            AstStatement* declaration = Allocate(sizeof(AstStatement));
            declaration->Kind = AstStatementKind_Declaration;
            declaration->Declaration.Name = expression->Name.Name;
            declaration->Declaration.Type = type;
            declaration->Declaration.Value = value;
            declaration->Declaration.Constant = constant;
            return declaration;
        } else if (TokenIsAssignment(parser->Current)) {
            Token operator = Parser_NextToken(parser);
            AstExpression* value = Parser_ParseExpression(parser, parentScope);
            Parser_ExpectToken(parser, TokenKind_Semicolon);

            AstStatement* assignment = Allocate(sizeof(AstStatement));
            assignment->Kind = AstStatementKind_Assignment;
            assignment->Assignment.Operand = expression;
            assignment->Assignment.Operator = operator;
            assignment->Assignment.Value = value;
            return assignment;
        } else {
            Parser_ExpectToken(parser, TokenKind_Semicolon);
            AstStatement* statement = Allocate(sizeof(AstStatement));
            statement->Kind = AstStatementKind_Expression;
            statement->Expression = *expression; // Memory leak
            return statement;
        }
    }

    ASSERT(FALSE);
    return NULL;
}

AstScope* Parser_ParseScope(Parser* parser, AstScope* parentScope) {
    AstScope* scope = Allocate(sizeof(AstScope));
    scope->Parent = parentScope;

    Parser_ExpectToken(parser, TokenKind_LBrace);
    AstStatement** statements = DynamicArrayCreate(AstStatement*);

    while (parser->Current.Kind != TokenKind_RBrace) {
        DynamicArrayPush(statements, Parser_ParseStatement(parser, scope));
    }

    Parser_ExpectToken(parser, TokenKind_RBrace);

    scope->Statements = statements;

    return scope;
}

AstStatement* FindDeclaration(const char* name, AstScope* scope, AstScope** scopeFoundIn) {
    if (!scope) {
        if (scopeFoundIn) {
            *scopeFoundIn = NULL;
        }
        return NULL;
    }

    for (u64 i = 0; i < DynamicArrayLength(scope->Statements); i++) {
        if (scope->Statements[i]->Kind == AstStatementKind_Declaration &&
            strcmp(scope->Statements[i]->Declaration.Name.Name, name) == 0) {
            if (scopeFoundIn) {
                *scopeFoundIn = scope;
            }
            return scope->Statements[i];
        }
    }

    if (scope->Parent) {
        return FindDeclaration(name, scope->Parent, scopeFoundIn);
    }

    if (scopeFoundIn) {
        *scopeFoundIn = NULL;
    }
    return NULL;
}

void Complete_Statement(AstStatement* statement, AstScope* parentScope) {
}

void Complete_Expression(AstExpression* expression, AstScope* parentScope) {
    if (!expression->Type) {
        expression->Type = Allocate(sizeof(AstType));
    } else if (expression->Type->Completion == AstTypeCompletion_Complete) {
        return;
    } else if (expression->Type->Completion == AstTypeCompletion_Completing) {
        Error("Cyclic dependency detected!");
        return;
    }

    expression->Type->Completion = AstTypeCompletion_Completing;

    switch (expression->Kind) {
        case AstExpressionKind_Literal: {
            expression->Constant = TRUE;
            switch (expression->Literal.Token.Kind) {
                case TokenKind_Integer: {
                    expression->Type->Kind = AstTypeKind_Integer;
                    expression->Type->Size = 0;
                } break;

                case TokenKind_Float: {
                    expression->Type->Kind = AstTypeKind_Float;
                    expression->Type->Size = 0;
                } break;

                case TokenKind_String: {
                    expression->Type->Kind = AstTypeKind_String;
                    expression->Type->Size = sizeof(u8*) + sizeof(u64); // TODO:
                } break;

                default: {
                    ASSERT(FALSE);
                } break;
            }
        } break;

        case AstExpressionKind_Name: {
            AstScope* foundScope = NULL;
            AstStatement* statement = FindDeclaration(expression->Name.Name.Name, parentScope, &foundScope);
            if (!statement) {
                expression->Type->Completion = AstTypeCompletion_Incomplete;
                Error("Unable to find '%s'", expression->Name.Name.Name);
                return;
            }
            Complete_Statement(statement, foundScope);

            AstType* type = Allocate(sizeof(AstType));
            memcpy(type, statement->Declaration.Type, sizeof(AstType));
            expression->Type = type;
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }

    expression->Type->Completion = AstTypeCompletion_Complete;
}

void Print_AstType(AstType* type, u64 indent);
void Print_AstStatement(AstStatement* statement, u64 indent);
void Print_AstExpression(AstExpression* expression, u64 indent);

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage Thallium.exe [main file]\n");
        return -2;
    }

    const char* path = argv[1];

    FILE* file = fopen(path, "rb");

    fseek(file, 0, SEEK_END);
    u64 length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = Allocate(length + 1);
    fread(source, sizeof(char), length, file);
    source[length] = '\0';

    fclose(file);

#if 0
    Lexer lexer;
    Lexer_Init(&lexer, path, source);

    Token token;
    while ((token = Lexer_NextToken(&lexer)).Kind != TokenKind_EndOfFile) {
        switch (token.Kind) {
            case TokenKind_Name: {
                printf("%s: '%s'\n", TokenKindNames[token.Kind], token.Name);
            } break;

            case TokenKind_Integer: {
                printf("%s: %llu\n", TokenKindNames[token.Kind], token.Integer);
            } break;

            case TokenKind_Float: {
                printf("%s: %f\n", TokenKindNames[token.Kind], token.Float);
            } break;

            case TokenKind_String: {
                printf("%s: \"%s\"\n", TokenKindNames[token.Kind], token.String);
            } break;

            default: {
                printf("%s\n", TokenKindNames[token.Kind]);
            } break;
        }
    }
    
    putchar('\n');
    putchar('\n');
#endif

    Parser parser;
    Parser_Init(&parser, path, source);
    AstStatement* statement = Parser_ParseStatement(&parser, NULL);
    Print_AstStatement(statement, 0);

    return 0;
}

void Print_Indent(u64 indent) {
    for (u64 i = 0; i < indent; i++) {
        printf("    ");
    }
}

void Print_AstType(AstType* type, u64 indent) {
    switch (type->Kind) {
        case AstTypeKind_Unknown: {
            printf("%s", type->Unknown.Name.Name);
        } break;

        case AstTypeKind_Pointer: {
            printf("^");
            Print_AstType(type->Pointer.PointerTo, indent);
        } break;

        case AstTypeKind_Array: {
            printf("[");
            if (type->Array.Dynamic) {
                printf("..");
            } else if (type->Array.Count) {
                Print_AstExpression(type->Array.Count, indent);
            }
            printf("]");
            Print_AstType(type->Array.ArrayOf, indent);
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}

void Print_AstStatement(AstStatement* statement, u64 indent) {
    switch (statement->Kind) {
        case AstStatementKind_Expression: {
            Print_AstExpression(&statement->Expression, indent);
            printf(";\n");
        } break;

        case AstStatementKind_Declaration: {
            Print_Indent(indent);
            printf("%s", statement->Declaration.Name.Name);

            if (statement->Declaration.Type) {
                printf(": ");
                Print_AstType(statement->Declaration.Type, indent);
            } else {
                printf(" :%c ", statement->Declaration.Constant ? ':' : '=');
            }

            if (statement->Declaration.Value) {
                if (statement->Declaration.Type) {
                    printf(" %c ", statement->Declaration.Constant ? ':' : '=');
                }

                Print_AstExpression(statement->Declaration.Value, indent);
            }

            printf(";\n");
        } break;

        case AstStatementKind_Assignment: {
            Print_Indent(indent);
            Print_AstExpression(statement->Assignment.Operand, indent);
            printf(" %s ", TokenKindNames[statement->Assignment.Operator.Kind]);
            Print_AstExpression(statement->Assignment.Value, indent);
            printf(";\n");
        } break;

        case AstStatementKind_Scope: {
            printf("{\n");
            for (u64 i = 0; i < DynamicArrayLength(statement->Scope.Statements); i++) {
                Print_AstStatement(statement->Scope.Statements[i], indent + 1);
            }
            Print_Indent(indent);
            printf("}");
        } break;

        case AstStatementKind_Return: {
            Print_Indent(indent);
            printf("return ");
            Print_AstExpression(statement->Return.Expression, indent);
            printf(";\n");
        } break;

        case AstStatementKind_If: {
            Print_Indent(indent);
            printf("if ");
            Print_AstExpression(statement->If.Condition, indent);

            if (statement->If.Then->Kind != AstStatementKind_Scope) {
                printf("\n");
                Print_Indent(indent);
            } else {
                printf(" ");
            }
            Print_AstStatement(statement->If.Then, indent);

            if (statement->If.Else) {
                if (statement->If.Then->Kind == AstStatementKind_Scope) {
                    printf(" ");
                } else {
                    Print_Indent(indent);
                }

                printf("else ");
                if (statement->If.Else->Kind != AstStatementKind_Scope) {
                    printf("\n");
                    Print_Indent(indent);
                }

                Print_AstStatement(statement->If.Else, indent);
            }

            printf("\n");
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}

void Print_AstExpression(AstExpression* expression, u64 indent) {
    switch (expression->Kind) {
        case AstExpressionKind_Name: {
            printf("%s", expression->Name.Name.Name);
        } break;

        case AstExpressionKind_Literal: {
            switch (expression->Literal.Token.Kind) {
                case TokenKind_Integer: {
                    printf("%llu", expression->Literal.Token.Integer);
                } break;

                case TokenKind_Float: {
                    printf("%f", expression->Literal.Token.Float);
                } break;

                case TokenKind_String: {
                    printf("\"%s\"", expression->Literal.Token.String);
                } break;

                default: {
                    ASSERT(FALSE);
                } break;
            }
        } break;

        case AstExpressionKind_Unary: {
            printf("(%s ", TokenKindNames[expression->Unary.Operator.Kind]);
            Print_AstExpression(expression->Unary.Operand, indent);
            putchar(')');
        } break;

        case AstExpressionKind_Binary: {
            putchar('(');
            Print_AstExpression(expression->Binary.Left, indent);
            printf(" %s ", TokenKindNames[expression->Binary.Operator.Kind]);
            Print_AstExpression(expression->Binary.Right, indent);
            putchar(')');
        } break;

        case AstExpressionKind_Field: {
            putchar('(');
            Print_AstExpression(expression->Field.Expression, indent);
            printf(".%s)", expression->Field.Name.Name);
        } break;

        case AstExpressionKind_Procedure: {
            printf("(");
            for (u64 i = 0; i < DynamicArrayLength(expression->Procedure.Arguments); i++) {
                if (i > 0) {
                    printf(", ");
                }

                printf("%s: ", expression->Procedure.Arguments[i].Name.Name);
                Print_AstType(expression->Procedure.Arguments[i].Type, indent);
            }
            printf(")");

            if (expression->Procedure.ReturnType) {
                printf(" -> ");
                Print_AstType(expression->Procedure.ReturnType, indent);
            }

            printf(" ");
            Print_AstStatement(&(AstStatement){
                .Kind = AstStatementKind_Scope,
                .Scope = *expression->Procedure.Body,
            }, indent);
        } break;

        case AstExpressionKind_Struct: {
            printf("struct {\n");
            for (u64 i = 0; i < DynamicArrayLength(expression->Struct.Declarations); i++) {
                AstStatement statement = {};
                statement.Kind = AstStatementKind_Declaration;
                statement.Declaration = expression->Struct.Declarations[i];
                Print_AstStatement(&statement, indent + 1);
            }
            Print_Indent(indent);
            printf("}");
        } break;

        case AstExpressionKind_True: {
            printf("true");
        } break;

        case AstExpressionKind_False: {
            printf("false");
        } break;

        case AstExpressionKind_Null: {
            printf("null");
        } break;

        case AstExpressionKind_Call: {
            Print_AstExpression(expression->Call.Operand, indent);
            printf("(");
            for (u64 i = 0; i < DynamicArrayLength(expression->Call.Arguments); i++) {
                if (i > 0) {
                    printf(", ");
                }
                Print_AstExpression(expression->Call.Arguments[i], indent);
            }
            printf(")");
        } break;

        case AstExpressionKind_Index: {
            Print_AstExpression(expression->Index.Operand, indent);
            printf("[");
            Print_AstExpression(expression->Index.Index, indent);
            printf("]");
        } break;

        case AstExpressionKind_Sizeof: {
            printf("size_of(");
            Print_AstExpression(expression->SizeOf.Expression, indent);
            printf(")");
        } break;

        case AstExpressionKind_Cast: {
            printf("(cast(");
            Print_AstType(expression->Cast.Type, indent);
            printf(") ");
            Print_AstExpression(expression->Cast.Expression, indent);
            printf(")");
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}
