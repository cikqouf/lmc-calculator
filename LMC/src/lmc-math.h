#include <math.h>

struct _MathExp
{
	MathSymbol symbol;
	void* inside;
}

typedef MathExp struct _MathExp;

MathExp* math_exp_calculate(MathExp math_exp,MathKvlist math_kvlist);  

