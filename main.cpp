#include<iostream>
#include<string>
using namespace std;
string Ep;
double result;
char multi = '*';
char plu = '+';
char minu = '-';
char divi = '/';
char invo = '^';
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

double numtable[];

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

void Expression()
{
    cout<< "[In" << i << "]>>> " ; cin>> Ep;
    cout<< "[Out" << i << "]<<< " << Ep <<endl;
}

void computing()
{
    //正则表达式,准备支持多项式计算（数值）
}

void inout()
{
    for(int i =1; n < i; i++)
        {
            Expression();
            computing();
            console();
        }
}

int main()
{
    inout();   
    system("pause");
    return 0;
}