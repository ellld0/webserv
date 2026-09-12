#include "../../includes/utils/Utils.hpp"
#include <climits>

int ft_atoi(const char* value)
{
    if (!value)
        return 0;

    int sign = 1;
    int result = 0;
    const char* current = value;

    while (*current == ' ' || *current == '\t' || *current == '\n'
        || *current == '\r' || *current == '\f' || *current == '\v')
        ++current;

    if (*current == '-' || *current == '+')
    {
        if (*current == '-')
            sign = -1;
        ++current;
    }

    while (*current >= '0' && *current <= '9')
    {
        int digit = *current - '0';
        if (result > (INT_MAX - digit) / 10)
            return sign == 1 ? INT_MAX : INT_MIN;
        result = result * 10 + digit;
        ++current;
    }

    return result * sign;
}

bool ft_isdigit(char value)
{
    return value >= '0' && value <= '9';
}

