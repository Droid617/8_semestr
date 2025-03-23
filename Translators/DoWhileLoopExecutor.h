#ifndef DOWHILELOOPEXECUTOR_H
#define DOWHILELOOPEXECUTOR_H

#include "DoWhileLoopParser.h"
#include "common.h"
#include <iostream>

class DoWhileLoopExecutor 
{
public:
    static bool checkCondition(DoWhileLoopParser::ConditionContext* conditionCtx) 
    {
        std::string varName = conditionCtx->VAR()->getText();
        int value = variables[varName];
        int limit = std::stoi(conditionCtx->INT()->getText());

        if (conditionCtx->increment() != nullptr && conditionCtx->increment()->getText() == "++") 
        {
            value++;
            variables[varName] = value;
        }

        bool conditionResult = value < limit;
        
        return conditionResult;
    }
    
    static bool executeStatement(DoWhileLoopParser::StatementContext* statementCtx, DoWhileLoopParser::ConditionContext* conditionCtx) 
    {
        if (statementCtx->expression() != nullptr) 
        {
            auto expressionCtx = statementCtx->expression();

            if (expressionCtx->PRINT() != nullptr) 
            {
                std::string varName = expressionCtx->VAR()->getText();
                int value = variables[varName];

                if (expressionCtx->increment() != nullptr && expressionCtx->increment()->getText() == "++") 
                {
                    value++;
                    variables[varName] = value;
                }

                std::cout << value << std::endl;
            }
        } 
        else if (statementCtx->program() != nullptr) 
        {
            auto programCtx = statementCtx->program();

            while (checkCondition(conditionCtx)) 
            {
                executeStatement(programCtx->statement(), conditionCtx);
            }
            
        }

        return true;
    }
    
};

#endif // DOWHILELOOPEXECUTOR_H
