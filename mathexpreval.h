#ifndef MATHEXPREVAL_H
#define MATHEXPREVAL_H

#include <stack>
#include <string>
#include <vector>

class MathExprEval
{
public:
    MathExprEval();

    struct EvalResult
    {
        int value;
        char topLevelOp;
        std::string expression;
    };
    using EvalResults = std::vector<EvalResult>;

    int evaluateExpression(const std::string &expression);
    int evaluateExpression(const std::vector<int> &valuesIn, const std::vector<char> &operatorsIn);
    EvalResults evaluateAllParenthesisations(const std::vector<int> &values, const std::vector<char> &operators);

private:
    int precedence(char op) const;
    bool needsParentheses(char childOp, char parentOp, bool isRightChild) const;
    int applyOperator(int a, int b, char op) const;
    void reduceStacks(std::stack<int> &values, std::stack<char> &operators);
    EvalResults &evaluateRangeParenthesisations(const std::vector<int> &values, const std::vector<char> &operators, int first, int last);

    struct
    {
        std::vector<std::vector<EvalResults>> cache;
        std::vector<std::vector<bool>> calculated;
    } memo;
};

#endif // MATHEXPREVAL_H
