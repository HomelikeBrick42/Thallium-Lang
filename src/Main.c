#include "./Typedefs.h"
#include "./DynamicArray.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

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

    TokenKind_LParen,
    TokenKind_RParen,
    TokenKind_LBrace,
    TokenKind_RBrace,
    TokenKind_LBracket,
    TokenKind_RBracket,
    TokenKind_Colon,
    TokenKind_Semicolon,
    TokenKind_Period,
    TokenKind_Caret,

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
} TokenKind;

const char* TokenKindNames[] = {
    [TokenKind_EndOfFile] = "EndOfFile",

    [TokenKind_Name] = "Name",
    [TokenKind_Integer] = "Integer",
    [TokenKind_Float] = "Float",
    [TokenKind_String] = "String",

    [TokenKind_LParen] = "(",
    [TokenKind_RParen] = ")",
    [TokenKind_LBrace] = "{",
    [TokenKind_RBrace] = "}",
    [TokenKind_LBracket] = "[",
    [TokenKind_RBracket] = "]",
    [TokenKind_Colon] = ":",
    [TokenKind_Semicolon] = ";",
    [TokenKind_Period] = ".",
    [TokenKind_Caret] = "^",

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
    };
} Token;

void Error(const char* message, ...) {
    __builtin_va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    putchar('\n');
    ASSERT(FALSE);
    abort();
}

void* Duplicate(const void* src, u64 size) {
    void* block = malloc(size);
    memcpy(block, src, size);
    return block;
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
        CHAR('.', TokenKind_Period);
        CHAR('^', TokenKind_Caret);

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
        
        CHAR2('=', TokenKind_Equals, '=', TokenKind_EqualsEquals);
        CHAR2('+', TokenKind_Plus, '=', TokenKind_PlusEquals);
        CHAR2('-', TokenKind_Minus, '=', TokenKind_MinusEquals);
        CHAR2('*', TokenKind_Asterisk, '=', TokenKind_AsteriskEquals);
        CHAR2('/', TokenKind_Slash, '=', TokenKind_SlashEquals);
        CHAR2('%', TokenKind_Percent, '=', TokenKind_PercentEquals);
        CHAR2('!', TokenKind_ExclamationMark, '=', TokenKind_ExclamationMarkEquals);

        CHAR2('&', TokenKind_Ampersand, '&', TokenKind_AmpersandAmpersand);
        CHAR2('|', TokenKind_Pipe, '|', TokenKind_PipePipe);

        #undef CHAR2

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
            char* name = malloc(DynamicArraySize(buffer));
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
            char* string = malloc(DynamicArraySize(buffer));
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

typedef struct AstType AstType;
typedef struct AstTypeName AstTypeName;
typedef struct AstTypePointer AstTypePointer;

typedef struct AstExpression AstExpression;
typedef struct AstLiteral AstLiteral;
typedef struct AstName AstName;
typedef struct AstUnaryExpression AstUnaryExpression;
typedef struct AstBinaryExpression AstBinaryExpression;
typedef struct AstField AstField;

typedef struct AstStatement AstStatement;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstAssignment AstAssignment;
typedef struct AstStruct AstStruct;

typedef struct Ast Ast;
typedef struct AstStructField AstStructField;

// Type

struct AstTypeName {
    AstName* Name;
};

struct AstTypePointer {
    AstType* PointerTo;
};

typedef enum AstTypeKind {
    AstTypeKind_None,
    AstTypeKind_Integer,
    AstTypeKind_Float,
    AstTypeKind_String,
    AstTypeKind_Name,
    AstTypeKind_Pointer,
} AstTypeKind;

struct AstType {
    AstTypeKind Kind;

    union {
        AstTypeName Name;
        AstTypePointer Pointer;
    };
};

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

typedef enum AstExpressionKind {
    AstExpressionKind_None,
    AstExpressionKind_Literal,
    AstExpressionKind_Name,
    AstExpressionKind_Unary,
    AstExpressionKind_Binary,
    AstExpressionKind_Field,
} AstExpressionKind;

struct AstExpression {
    AstExpressionKind Kind;

    union {
        AstLiteral Literal;
        AstName Name;
        AstUnaryExpression Unary;
        AstBinaryExpression Binary;
        AstField Field;
    };
};

// Statement

struct AstDeclaration {
    AstName* Name;
    AstType* Type;
    AstExpression* Value;
};

struct AstAssignment {
    AstExpression* Left;
    Token Operator;
    AstExpression* Right;
};

struct AstStruct {
    Token Name;
    AstStructField* Fields;
};

typedef enum AstStatementKind {
    AstStatementKind_None,
    AstStatementKind_Expression,
    AstStatementKind_Declaration,
    AstStatementKind_Assignment,
    AstStatementKind_Struct,
} AstStatementKind;

struct AstStatement {
    AstStatementKind Kind;

    union {
        AstExpression Expression;
        AstDeclaration Declaration;
        AstAssignment Assignment;
        AstStruct Struct;
    };
};

// Ast

struct AstStructField {
    Token Name;
    AstType* Type;
};

typedef enum AstKind {
    AstKind_None,
    AstKind_Statement,
    AstKind_StructField,
} AstKind;

struct Ast {
    AstKind Kind;

    union {
        AstStatement Statement;
        AstStructField StructField;
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

AstExpression* Parser_ParseExpression(Parser* parser) {
    AstExpression* Parser_ParseBinaryExpression(Parser* parser, u64 presedence);
    return Parser_ParseBinaryExpression(parser, 0);
}

u64 Parser_GetUnaryPresedence(Token token) {
    switch (token.Kind) {
        case TokenKind_Plus:
        case TokenKind_Minus:
        case TokenKind_Caret:
        case TokenKind_Asterisk:
        case TokenKind_ExclamationMark:
            return 4;
        default:
            return 0;
    }
}

u64 Parser_GetBinaryPresedence(Token token) {
    switch (token.Kind) {
        case TokenKind_Period:
            return 5;
        case TokenKind_Asterisk:
        case TokenKind_Slash:
        case TokenKind_Percent:
        case TokenKind_Ampersand:
        case TokenKind_Pipe:
            return 3;
        case TokenKind_Plus:
        case TokenKind_Minus:
            return 2;
        case TokenKind_AmpersandAmpersand:
        case TokenKind_PipePipe:
            return 1;
        default:
            return 0;
    }
}

AstExpression* Parser_ParsePrimaryExpression(Parser* parser) {
    switch (parser->Current.Kind) {
        case TokenKind_Name: {
            return Duplicate(&(AstExpression){
                .Kind = AstExpressionKind_Name,
                .Name = (AstName){
                    .Name = Parser_NextToken(parser),
                },
            }, sizeof(AstExpression));
        } break;

        case TokenKind_Integer:
        case TokenKind_Float:
        case TokenKind_String: {
            return Duplicate(&(AstExpression){
                .Kind = AstExpressionKind_Literal,
                .Literal = (AstLiteral){
                    .Token = Parser_NextToken(parser),
                },
            }, sizeof(AstExpression));
        } break;

        case TokenKind_LParen: {
            Parser_NextToken(parser);
            AstExpression* expression = Parser_ParseExpression(parser);
            Parser_ExpectToken(parser, TokenKind_RParen);
            return expression;
        } break;

        default: {
            Error("Unexpected token '%s'", TokenKindNames[Parser_NextToken(parser).Kind]);
            return NULL;
        } break;
    }

    ASSERT(FALSE);
    return NULL;
}

AstExpression* Parser_ParseBinaryExpression(Parser* parser, u64 presedence) {
    u64 unaryPresedence = Parser_GetUnaryPresedence(parser->Current);
    AstExpression* left;
    if (unaryPresedence != 0 && unaryPresedence > presedence) {
        Token operator = Parser_NextToken(parser);
        AstExpression* operand = Parser_ParseBinaryExpression(parser, unaryPresedence);
        left = Duplicate(&(AstExpression){
            .Kind = AstExpressionKind_Unary,
            .Unary = (AstUnaryExpression){
                .Operator = operator,
                .Operand = operand,
            },
        }, sizeof(AstExpression));
    } else {
        left = Parser_ParsePrimaryExpression(parser);
    }

    while (TRUE) {
        u64 binaryPresedence = Parser_GetBinaryPresedence(parser->Current);
        if (binaryPresedence == 0 || binaryPresedence <= presedence) {
            break;
        }

        Token operator = Parser_NextToken(parser);

        switch (operator.Kind) {
            case TokenKind_Period: {
                Token name = Parser_ExpectToken(parser, TokenKind_Name);

                left = Duplicate(&(AstExpression){
                    .Kind = AstExpressionKind_Field,
                    .Field = (AstField){
                        .Expression = left,
                        .Name = name,
                    },
                }, sizeof(AstExpression));
            } break;

            default: {
                AstExpression* right = Parser_ParseBinaryExpression(parser, binaryPresedence);
                left = Duplicate(&(AstExpression){
                    .Kind = AstExpressionKind_Binary,
                    .Binary = (AstBinaryExpression){
                        .Left = left,
                        .Operator = operator,
                        .Right = right,
                    },
                }, sizeof(AstExpression));
            } break;
        }
    }

    return left;
}

AstType* Parser_ParseType(Parser* parser) {
    if (parser->Current.Kind == TokenKind_Name) {
        Token nameToken = Parser_ExpectToken(parser, TokenKind_Name);

        AstName* name = Duplicate(&(AstName){
            .Name = nameToken,
        }, sizeof(AstName));

        if (strcmp(name->Name.Name, "int") == 0) {
            return Duplicate(&(AstType){
                .Kind = AstTypeKind_Integer,
            }, sizeof(AstType));
        } else if (strcmp(name->Name.Name, "float") == 0) {
            return Duplicate(&(AstType){
                .Kind = AstTypeKind_Float,
            }, sizeof(AstType));
        } else if (strcmp(name->Name.Name, "string") == 0) {
            return Duplicate(&(AstType){
                .Kind = AstTypeKind_String,
            }, sizeof(AstType));
        }

        return Duplicate(&(AstType){
            .Kind = AstTypeKind_Name,
            .Name = (AstTypeName){
                .Name = name,
            },
        }, sizeof(AstType));
    } else if (parser->Current.Kind == TokenKind_Caret) {
        Parser_NextToken(parser);
        return Duplicate(&(AstType){
            .Kind = AstTypeKind_Pointer,
            .Pointer = (AstTypePointer){
                .PointerTo = Parser_ParseType(parser),
            },
        }, sizeof(AstType));
    }

    ASSERT(FALSE);
    return NULL;
}

AstStatement* Parser_ParseStatement(Parser* parser) {
    if (parser->Current.Kind == TokenKind_Name && strcmp(parser->Current.Name, "struct") == 0) {
        Parser_NextToken(parser);
        
        Token name = Parser_ExpectToken(parser, TokenKind_Name);
        Parser_ExpectToken(parser, TokenKind_LBrace);

        AstStructField* fields = DynamicArrayCreate(AstStructField);
        while (parser->Current.Kind != TokenKind_RBrace) {
            Token name = Parser_NextToken(parser);
            Parser_ExpectToken(parser, TokenKind_Colon);
            AstType* type = Parser_ParseType(parser);
            Parser_ExpectToken(parser, TokenKind_Semicolon);

            DynamicArrayPush(fields, ((AstStructField){
                .Name = name,
                .Type = type,
            }));
        }

        Parser_ExpectToken(parser, TokenKind_RBrace);

        return Duplicate(&(AstStatement){
            .Kind = AstStatementKind_Struct,
            .Struct = (AstStruct){
                .Name = name,
                .Fields = fields,
            },
        }, sizeof(AstStatement));
    } else {
        AstExpression* expression = Parser_ParseExpression(parser);

        switch (parser->Current.Kind) {
            case TokenKind_Colon: {
                Parser_NextToken(parser);

                if (expression->Kind != AstExpressionKind_Name) {
                    Error("There must be a name before a declaration!");
                    return NULL;
                }

                AstType* type = NULL;
                if (parser->Current.Kind != TokenKind_Equals) {
                    type = Parser_ParseType(parser);
                }

                AstExpression* value = NULL;
                if (parser->Current.Kind == TokenKind_Equals) {
                    Parser_NextToken(parser);
                    value = Parser_ParseExpression(parser);
                }

                Parser_ExpectToken(parser, TokenKind_Semicolon);

                if (type == NULL && value == NULL) {
                    Error("Must have type or value for a declaration!");
                    return NULL;
                }

                return Duplicate(&(AstStatement){
                    .Kind = AstStatementKind_Declaration,
                    .Declaration = (AstDeclaration){
                        .Name = Duplicate(&expression->Name, sizeof(AstName)), // TODO: Memory leak
                        .Type = type,
                        .Value = value,
                    },
                }, sizeof(AstStatement));
            } break;

            case TokenKind_Equals:
            case TokenKind_PlusEquals:
            case TokenKind_MinusEquals:
            case TokenKind_AsteriskEquals:
            case TokenKind_SlashEquals:
            case TokenKind_PercentEquals: {
                Token operator = Parser_NextToken(parser);
                AstExpression* right = Parser_ParseExpression(parser);

                Parser_ExpectToken(parser, TokenKind_Semicolon);

                return Duplicate(&(AstStatement){
                    .Kind = AstStatementKind_Assignment,
                    .Assignment = (AstAssignment){
                        .Left = expression,
                        .Operator = operator,
                        .Right = right,
                    },
                }, sizeof(AstStatement));
            } break;

            default: {
                Parser_ExpectToken(parser, TokenKind_Semicolon);

                return Duplicate(&(AstStatement){
                    .Kind = AstStatementKind_Expression,
                    .Expression = *expression,
                }, sizeof(AstStatement));
            } break;
        }
    }

    ASSERT(FALSE);
    return NULL;
}

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

    char* source = malloc(length + 1);
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

    // HACK: Temporary
    while (parser.Current.Kind != TokenKind_EndOfFile) {
        AstStatement* statement = Parser_ParseStatement(&parser);
        void Print_AstStatement(AstStatement* statement);
        Print_AstStatement(statement);
        putchar('\n');
    }

    return 0;
}

void Print_AstType(AstType* type);
void Print_AstExpression(AstExpression* expression);
void Print_AstStatement(AstStatement* statement);

void Print_AstStatement(AstStatement* statement) {
    switch (statement->Kind) {
        case AstStatementKind_Expression: {
            Print_AstExpression(&statement->Expression);
        } break;

        case AstStatementKind_Declaration: {
            printf("%s", statement->Declaration.Name->Name.Name);

            if (statement->Declaration.Type) {
                printf(": ");
                Print_AstType(statement->Declaration.Type);
            } else {
                printf(" := ");
            }

            if (statement->Declaration.Value) {
                if (statement->Declaration.Type) {
                    printf(" = ");
                }
                Print_AstExpression(statement->Declaration.Value);
            }

            putchar(';');
            putchar('\n');
        } break;

        case AstStatementKind_Assignment: {
            Print_AstExpression(statement->Assignment.Left);
            printf(" %s ", TokenKindNames[statement->Assignment.Operator.Kind]);
            Print_AstExpression(statement->Assignment.Right);

            putchar(';');
            putchar('\n');
        } break;

        case AstStatementKind_Struct: {
            printf("struct %s {\n", statement->Struct.Name.Name);
            for (u64 i = 0; i < DynamicArrayLength(statement->Struct.Fields); i++) {
                printf("%s: ", statement->Struct.Fields[i].Name.Name);
                Print_AstType(statement->Struct.Fields[i].Type);
                putchar(';');
                putchar('\n');
            }
            putchar('}');
            putchar('\n');
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}

void Print_AstType(AstType* type) {
    switch (type->Kind) {
        case AstTypeKind_Name: {
            printf("%s", type->Name.Name->Name.Name);
        } break;

        case AstTypeKind_Integer: {
            printf("int");
        } break;

        case AstTypeKind_Float: {
            printf("float");
        } break;

        case AstTypeKind_String: {
            printf("string");
        } break;

        case AstTypeKind_Pointer: {
            putchar('^');
            Print_AstType(type->Pointer.PointerTo);
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}

void Print_AstExpression(AstExpression* expression) {
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
            Print_AstExpression(expression->Unary.Operand);
            putchar(')');
        } break;

        case AstExpressionKind_Binary: {
            putchar('(');
            Print_AstExpression(expression->Binary.Left);
            printf(" %s ", TokenKindNames[expression->Binary.Operator.Kind]);
            Print_AstExpression(expression->Binary.Right);
            putchar(')');
        } break;

        case AstExpressionKind_Field: {
            putchar('(');
            Print_AstExpression(expression->Field.Expression);
            printf(".%s)", expression->Field.Name.Name);
        } break;

        default: {
            ASSERT(FALSE);
        } break;
    }
}
