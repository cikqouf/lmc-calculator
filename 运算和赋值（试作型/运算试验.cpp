#include<iostream>
#include<iomanip>
#include<string>
#include<cmath>
#include<string>
#include<regex>
using namespace std;
main()
{
	string z;
	cout<< "the expression is "; cin>> z;
	regex p ("(\\+)"); //一个浅陋的思路，肯定会很棘手
	if (regex_search(z,p))
	{
		cout<< " + ";
	}
	system("pause");
	return 0;
}
		

