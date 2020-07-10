#include "lmc-math.h"
#define FIRST_VALUE (exp)  ( (MathConst)(exp -> inside) -> value)
#define SECOND_VALUE (exp) ( (MathConst)((exp + 1) -> inside) -> value)


static MathConst* math_exp_plus      (MathExp* math_exp);
static MathConst* math_exp_minus     (MathExp* math_exp);
static MathConst* math_exp_multiply  (MathExp* math_exp);
static MathConst* math_exp_divide    (MathExp* math_exp);
/* FIXME 
 * Here should be a function which give value(const) to variate.
 */

static MathConst*
math_exp_plus (MathExp* math_exp);
{
	MathConst* math_const_p;
	
	math_const_p -> value = FIRST_VALUE (math_exp) + \
		                SECOND_VALUE (math_exp);
	return math_const_p;
}

static MathConst*
math_exp_minus (MathExp* math_exp);
{
	MathConst* math_const_p;

	math_const_p -> value = FIRST_VALUE (math_exp) - \
				SECOND_VALUE (math_exp);
	return math_const_p;
}


