#include<iostream>
#include<iomanip>
#include<string>
#include<cmath>
#include<string>
using namespace std;
main()
{
        float x,z;
	string y;
	cout<< "The var is ";
	cin>> y;
	/*So you can set any var you want but now you can only set just one var in a time, which is really useless*/
	/*What I want to do now is to connect x with y so that I can give a value to x, and meanwhile I can set y as its name*/
       	cout<< "var = " << y << endl;
	cout<< "Press 0 to give " << y << " a value or to press a number except 0 to ignore ";
	cin>> z;
	if(z==0)
	{
		cout<< "Value = "; cin>> x;
	        cout<< y << " = " << x << endl;
	}
	else
	{
		cout<< "The expression is " << y << endl;
	}	
        system("pause");
	return 0;
}

	
