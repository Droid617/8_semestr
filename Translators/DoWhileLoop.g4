grammar DoWhileLoop;

program
    : DO '{' statement '}' WHILE '(' condition ')' ';'
    | DO ' ' statement WHILE '(' condition ')' ';'
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
