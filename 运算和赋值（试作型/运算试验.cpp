#include<iostream>
#include<iomanip>
#include<string>
#include<cmath>
#include<string>
#include<regex>
using namespace std;
main()
{
	string z,var;
	int n;
	cout<< "The expression is "; cin>> z;
	//For the First and Second
	cout<< "How many vars are there?(shoud be an integer) "; cin>> n;
	if (n<=0)
	{
		cout<< "The fuction is " << z;
	}
	else
	{
		for (int i=1; i<=n; i++)
		{
			cout<< "Please input the exact vars(s) "; cin>> var;
			if ()//如果var是数字或不在表达式(expression)中，则报错
		        cout<< "var" << i << " = " << var <<endl;
		}
	}

        regex p ("(\\+)"); //一个浅陋的思路，肯定会很棘手
	/* First 如果是这个思路，必须对常用函数名(sin, cos...)进行标注(mathematica的必须大写首字母是个思路)，避免混淆
	 * Second 同时，如果式子中的运算符号(+, -, *, /...)以函数名的形式出现，也需进行标注，避免混淆
	 */
	/*if (regex_search(z,p))
	{
		cout<< " + " << endl;
	}
	system("pause");*/
	return 0;
}
		

