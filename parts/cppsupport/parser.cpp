/***************************************************************************
 *   Copyright (C) 2002 by Roberto Raggi                                   *
 *   raggi@cli.di.unipi.it                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "parser.h"
#include "driver.h"
#include "lexer.h"
#include "errors.h"
#include "problemreporter.h"

#include <qstring.h>
#include <qstringlist.h>

#include <klocale.h>
#include <kdebug.h>

using namespace CppSupport;
using namespace std;


#define ADVANCE(tk, descr) \
{ \
    Token token = lex->lookAhead( 0 ); \
    if( token != tk ){ \
        reportError( Errors::SyntaxError ); \
        return 0; \
    } \
    lex->nextToken(); \
}

#define MATCH(tk, descr) \
{ \
    Token token = lex->lookAhead( 0 ); \
    if( token != tk ){ \
        reportError( Errors::SyntaxError ); \
        return 0; \
    } \
}

Parser::Parser( ProblemReporter* pr, Driver* drv, Lexer* lexer )
    : m_problemReporter( pr ),
      driver( drv ),
      lex( lexer )
{
    m_fileName = "<stdin>";

    m_maxErrors = 5;
    resetErrors();
}

Parser::~Parser()
{
}

void Parser::setFileName( const QString& fileName )
{
    m_fileName = fileName;
}

bool Parser::reportError( const Error& err )
{
    if( m_errors < m_maxErrors ){
        int line=0, col=0;
        const Token& token = lex->lookAhead( 0 );
        lex->getTokenPosition( token, &line, &col );

        m_problemReporter->reportError( err.text.arg(lex->lookAhead(0).toString()),
                                        m_fileName,
                                        line,
                                        col );
    }

    ++m_errors;

    return true;
}

bool Parser::reportError( const QString& msg )
{
    if( m_errors < m_maxErrors ){
        int line=0, col=0;
        const Token& token = lex->lookAhead( 0 );
        lex->getTokenPosition( token, &line, &col );

        m_problemReporter->reportError( msg,
                                        m_fileName,
                                        line,
                                        col );
    }

    ++m_errors;

    return true;
}

void Parser::syntaxError()
{
    reportError( Errors::SyntaxError );
}

void Parser::parseError()
{
    reportError( Errors::ParseError );
}

bool Parser::skipUntil( int token )
{
    while( !lex->lookAhead(0).isNull() ){
        if( lex->lookAhead(0) == token )
            return true;

        lex->nextToken();
    }

    return false;
}

bool Parser::skipUntilDeclaration()
{
    while( !lex->lookAhead(0).isNull() ){
        switch( lex->lookAhead(0) ){
        case ';':
        case '~':
        case Token_scope:
        case Token_identifier:
        case Token_operator:
        case Token_char:
        case Token_wchar_t:
        case Token_bool:
        case Token_short:
        case Token_int:
        case Token_long:
        case Token_signed:
        case Token_unsigned:
        case Token_float:
        case Token_double:
        case Token_void:
        case Token_extern:
        case Token_namespace:
        case Token_using:
        case Token_typedef:
        case Token_asm:
        case Token_template:
        case Token_export:

        case Token_const:       // cv
        case Token_volatile:    // cv

        case Token_public:
        case Token_protected:
        case Token_private:
        case Token_signals:      // Qt
        case Token_slots:        // Qt
            return true;

        default:
            lex->nextToken();
        }
    }

    return false;
}

bool Parser::skip( int l, int r )
{
    int count = 0;
    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );

        if( tk == l )
            ++count;
        else if( tk == r )
            --count;

        if( count == 0 )
            return true;

        lex->nextToken();
    }

    return false;
}

bool Parser::parseName()
{
    //kdDebug(9007) << "Parser::parseName()" << endl;

    if( lex->lookAhead(0) == Token_scope ){
        lex->nextToken();
    }

    parseNestedNameSpecifier();
    parseUnqualiedName();
    return true;
}

TranslationUnitAST* Parser::parseTranslationUnit()
{
    TranslationUnitAST* ast = new TranslationUnitAST();

    //kdDebug(9007) << "Parser::parseTranslationUnit()" << endl;
    while( !lex->lookAhead(0).isNull() ){
        DeclarationAST* decl = parseDefinition();
        if( decl ){
            ast->addDeclaration( decl );
        } else {
            // error recovery
            syntaxError();
            int tk = lex->lookAhead( 0 );
            skipUntilDeclaration();
            if( lex->lookAhead(0) == tk ){
                lex->nextToken();
            }
        }
    }

    return ast;
}

DeclarationAST* Parser::parseDefinition()
{
    //kdDebug(9007) << "Parser::parseDefinition()" << endl;

    DeclarationAST* ast = 0;

    switch( lex->lookAhead(0) ){

    case ';':
        ast = new NullDeclarationAST();
        ast->setStart( lex->index() );
        lex->nextToken();
        ast->setEnd( lex->index() );
        break;

    case Token_extern:
        ast = parseLinkageSpecification();
        break;

    case Token_namespace:
        ast = parseNamespace();
        break;

    case Token_using:
        ast = parseUsing();
        break;

    case Token_typedef:
        ast = parseTypedef();
        break;

    case Token_asm:
        ast = parseAsmDefinition();
        break;

    case Token_template:
    case Token_export:
        ast = parseTemplateDeclaration();
        break;

    default:
        ast = parseDeclaration();

    } // end switch

    return ast;
}


DeclarationAST* Parser::parseLinkageSpecification()
{
    //kdDebug(9007) << "Parser::parseLinkageSpecification()" << endl;

    DeclarationAST* ast = 0;
    int start = lex->index();

    if( lex->lookAhead(0) != Token_extern ){
        return 0;
    }
    lex->nextToken();

    if( lex->lookAhead(0) == Token_string_literal ){
        lex->nextToken(); // skip string literal
    }

    if( lex->lookAhead(0) == '{' ){
        ast = parseLinkageBody();
    } else {
        ast = parseDefinition();
        if( !ast ){
            reportError( i18n("Declaration syntax error") );
            return 0;
        }
    }

    if( ast ){
        ast->setStart( start );
    }

    return ast;
}

LinkageBodyAST* Parser::parseLinkageBody()
{
    //kdDebug(9007) << "Parser::parseLinkageBody()" << endl;

    LinkageBodyAST* ast = 0;
    int start, end;

    start = lex->index();

    if( lex->lookAhead(0) != '{' ){
        return 0;
    }
    lex->nextToken();

    ast = new LinkageBodyAST();

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );

        if( tk == '}' )
            break;

        DeclarationAST* decl = parseDefinition();
        if( decl ){
            ast->addDeclaration( decl );
        } else {
            // error recovery
            // TODO: skip until next declaration
            syntaxError();
            if( skipUntil(';') ){
                lex->nextToken(); // skip ;
            }
        }
    }

    if( lex->lookAhead(0) != '}' ){
        reportError( i18n("} expected") );
        skipUntil( '}' );
    }
    lex->nextToken();

    end = lex->index();

    ast->setStart( start );
    ast->setEnd( end );

    return ast;
}

DeclarationAST* Parser::parseNamespace()
{
    //kdDebug(9007) << "Parser::parseNamespace()" << endl;

    DeclarationAST* ast = 0;
    int start, end;
    int nameStart, nameEnd;

    start = lex->index();

    if( lex->lookAhead(0) != Token_namespace ){
        return ast;
    }
    lex->nextToken();

    nameStart = lex->index();
    if( lex->lookAhead(0) == Token_identifier ){
        lex->nextToken();
    }
    nameEnd = lex->index();

    if( lex->lookAhead(0) != '{' ){
        reportError( i18n("{ expected") );
        skipUntil( '{' );
    }

    if( parseLinkageBody() ){
    }

    end = lex->index();

    ast = new NamespaceDeclarationAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setNameStart( nameStart );
    ast->setNameEnd( nameEnd );

    return ast;
}

DeclarationAST* Parser::parseUsing()
{
    //kdDebug(9007) << "Parser::parseUsing()" << endl;

    DeclarationAST* ast = 0;
    int start, end;
    int nameStart, nameEnd;

    start = lex->index();

    if( lex->lookAhead(0) != Token_using ){
        return 0;
    }
    lex->nextToken();

    if( lex->lookAhead(0) == Token_namespace ){
        ast = parseUsingDirective();
        if( ast )
            ast->setStart( start );

        return ast;
    }

    if( lex->lookAhead(0) == Token_typename ){
        lex->nextToken();
    }

    nameStart = lex->index();
    if( !parseName() ){
        reportError( i18n("namespace name expected") );
    }
    nameEnd = lex->index();
    lex->nextToken();

    if( lex->lookAhead(0) != ';' ){
        reportError( i18n("; expected") );
    }
    lex->nextToken(); // skip ;

    end = lex->index();

    ast = new UsingDeclarationAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setNameStart( nameStart );
    ast->setNameEnd( nameEnd );

    return ast;
}

DeclarationAST* Parser::parseUsingDirective()
{
    //kdDebug(9007) << "Parser::parseUsingDirective()" << endl;

    UsingDeclarationAST* ast = 0;
    int start, end;
    int nameStart, nameEnd;

    start = lex->index();

    if( lex->lookAhead(0) != Token_namespace ){
        return 0;
    }
    lex->nextToken();


    nameStart = lex->index();
    if( !parseName() ){
        reportError( i18n("Namespace name expected") );
    }
    nameEnd = lex->index();

    if( lex->lookAhead(0) != ';' ){
        reportError( i18n("; expected") );
        skipUntil( ';' );
    }
    lex->nextToken();

    end = lex->index();

    ast = new UsingDeclarationAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setNameStart( nameStart );
    ast->setNameEnd( nameEnd );

    return ast;
}


bool Parser::parseOperatorFunctionId()
{
    //kdDebug(9007) << "Parser::parseOperatorFunctionId()" << endl;
    if( lex->lookAhead(0) != Token_operator ){
        return false;
    }
    lex->nextToken();


    if( parseOperator() )
        return true;
    else {
        // parse cast operator
        QStringList cv;
        if( parseCvQualify(cv) ){
        }

        if( !parseSimpleTypeSpecifier() ){
            parseError();
        }

        if( parseCvQualify(cv) ){
        }

        while( parsePtrOperator() ){
        }

        return true;
    }
}

bool Parser::parseTemplateArgumentList()
{
    //kdDebug(9007) << "Parser::parseTemplateArgumentList()" << endl;

    if( !parseTemplateArgument() )
        return false;

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseTemplateArgument() ){
        } else {
            parseError();
            break;
        }
    }

    return true;
}

DeclarationAST* Parser::parseTypedef()
{
    //kdDebug(9007) << "Parser::parseTypedef()" << endl;

    TypedefDeclarationAST* ast = 0;
    int start, end;
    int nameStart=0, nameEnd=0;

    start = lex->index();
    if( lex->lookAhead(0) != Token_typedef ){
        return 0;
    }
    lex->nextToken();


    if( !parseTypeSpecifier() ){
        reportError( i18n("Need a type specifier to declare") );
    }

    DeclaratorAST* declarator = parseDeclarator();
    if( declarator ){
        nameStart = declarator->nameStart();
        nameEnd = declarator->nameEnd();
    } else {
        reportError( i18n("Need an identifier to declare") );
    }

    if( lex->lookAhead(0) != ';' ){
        reportError( i18n("} expected") );
        skipUntil( ';' );
    }
    lex->nextToken();

    end = lex->index();

    ast = new TypedefDeclarationAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setNameStart( nameStart );
    ast->setNameEnd( nameEnd );
    ast->setDeclarator( declarator );

    return ast;
}

DeclarationAST* Parser::parseAsmDefinition()
{
    //kdDebug(9007) << "Parser::parseAsmDefinition()" << endl;

    DeclarationAST* ast = 0;
    int start=0, end=0;

    start = lex->index();

    ADVANCE( Token_asm, "asm" );
    ADVANCE( '(', '(' );

    MATCH( Token_string_literal, "string literal" );
    lex->nextToken();

    ADVANCE( ')', ')' );
    ADVANCE( ';', ';' );

    end = lex->index();

    ast = new AsmDeclaratationAST();
    ast->setStart( start );
    ast->setEnd( end );

    return ast;
}

DeclarationAST* Parser::parseTemplateDeclaration()
{
    //kdDebug(9007) << "Parser::parseTemplateDeclaration()" << endl;

    TemplateDeclarationAST* ast = 0;
    int start, end;
    int argStart, argEnd;
    bool _export = false;

    start = lex->index();

    if( lex->lookAhead(0) == Token_export ){
        _export = true;
        lex->nextToken();
    }

    if( lex->lookAhead(0) != Token_template ){
        if( _export ){
            ADVANCE( Token_template, "template" );
        } else
            return 0;
    }

    ADVANCE( Token_template, "template" );

    argStart = lex->index();
    if( lex->lookAhead(0) == '<' ){
        lex->nextToken();

        if( parseTemplateParameterList() ){
        }

        if( lex->lookAhead(0) != '>' ){
            reportError( i18n("> expected") );
            skipUntil( '>' );
        }
        lex->nextToken();
    }
    argEnd = lex->index();

    DeclarationAST* decl = parseDefinition();
    if( !decl ){
        reportError( i18n("expected a declaration") );
    }
    end = lex->index();

    ast = new TemplateDeclarationAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setDeclaration( decl );

    return ast;
}

DeclarationAST* Parser::parseDeclaration()
{
    //kdDebug(9007) << "Parser::parseDeclaration()" << endl;

    DeclarationAST* ast = 0;
    ast = parseIntegralDeclaration();
    if( ast != 0 )
        return ast;

    ast = parseConstDeclaration();
    if( ast != 0 )
        return ast;

    ast = parseOtherDeclaration();
    if( ast != 0 )
        return ast;

    return 0;
}

bool Parser::parseDeclHead()
{
    //kdDebug(9007) << "Parser::parseDeclHead()" << endl;

    while( parseStorageClassSpecifier() || parseFunctionSpecifier() ){
    }

    QStringList cv;
    if( parseCvQualify(cv) ){
    }

    return true;
}

DeclarationAST* Parser::parseIntegralDeclaration()
{
    //kdDebug(9007) << "Parser::parseIntegralDeclaration()" << endl;
    int index = lex->index();

    if( !parseIntegralDeclHead() ){
        lex->setIndex( index );
        return 0;
    }

    int tk = lex->lookAhead( 0 );
    if( tk == ';' ){
        lex->nextToken();
    } else if( tk == ':' ){
        lex->nextToken();
        if( !parseConstantExpression() ){
            lex->setIndex( index );
            return 0;
        }
        ADVANCE( ';', ';' );
    } else {
        DeclaratorListAST* declarators = parseInitDeclaratorList();
        if( !declarators ){
            lex->setIndex( index );
            return 0;
        }
        delete( declarators );

        if( lex->lookAhead(0) == ';' ){
            lex->nextToken();
        } else {
            if( !parseFunctionBody() ){
                lex->setIndex( index );
                return 0;
            }
        }
    }

    DeclarationAST* ast = 0;
    return ast;
}

bool Parser::parseIntegralDeclHead()
{
    //kdDebug(9007) << "Parser::parseIntegralDeclHead()" << endl;

    if( !parseDeclHead() ){
        return false;
    }

    if( !parseTypeSpecifier() ){
        return false;
    }

    QStringList cv;
    parseCvQualify( cv );

    return true;
}

DeclarationAST* Parser::parseOtherDeclaration()
{
    //kdDebug(9007) << "Parser::parseOtherDeclaration()" << endl;

    int start, end;

    start = lex->index();

    if( lex->lookAhead(0) == Token_friend ){
        lex->nextToken();
        DeclarationAST* decl = parseDefinition();
        if( decl ){
            reportError( i18n("expected a declaration") );
            return 0;
        }
        end = lex->index();

        FriendDeclarationAST* ast = new FriendDeclarationAST();
        ast->setStart( start );
        ast->setEnd( end );
        ast->setDeclaration( decl );
        return ast;

    } else {

        if( !parseDeclHead() ){
            return 0;
        }

        int nameStart = lex->index();
        if( !parseName() ){
            return 0;
        }
        int nameEnd = lex->index();

        if( isConstructorDecl() ){
            if( !parseConstructorDeclaration() ){
                return 0;
            }
        } else {
            QStringList cv;
            if( parseCvQualify(cv) ){
            }

            DeclaratorListAST* declarators = parseInitDeclaratorList();
            if( !declarators ){
                return 0;
            }
            delete( declarators );
        }

        if( lex->lookAhead(0) == ';' ){
            // constructor/destructor or cast operator
            lex->nextToken();
        } else {
            if( !parseFunctionBody() ){
                return 0;
            }
        }

        end = lex->index();
    }

    return 0;
}

DeclarationAST* Parser::parseConstDeclaration()
{
    //kdDebug(9007) << "Parser::parseConstDeclaration()" << endl;
    int start, end;

    start = lex->index();

    if( lex->lookAhead(0) != Token_const ){
        return 0;
    }
    lex->nextToken();

    int index = lex->index();

    DeclaratorListAST* declarators = parseInitDeclaratorList();
    if( !declarators ){
        lex->setIndex( index );
        return 0;
    }
    delete( declarators );

    ADVANCE( ';', ';' );

    end = lex->index();

    DeclarationAST* ast = 0;
    return ast;
}

bool Parser::isConstructorDecl()
{
    //kdDebug(9007) << "Parser::isConstructorDecl()" << endl;

    if( lex->lookAhead(0) != '(' ){
        return false;
    }

    int tk = lex->lookAhead( 1 );
    switch( tk ){
    case '*':
    case '&':
    case '(':
        return false;

    case Token_const:
    case Token_volatile:
        return true;

    case Token_scope:
    {
        int index = lex->index();
        if( parsePtrToMember() )
            return false;

        lex->setIndex( index );
    }
    break;

    }

    return true;
}

bool Parser::parseConstructorDeclaration() // or castoperator declaration
{
    //kdDebug(9007) << "Parser::parseConstructorDeclaration()" << endl;

    if( lex->lookAhead(0) != '(' ){
        return false;
    }
    lex->nextToken();


    if( !parseParameterDeclarationClause() ){
        return false;
    }

    if( lex->lookAhead(0) != ')' ){
        return false;
    }
    lex->nextToken();

    if( lex->lookAhead(0) == Token_const ){
        lex->nextToken();
    }

    if( lex->lookAhead(0) == ':' ){
        if( !parseCtorInitializer() ){
            return false;
        }
    }

    return true;
}

bool Parser::parseOperator()
{
    //kdDebug(9007) << "Parser::parseOperator()" << endl;

    switch( lex->lookAhead(0) ){
    case Token_new:
    case Token_delete:
        lex->nextToken();
        if( lex->lookAhead(0) == '[' && lex->lookAhead(1) == ']' ){
            lex->nextToken();
            lex->nextToken();
        } else {
        }
        return true;

    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
    case '&':
    case '|':
    case '~':
    case '!':
    case '=':
    case '<':
    case '>':
    case ',':
    case Token_assign:
    case Token_shift:
    case Token_eq:
    case Token_not_eq:
    case Token_leq:
    case Token_geq:
    case Token_and:
    case Token_or:
    case Token_incr:
    case Token_decr:
    case Token_ptrmem:
    case Token_arrow:
        lex->nextToken();
        return true;

    default:
        if( lex->lookAhead(0) == '(' && lex->lookAhead(1) == ')' ){
            lex->nextToken();
            lex->nextToken();
            return true;
        } else if( lex->lookAhead(0) == '[' && lex->lookAhead(1) == ']' ){
            lex->nextToken();
            lex->nextToken();
            return true;
        }
    }

    return false;
}

bool Parser::parseCvQualify( QStringList& l )
{
    //kdDebug(9007) << "Parser::parseCvQualify()" << endl;

    int n = 0;
    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );
        if( tk == Token_const || tk == Token_volatile ){
            ++n;
            l << lex->lookAhead( 0 ).toString();
            lex->nextToken();
        } else
            break;
    }
    return n != 0;
}

bool Parser::parseSimpleTypeSpecifier()
{
    //kdDebug(9007) << "Parser::parseSimpleTypeSpecifier()" << endl;

    bool isIntegral = false;

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );

        if( tk == Token_char || tk == Token_wchar_t || tk == Token_bool || tk == Token_short ||
            tk == Token_int || tk == Token_long || tk == Token_signed || tk == Token_unsigned ||
            tk == Token_float || tk == Token_double || tk == Token_void ){

            isIntegral = true;
            lex->nextToken();
        } else if( isIntegral ){
            return true;
        } else
            break;
    }

    return parseName();
}

bool Parser::parsePtrOperator()
{
    //kdDebug(9007) << "Parser::parsePtrOperator()" << endl;

    if( lex->lookAhead(0) == '&' ){
        lex->nextToken();
    } else if( lex->lookAhead(0) == '*' ){
        lex->nextToken();
    } else {
        int index = lex->index();
        if( !parsePtrToMember() ){
            lex->setIndex( index );
            return false;
        }
    }

    QStringList cv;
    if( parseCvQualify(cv) ){
    }

    return true;
}


bool Parser::parseTemplateArgument()
{
    //kdDebug(9007) << "Parser::parseTemplateArgument()" << endl;

#warning "TODO Parser::parseTemplateArgument()"
#warning "parse type id"

#if 0
    if( parseTypeId() ){
        qWarning( "token = %s", lex->lookAhead(0).toString().latin1() );
        return true;
    }
#endif

    return parseAssignmentExpression();
}

bool Parser::parseTypeSpecifier()
{
    //kdDebug(9007) << "Parser::parseTypeSpecifier()" << endl;

    QStringList cv;
    parseCvQualify(cv);

    if( parseClassSpecifier() || parseEnumSpecifier() ||
        parseElaboratedTypeSpecifier() || parseSimpleTypeSpecifier() ){

        if( parseCvQualify(cv) ){
        }
    } else
        return false;

    return true;
}

DeclaratorAST* Parser::parseDeclarator()
{
    //kdDebug(9007) << "Parser::parseDeclarator()" << endl;

    DeclaratorAST* ast = 0;
    DeclaratorAST* sub = 0;
    int start, end;
    int nameStart=0, nameEnd=0;

    start = lex->index();
    while( parsePtrOperator() ){
    }

    if( lex->lookAhead(0) == '(' ){
        lex->nextToken();

        sub = parseDeclarator();
        if( !sub ){
            return 0;
        }
        if( lex->lookAhead(0) != ')'){
            return 0;
        }
        lex->nextToken();
    } else {
        nameStart = lex->index();
        if( !parseDeclaratorId() ){
            return 0;
        }
        nameEnd = lex->index();

        if( lex->lookAhead(0) == ':' ){
            lex->nextToken();
            if( !parseConstantExpression() ){
                reportError( i18n("Constant expression expected") );
                return 0;
            }
            return 0;
        }
    }

    while( lex->lookAhead(0) == '[' ){
        lex->nextToken();
        if( !parseCommaExpression() ){
            reportError( i18n("Expression expected") );
        }

        if( lex->lookAhead(0) != ']' ){
            reportError( i18n("] expected") );
            skipUntil( ']' );
        }
        lex->nextToken();
    }

    if( lex->lookAhead(0) == '(' ){
        // parse function arguments
        lex->nextToken();

        if( !parseParameterDeclarationClause() ){
            return 0;
        }
        if( lex->lookAhead(0) != ')' ){
            reportError( i18n(") expected") );
            skipUntil( ')' );
        }
        lex->nextToken();

        QStringList cv;
        if( parseCvQualify(cv) ){
        }

        if( parseExceptionSpecification() ){
        }
    }
    end = lex->index();

    ast = new DeclaratorAST();
    ast->setStart( start );
    ast->setEnd( end );
    ast->setNameStart( nameStart );
    ast->setNameEnd( nameEnd );
    ast->setSubDeclarator( sub );

    return ast;
}

bool Parser::parseEnumSpecifier()
{
    //kdDebug(9007) << "Parser::parseEnumSpecifier()" << endl;

    int index = lex->index();

    if( lex->lookAhead(0) != Token_enum ){
        return false;
    }

    lex->nextToken();

    if( lex->lookAhead(0) == Token_identifier ){
        lex->nextToken();
    }

    if( lex->lookAhead(0) != '{' ){
        lex->setIndex( index );
        return false;
    }

    lex->nextToken();

    if( parseEnumeratorList() ){
    }

    if( lex->lookAhead(0) != '}' ){
        reportError( i18n("} expected") );
        skipUntil( '}' );
    }
    lex->nextToken();

    return true;
}

bool Parser::parseTemplateParameterList()
{
    //kdDebug(9007) << "Parser::parseTemplateParameterList()" << endl;

    if( !parseTemplateParameter() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseTemplateParameter() ){
        } else {
            parseError();
            break;
        }
    }

    return true;
}

bool Parser::parseTemplateParameter()
{
    //kdDebug(9007) << "Parser::parseTemplateParameter()" << endl;

    int tk = lex->lookAhead( 0 );
    if( tk == Token_class ||
        tk == Token_typename ||
        tk == Token_template )
        return parseTypeParameter();
    else
        return parseParameterDeclaration();
}

bool Parser::parseTypeParameter()
{
    //kdDebug(9007) << "Parser::parseTypeParameter()" << endl;

    switch( lex->lookAhead(0) ){

    case Token_class:
    {

        lex->nextToken(); // skip class

        // parse optional name
        QString name;
        if( lex->lookAhead(0) == Token_identifier ){
            name = lex->lookAhead( 0 ).toString();
            lex->nextToken();
        }
        if( name )

        if( lex->lookAhead(0) == '=' ){
            lex->nextToken();

            if( !parseTypeId() ){
                parseError();
            }
        }
    }
    return true;

    case Token_typename:
    {

        lex->nextToken(); // skip typename

        // parse optional name
        QString name;
        if( lex->lookAhead(0) == Token_identifier ){
            name = lex->lookAhead( 0 ).toString();
            lex->nextToken();
        }
        if( name )

        if( lex->lookAhead(0) == '=' ){
            lex->nextToken();

            if( !parseTypeId() ){
                parseError();
            }
        }
    }
    return true;

    case Token_template:
    {

        lex->nextToken(); // skip template
        ADVANCE( '<', '<' );
        if( !parseTemplateParameterList() ){
            return false;
        }

        ADVANCE( '>', '>' );
        ADVANCE( Token_class, "class" );

        // parse optional name
        QString name;
        if( lex->lookAhead(0) == Token_identifier ){
            name = lex->lookAhead( 0 ).toString();
            lex->nextToken();
        }
        if( name )

        if( lex->lookAhead(0) == '=' ){
            lex->nextToken();

            QString templ_name = lex->lookAhead( 0 ).toString();
            if( lex->lookAhead(0) != Token_identifier ){
                reportError( i18n("Expected an identifier") );
            } else {
                lex->nextToken(); // skip template-name
            }
        }
    }
    return true;

    } // end switch


    return false;
}

bool Parser::parseStorageClassSpecifier()
{
    //kdDebug(9007) << "Parser::parseStorageClassSpecifier()" << endl;

    switch( lex->lookAhead(0) ){
    case Token_friend:
    case Token_auto:
    case Token_register:
    case Token_static:
    case Token_extern:
    case Token_mutable:
        lex->nextToken();
        return true;
    }

    return false;
}

bool Parser::parseFunctionSpecifier()
{
    //kdDebug(9007) << "Parser::parseFunctionSpecifier()" << endl;

    switch( lex->lookAhead(0) ){
    case Token_inline:
    case Token_virtual:
    case Token_explicit:
        lex->nextToken();
        return true;
    }

    return false;
}

bool Parser::parseTypeId()
{
    //kdDebug(9007) << "Parser::parseTypeId()" << endl;

    if( !parseTypeSpecifier() ){
        return false;
    }

    if( parseAbstractDeclarator() ){
    }

    return true;
}

bool Parser::parseAbstractDeclarator()
{
    //kdDebug(9007) << "Parser::parseAbstractDeclarator()" << endl;

#warning "TODO Parser::parseAbstractDeclarator()"
#warning "resolve ambiguities between sub-abstract-declarator and function argument"

    while( parsePtrOperator() ){
    }

    if( lex->lookAhead(0) == '(' ){
        lex->nextToken();

        DeclaratorAST* declarator = parseDeclarator();;
        if( declarator ){
//             reportError( "expected a nested declarator",
//                          lex->lookAhead(0) );
            return false;
        }
        if( lex->lookAhead(0) != ')'){
            reportError( i18n(") expected") );
            skipUntil( ')' );
        }
        lex->nextToken();
        delete( declarator ); // TODO: remove
    }

    while( lex->lookAhead(0) == '[' ){
        lex->nextToken();
        if( !parseCommaExpression() ){
            reportError( i18n("Expression expected") );
            return false;
        }
        if( lex->lookAhead(0) != ']' ){
            reportError( i18n("] expected") );
            skipUntil( ']' );
            return false;
        }
        lex->nextToken();
    }

    if( lex->lookAhead(0) == '(' ){
        // parse function arguments
        lex->nextToken();

        if( !parseParameterDeclarationClause() ){
            return false;
        }
        if( lex->lookAhead(0) != ')' ){
            reportError( i18n(") expected") );
            skipUntil( ')' );
        }
        lex->nextToken();


        QStringList cv;
        if( parseCvQualify(cv) ){
        }

        if( parseExceptionSpecification() ){
        }
    }

    return true;
}

bool Parser::parseConstantExpression()
{
    //kdDebug(9007) << "Parser::parseConstantExpression()" << endl;

    QStringList l;

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );

        if( tk == '(' ){
            l << "(...)";
            if( !skip('(', ')') ){
                return false;
            }
            lex->nextToken();
        } else if( tk == ',' || tk == ';' || tk == '<'
            || tk == Token_assign || tk == ']' || tk == ')' || tk == '}' ){
            break;
        } else {
            l << lex->lookAhead( 0 ).toString();
            lex->nextToken();
        }
    }

    return !l.isEmpty();
}


DeclaratorListAST* Parser::parseInitDeclaratorList()
{
    //kdDebug(9007) << "Parser::parseInitDeclaratorList()" << endl;

    DeclaratorAST* declarator = parseInitDeclarator();
    if( !declarator ){
        return 0;
    }

    DeclaratorListAST* ast = new DeclaratorListAST();
    ast->addDeclarator( declarator );

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        declarator = parseInitDeclarator();
        if( declarator ){
            ast->addDeclarator( declarator );
        } else {
            parseError();
            break;
        }
    }

    return ast;
}

bool Parser::parseFunctionBody()
{
    //kdDebug(9007) << "Parser::parseFunctionBody()" << endl;

    if( lex->lookAhead(0) != '{' ){
        return false;
    }

    if( !skip('{', '}') ){
        reportError( i18n("missing }") );
    }
    lex->nextToken();

    return true;
}

bool Parser::parseParameterDeclarationClause()
{
    //kdDebug(9007) << "Parser::parseParameterDeclarationClause()" << endl;

    if( parseParameterDeclarationList() ){
    }

    if( lex->lookAhead(0) == ',' ){
        lex->nextToken();
    }

    if( lex->lookAhead(0) == Token_ellipsis ){
        lex->nextToken();
    }

    return true;
}

bool Parser::parseParameterDeclarationList()
{
    //kdDebug(9007) << "Parser::parseParameterDeclarationList()" << endl;

    if( !parseParameterDeclaration() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseParameterDeclaration() ){
        } else {
            break;
        }
    }
    return true;
}

bool Parser::parseParameterDeclaration()
{
    //kdDebug(9007) << "Parser::parseParameterDeclaration()" << endl;

    // parse decl spec
    if( !parseTypeSpecifier() ){
        return false;
    }

    int index = lex->index();
    DeclaratorAST* declarator = parseDeclarator();
    if( declarator ){
        delete( declarator ); // TODO: remove
    } else {
        lex->setIndex( index );
        // optional abstract declarator
        if( parseAbstractDeclarator() ){
        }
    }

    if( lex->lookAhead(0) == '=' ){
        lex->nextToken();
        if( !parseAssignmentExpression() ){
            reportError( i18n("Expression expected") );
        }
    }

    return true;
}

bool Parser::parseClassSpecifier()
{
    //kdDebug(9007) << "Parser::parseClassSpecifier()" << endl;

    int index = lex->index();

    int kind = lex->lookAhead( 0 );
    if( kind == Token_class || kind == Token_struct || kind == Token_union ){
        lex->nextToken();
    } else {
        return false;
    }

    if( lex->lookAhead(0) == Token_identifier ){
        lex->nextToken();
    }

    if( parseBaseClause() ){
    }

    if( lex->lookAhead(0) != '{' ){
        lex->setIndex( index );
        return false;
    }

    ADVANCE( '{', '{' );

    if( lex->lookAhead(0) != '}' ){
        if( parseMemberSpecificationList() ){
        }
    }

    if( lex->lookAhead(0) != '}' ){
        reportError( i18n("} expected") );
        skipUntil( '}' );
    }
    lex->nextToken();

    return true;
}

bool Parser::parseAccessSpecifier()
{
    //kdDebug(9007) << "Parser::parseAccessSpecifier()" << endl;

    switch( lex->lookAhead(0) ){
    case Token_public:
    case Token_protected:
    case Token_private:
        lex->nextToken();
        return true;
    }

    return false;
}

bool Parser::parseMemberSpecificationList()
{
    //kdDebug(9007) << "Parser::parseMemberSpecificationList()" << endl;

    if( !parseMemberSpecification() ){
        return false;
    }

    while( !lex->lookAhead(0).isNull() ){
        if( lex->lookAhead(0) == '}' )
            break;

        if( !parseMemberSpecification() ){
            int tk = lex->lookAhead( 0 );
            skipUntilDeclaration();
            if( lex->lookAhead(0) == tk ){
                lex->nextToken();
            }
        }
    }

    return true;
}

bool Parser::parseMemberSpecification()
{
    //kdDebug(9007) << "Parser::parseMemberSpecification()" << endl;

    if( lex->lookAhead(0) == ';' ){
        lex->nextToken();
        return true;
    } else if( lex->lookAhead(0) == Token_Q_OBJECT || lex->lookAhead(0) == Token_K_DCOP ){
        lex->nextToken();
        return true;
    } else if( lex->lookAhead(0) == Token_signals || lex->lookAhead(0) == Token_k_dcop ){
        lex->nextToken();
        if( lex->lookAhead(0) != ':' ){
            reportError( i18n(": expected") );
        } else
            lex->nextToken();
        return true;
    } else if( parseTypedef() ){
        return true;
    } else if( parseUsing() ){
        return true;
    } else if( parseTemplateDeclaration() ){
        return true;
    } else if( parseAccessSpecifier() ){
        if( lex->lookAhead(0) == Token_slots ){
            lex->nextToken();
        }
        if( lex->lookAhead(0) != ':' ){
            reportError( i18n(": expected") );
            return false;
        } else
            lex->nextToken();
        return true;
    }

    return parseDeclaration();
}

bool Parser::parseCtorInitializer()
{
    //kdDebug(9007) << "Parser::parseCtorInitializer()" << endl;

    if( lex->lookAhead(0) != ':' ){
        return false;
    }
    lex->nextToken();

    if( !parseMemInitializerList() ){
        reportError( i18n("Member initializers expected") );
    }

    return true;
}

bool Parser::parseElaboratedTypeSpecifier()
{
    //kdDebug(9007) << "Parser::parseElaboratedTypeSpecifier()" << endl;

    int index = lex->index();

    int tk = lex->lookAhead( 0 );
    if( tk == Token_class  ||
        tk == Token_struct ||
        tk == Token_union  ||
        tk == Token_enum   ||
        tk == Token_typename )
    {
        lex->nextToken();

        if( parseName() ){
            return true;
        }
    }

    lex->setIndex( index );
    return false;
}

bool Parser::parseDeclaratorId()
{
    //kdDebug(9007) << "Parser::parseDeclaratorId()" << endl;
    return parseName();
}

bool Parser::parseCommaExpression()
{
    //kdDebug(9007) << "Parser::parseCommaExpression()" << endl;

#warning "TODO Parser::parseCommaExpression()"

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );
        if( tk == '(' ){
            if( !skip('(',')') ){
                return false;
            }
        } else if( tk == ']' || tk == ';' ){
            break;
        }
        lex->nextToken();
    }
    return true;
}

bool Parser::parseExceptionSpecification()
{
    //kdDebug(9007) << "Parser::parseExceptionSpecification()" << endl;

    if( lex->lookAhead(0) != Token_throw ){
        return false;
    }
    lex->nextToken();


    ADVANCE( '(', '(' );
    if( !parseTypeIdList() ){
        reportError( i18n("Type id list expected") );
        return false;
    }

    if( lex->lookAhead(0) != ')' ){
        reportError( i18n(") expected") );
        skipUntil( ')' );
    }
    lex->nextToken();

    return true;
}

bool Parser::parseEnumeratorList()
{
    //kdDebug(9007) << "Parser::parseEnumeratorList()" << endl;

    if( !parseEnumerator() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseEnumerator() ){
        } else {
            reportError( i18n("Enumerator expected") );
            break;
        }
    }

    return true;
}

bool Parser::parseEnumerator()
{
    //kdDebug(9007) << "Parser::parseEnumerator()" << endl;

    if( lex->lookAhead(0) != Token_identifier ){
        return false;
    }
    lex->nextToken();

    if( lex->lookAhead(0) == '=' ){
        lex->nextToken();

        if( !parseConstantExpression() ){
            reportError( i18n("Constant expression expected") );
        }
    }

    return true;
}

DeclaratorAST* Parser::parseInitDeclarator()
{
    //kdDebug(9007) << "Parser::parseInitDeclarator()" << endl;

    DeclaratorAST* declarator = parseDeclarator();
    if( declarator ){
        return 0;
    }

    if( parseInitializer() ){
    }

    return declarator;
}

bool Parser::parseAssignmentExpression()
{
    //kdDebug(9007) << "Parser::parseAssignmentExpression()" << endl;

#warning "TODO Parser::parseAssignmentExpression()"

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );

        if( tk == '(' ){
            if( !skip('(', ')') ){
                return false;
            } else
                lex->nextToken();
        } else if( tk == '<' ){
            if( !skip('<', '>') ){
                return false;
            } else
                lex->nextToken();
        } else if( tk == '[' ){
            if( !skip('[', ']') ){
                return false;
            } else
                lex->nextToken();
        } else if( tk == ',' || tk == ';' ||
                   tk == '>' || tk == ']' || tk == ')' ||
                   tk == Token_assign ){
            break;
        } else
            lex->nextToken();
    }

    return true;
}

bool Parser::parseBaseClause()
{
    //kdDebug(9007) << "Parser::parseBaseClause()" << endl;

    if( lex->lookAhead(0) != ':' ){
        return false;
    }
    lex->nextToken();


    if( !parseBaseSpecifierList() ){
        reportError( i18n("expected base specifier list") );
        return false;
    }

    return true;
}

bool Parser::parseInitializer()
{
    //kdDebug(9007) << "Parser::parseInitializer()" << endl;

    if( lex->lookAhead(0) == '=' ){
        lex->nextToken();

        if( !parseInitializerClause() ){
            reportError( i18n("Initializer clause expected") );
            return false;
        }
    } else if( lex->lookAhead(0) == '(' ){
        lex->nextToken();
        if( !parseExpressionList() ){
            reportError( i18n("Expression expected") );
            return false;
        }
        if( lex->lookAhead(0) != ')' ){
            reportError( i18n(") expected") );
            skipUntil( ')' );
        }
        lex->nextToken();
    } else
        return false;

    return true;
}

bool Parser::parseMemInitializerList()
{
    //kdDebug(9007) << "Parser::parseMemInitializerList()" << endl;

    if( !parseMemInitializer() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseMemInitializer() ){
        } else {
            break;
        }
    }

    return true;
}

bool Parser::parseMemInitializer()
{
    //kdDebug(9007) << "Parser::parseMemInitializer()" << endl;

    if( !parseMemInitializerId() ){
        reportError( i18n("Identifier expected") );
        return false;
    }
    ADVANCE( '(', '(' );
    if( !parseExpressionList() ){
        reportError( i18n("Expression expected") );
        return false;
    }

    if( lex->lookAhead(0) != ')' ){
        reportError( i18n(") expected") );
        skipUntil( ')' );
    }
    lex->nextToken();

    return true;
}

bool Parser::parseTypeIdList()
{
    //kdDebug(9007) << "Parser::parseTypeIdList()" << endl;

    if( !parseTypeId() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        if( parseTypeId() ){
        } else {
            reportError( i18n("Type id expected") );
            break;
        }
    }

    return true;
}

bool Parser::parseBaseSpecifierList()
{
    //kdDebug(9007) << "Parser::parseBaseSpecifierList()" << endl;

    if( !parseBaseSpecifier() ){
        return false;
    }

    while( lex->lookAhead(0) == ',' ){
        lex->nextToken();

        if( parseBaseSpecifier() ){
        } else {
            reportError( i18n("Base class specifier expected") );
            break;
        }
    }

    return true;
}

bool Parser::parseBaseSpecifier()
{
    //kdDebug(9007) << "Parser::parseBaseSpecifier()" << endl;

    if( lex->lookAhead(0) == Token_virtual ){
        lex->nextToken();

        if( parseAccessSpecifier() ){
        }
    } else {

        if( parseAccessSpecifier() ){
        }

        if( lex->lookAhead(0) == Token_virtual ){
            lex->nextToken();
        }
    }

    if( lex->lookAhead(0) == Token_scope ){
        lex->nextToken();
    }

    if( !parseName() ){
        reportError( i18n("Identifier expected") );
    }

    return true;
}


bool Parser::parseInitializerClause()
{
    //kdDebug(9007) << "Parser::parseInitializerClause()" << endl;

#warning "TODO Parser::initializer-list"

    if( lex->lookAhead(0) == '{' ){
        if( !skip('{','}') ){
            reportError( i18n("} missing") );
        } else
            lex->nextToken();
    } else {
        if( !parseAssignmentExpression() ){
            reportError( i18n("Expression expected") );
        }
    }

    return true;
}

bool Parser::parseMemInitializerId()
{
    //kdDebug(9007) << "Parser::parseMemInitializerId()" << endl;

    return parseName();
}


bool Parser::parseExpressionList()
{
    //kdDebug(9007) << "Parser::parseExpressionList()" << endl;

    while( !lex->lookAhead(0).isNull() ){
        int tk = lex->lookAhead( 0 );
        if( tk == '(' ){
            if( !skip('(', ')') ){
                reportError( i18n(") missing") );
            } else
                lex->nextToken();
        } else if( tk == ')' ){
            break;
        } else
            lex->nextToken();
    }
    return true;
}

bool Parser::parseNestedNameSpecifier()
{
    //kdDebug(9007) << "Parser::parseNestedNameSpecifier()" << endl;

    if( lex->lookAhead(0) != Token_scope &&
        lex->lookAhead(0) != Token_identifier ){
        return false;
    }

    while( lex->lookAhead(0) == Token_identifier ){

        if( lex->lookAhead(1) == '<' ){
            lex->nextToken(); // skip template name
            lex->nextToken(); // skip <

            if( parseTemplateArgumentList() ){
            }

            if( lex->lookAhead(0) != '>' ){
                reportError( i18n("> expected") );
                skipUntil( '>' );
            }
            lex->nextToken(); // skip >

        } else if( lex->lookAhead(1) == Token_scope ){
            lex->nextToken(); // skip name
            lex->nextToken(); // skip name
        } else
            break;
    }

    return true;
}

bool Parser::parsePtrToMember()
{
    //kdDebug(9007) << "Parser::parsePtrToMember()" << endl;

    if( lex->lookAhead(0) == Token_scope ){
        lex->nextToken();
    }

    while( lex->lookAhead(0) == Token_identifier ){
        lex->nextToken();

        if( lex->lookAhead(0) == Token_scope && lex->lookAhead(1) == '*' ){
            lex->nextToken(); // skip scope
            lex->nextToken(); // skip *
            return true;
        } else
            break;
    }

    return false;
}

bool Parser::parseQualifiedName()
{
    //kdDebug(9007) << "Parser::parseQualifiedName()" << endl;
    parseNestedNameSpecifier();

    if( lex->lookAhead(0) == Token_template ){
        lex->nextToken();
    }

    return parseUnqualiedName();
}

bool Parser::parseUnqualiedName()
{
    //kdDebug(9007) << "Parser::parseUnqualiedName()" << endl;

    bool isDestructor = false;

    if( lex->lookAhead(0) == Token_identifier ){
        lex->nextToken();
    } else if( lex->lookAhead(0) == '~' && lex->lookAhead(1) == Token_identifier ){
        lex->nextToken(); // skip ~
        lex->nextToken(); // skip classname
        isDestructor = true;
    } else if( lex->lookAhead(0) == Token_operator ){
        return parseOperatorFunctionId();
    } else
        return false;

    if( !isDestructor ){

        if( lex->lookAhead(0) == '<' ){
            lex->nextToken();

            // optional template arguments
            if( parseTemplateArgumentList() ){
            }

            if( lex->lookAhead(0) != '>' ){
                reportError( i18n("> expected") );
                skipUntil( '>' );
            }
            lex->nextToken();
        }
    }

    return true;
}
