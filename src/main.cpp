#include<iostream>
#include<string>
#include<regex>
using namespace std;
string Ep;
double result = 90;
int n = 0;
int i = 1;

void console()
{
    char p;
    cout<< "quit? "; cin>> p;
            switch (p)
            {
                case 'N':
                n = 0;
                break;
                case 'Y':
                n = i + 10;
                break;
                default:
                cerr<< "wrong! ";
            }
}

double numtable[10];

    double multiply()
    {
        int m;
        return result;
    }

    double plus()
    {
        int m;
        return result;
    }

    double minus()
    {
        int m;
        return result;
    }
    double division()
    {
        int m;
        return result;
    }
    double involution()
    {
        int m;
        return result;
    }

void computing()
{
    //正则表达式,准备支持多项式计算（数值）
}

void inout()
{
    for(int i =1; n < i; i++)
        {
            cout<< "[In" << i << "]>>> " ; cin>> Ep;
            computing();
            cout<< "[Out" << i << "]<<< " << result <<endl;
            console();
        }
}

int main()
{
    inout();   
    system("pause");
    return 0;
}