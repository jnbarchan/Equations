#include <cmath>
#include <limits.h>

#include "mathexpreval.h"

MathExprEval::MathExprEval() {}

int MathExprEval::precedence(char op) const
{
    switch (op)
    {
    case '+':  // add
    case '-':  // subtract
        return 1;
    case '*':  // multiply
    case '/':  // divide
        return 2;
    case '^':  // exponent
    case '~':  // root
        return 3;
    default:
        return INT_MIN;
    }
}

bool MathExprEval::needsParentheses(char childOp, char parentOp, bool isRightChild) const
{
    int childPrecedence = precedence(childOp);
    int parentPrecedence = precedence(parentOp);

    if (childOp == '\0')
        return false;

    if (childPrecedence < parentPrecedence)
        return true;

    if (childPrecedence > parentPrecedence)
        return false;

    // Equal precedence
    if (!isRightChild)
        return false;

    // Equal precedence, right child

    if (parentOp == '-' || parentOp == '/')
        return true;

    if (parentOp == '*' && childOp == '/')
        return true;

    if (parentOp == '^' && childOp == '~')
        return true;

    if (parentOp == '~')
        return true;

    return false;
}

int MathExprEval::applyOperator(int a, int b, char op) const
{
    if (a == INT_MIN || b == INT_MIN)
        return INT_MIN;
    switch (op)
    {
    case '+':  // add
        return a + b;
    case '-':  // subtract
        return a - b;
    case '*':  // multiply
        return a * b;
    case '/':  // divide
        if (b == 0)
            return INT_MIN;
        return a / b;
    case '^':  // exponent
        return pow(a, b);
    case '~':  // root
        if (a == 0)
            return INT_MIN;
        return pow(b, 1.0 / a);
    default:
        return INT_MIN;
    }
}

void MathExprEval::reduceStacks(std::stack<int> &values, std::stack<char> &operators)
{
    char op = operators.top();
    operators.pop();
    int b = values.top();
    values.pop();
    int a = values.top();
    values.pop();

    values.push(applyOperator(a, b, op));
}

int MathExprEval::evaluateExpression(const std::string &expression)
{
    std::stack<int> values;
    std::stack<char> operators;

    int i = 0;
    while (i < expression.length())
    {
        char c = expression[i];

        if (c == '(')
        {
            operators.push(c);
        }
        else if (isdigit(c))
        {
            int num = 0;
            while (i < expression.length() && isdigit(expression[i]))
            {
                num = num * 10 + (expression[i] - '0');
                i++;
            }
            i--;

            values.push(num);
        }
        else if (c == ')')
        {
            while (operators.top() != '(')
                reduceStacks(values, operators);

            operators.pop();  // Discard the opening parenthesis
        }
        else
        {
            while (!operators.empty() && precedence(operators.top()) >= precedence(c))
                reduceStacks(values, operators);

            operators.push(c);
        }

        i++;
    }

    while (!operators.empty())
        reduceStacks(values, operators);

    return values.top();
}

int MathExprEval::evaluateExpression(const std::vector<int> &valuesIn, const std::vector<char> &operatorsIn)
{
    std::stack<int> values;
    std::stack<char> operators;

    int i = 0;
    values.push(valuesIn[i]);
    while (++i < valuesIn.size())
    {
        char op = operatorsIn[i - 1];
        int val = valuesIn[i];

        while (!operators.empty() && precedence(operators.top()) >= precedence(op))
            reduceStacks(values, operators);

        operators.push(op);
        values.push(val);
    }

    while (!operators.empty())
        reduceStacks(values, operators);

    return values.top();
}

MathExprEval::EvalResults MathExprEval::evaluateAllParenthesisations(const std::vector<int> &values, const std::vector<char> &operators)
{
    int count = static_cast<int>(values.size());
    memo.cache = std::vector<std::vector<EvalResults>>(count, std::vector<EvalResults>(count));
    memo.calculated = std::vector<std::vector<bool>>(count, std::vector<bool>(count, false));

    evaluateRangeParenthesisations(values, operators, 0, count - 1);

    return std::move(memo.cache[0][count - 1]);
}

MathExprEval::EvalResults &MathExprEval::evaluateRangeParenthesisations(
    const std::vector<int> &values, const std::vector<char> &operators,
    int first, int last)
{
    EvalResults &results = memo.cache[first][last];

    if (memo.calculated[first][last])
        return results;

    if (first == last)
    {
        EvalResult result;
        result.value = values[first];
        result.topLevelOp = '\0';
        result.expression = std::to_string(result.value);
        results.push_back(result);
    }
    else
    {
        for (int i = first; i < last; ++i)
        {
            EvalResults &leftResults = evaluateRangeParenthesisations(values, operators, first, i);

            EvalResults &rightResults = evaluateRangeParenthesisations(values, operators, i + 1, last);

            for (EvalResult &left : leftResults)
            {
                for (EvalResult &right : rightResults)
                {
                    int value = applyOperator(left.value, right.value, operators[i]);
                    if (value != INT_MIN)
                    {
                        EvalResult result;
                        result.value = value;
                        result.topLevelOp = operators[i];
                        bool leftParen = needsParentheses(left.topLevelOp, operators[i], false);
                        bool rightParen = needsParentheses(right.topLevelOp, operators[i], true);

                        std::size_t maxLength = 2 + left.expression.size() + 2 + right.expression.size() + 1;
                        result.expression.reserve(maxLength);
                        if (leftParen)
                            result.expression += '(';
                        result.expression += left.expression;
                        if (leftParen)
                            result.expression += ')';
                        result.expression += operators[i];
                        if (rightParen)
                            result.expression += '(';
                        result.expression += right.expression;
                        if (rightParen)
                            result.expression += ')';

                        results.push_back(result);
                    }
                }
            }
        }
    }

    memo.calculated[first][last] = true;
    return results;
}
