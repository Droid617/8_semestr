grammar DoWhileLoop;

options {
    language = Cpp;
}

@header {
#include "common.h"
#include "DoWhileLoopExecutor.h"
}

program
    : DO '{' statement '}' WHILE '(' condition ')' ';'
    {
    	auto conditionCtx = _localctx->condition();
    	
        while (DoWhileLoopExecutor::checkCondition(conditionCtx)) 
	{
            DoWhileLoopExecutor::executeStatement(_localctx->statement(), conditionCtx);
        }
    }
    | DO ' ' statement WHILE '(' condition ')' ';'
    {
        auto conditionCtx = _localctx->condition();
    	
        while (DoWhileLoopExecutor::checkCondition(conditionCtx)) 
	{
            DoWhileLoopExecutor::executeStatement(_localctx->statement(), conditionCtx);
        }
    }
    ;

statement
    : program
    | expression
    ;

condition
	: increment VAR '<' INT
	;

expression
	: PRINT '(' increment VAR ')' ';'
	{
	    std::string varName = $VAR.text;
	    int value = variables[varName];
	    if ($increment.ctx->getText() == "++") 
	    {
	        value++;
	        variables[varName] = value;
	    }
	    std::cout << value << std::endl;
	}
	;

increment
	: '++'
	|  
	;

DO : 'do' ;
WHILE : 'while' ;
PRINT : 'print' ;
SPACE : ' ' ;
SEMI : ';' ;
LPAREN : '(' ;
RPAREN : ')' ;
LCURLY : '{' ;
RCURLY : '}' ;
LOWERTHEN : '<' ;

INT : [0-9]+ ;
VAR : [_a-zA-Z][_a-zA-Z0-9]* ;
WS: [\t\n\r\f]+ -> skip ;
