#include "math.h"

// coded by Danoni631 for graphical programs when Nikinix become popular

static unsigned xsint = 42;

unsigned long xor_seed(unsigned long seed)
{
    xs = seed;
    return 0;
}

int xor_shift(void)
{
    xsint = xsint * 1103515245 + 12345;
    return (xsint / 65536) % RANDMAX;
}

int dword_xor_shift(void)
{
    xs ^= xs << 13;
  	xs ^= xs >> 17;
  	xs ^= xs << 5;
  	return xs;
}

double sine(double x)
{
    double sum = 0.0;

    double a  = 1.0;
    double b  = 1.0;
    double expx = x;

    for (int i = 1; i <= SUMLIM; i++)
    {
        sum += (a / b) * expx;
        
        expx *= x * x;

        a *= -1.0;
        b *= 4 * i * i + 2 * i;
    }

    return sum;
}

double cosine(double x)
{
    double sum = 0.0;

    double a  = 1.0;
    double b  = 1.0;
    double expx = 1;

    for (int i = 1; i <= SUMLIM; i++)
    {
        sum += (a / b) * expx;
        
        expx *= x * x;

        a *= -1.0;
        b *= 4 * i * i - 2 * i;
    }

    return sum;
}

double square_root(double x)
{
    const int MaxIterations = 30;
    const double InitialApproximation = 1.0;

    double approximation = InitialApproximation;

    for (int i = 0; i <  MaxIterations; ++i) 
    {
        approximation = 0.5 * (approximation + x / approximation);
    }

    return approximation;
}

double exp2(double x)
{
    double result = 1.0;
    double power_of_2 = 2.0;

    if (x == 0.0) 
    {
        return 1.0;
    }
    
    else if (x > 0.0) 
    {
        while (x >= 1.0) 
        {
            result *= power_of_2;
            x -= 1.0;
        }
    }
    
    else 
    {
        while (x <= -1.0) 
        {
            result /= power_of_2;
            x += 1.0;
        }
    }

    double frac = 1.0;
    double term = 1.0;
    double i = 1.0;

    while (i <= 32.0) 
    {
        term *= (x / i);
        frac += term;
        i += 1.0;
    }

    return result * frac;
}

float float_abs(double x)
{
    return (x < 0) ? -x : x;
}

int sign(int x)
{
    if (x < 0)
    {
        return -1;
    }

    return 1;
}

int floor(double x)
{
    if (x >= 0) 
    {
        return (int)x;
    }
      
    else 
    {
        int intpart = (int)x;
        return (x == intpart) ? intpart : intpart - 1;
    }
}

double inverted_tangent(double x)
{
    const int maxIter = 50;
    const double epsilon = 1e-10;

    if (x > 1 || x < -1) 
    {
        if (x > 0) 
        {
            return PI / 2 - atan(1 / x);
        } 
        else 
        {
            return -PI / 2 - atan(1 / x);
        }
    }

    double result = 0.0;
    double term = x;
    int n = 1;

    while (abs(term) > epsilon && n <= maxIter) 
    {
        result += term;
        term *= -x * x * (2 * n - 1) / (2 * n + 1);
        n++;
    }

    return result;
}

double inverted_tangent2(double y, double x)
{
    if (x == 0) 
    {
        if (y > 0) return PI / 2;
        if (y < 0) return -PI / 2;
        return 0;
    }

    double atanResult = atan(y / x);

    if (x > 0) 
    {
        return atanResult;
    } 
    else 
    {
        if (y >= 0) 
        {
            return atanResult + PI;
        }
        else 
        {
            return atanResult - PI;
        }
    }
}

double pow(double base, int exp)
{
    if (exp == 0) 
    {
        return 1.0;
    }
    
    if (exp < 0) 
    {
        return 1.0 / pow(base, -exp);
    }

    double result = 1.0;

    while (exp > 0) 
    {
        if (exp % 2 == 1) result *= base;
        base *= base;
        exp /= 2;
    }
    
    return result;
}
