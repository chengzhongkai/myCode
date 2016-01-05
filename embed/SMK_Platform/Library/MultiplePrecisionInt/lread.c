/*		lread.c		*/
/*    original     made by Tomy                              */
/*                 revised by Mr.Eiichi Shimada   2001/12/28 */
#include "longint.h"

/* setting LINT by string */

static char *my_strrev(char *str)
{
    char *p1, *p2, tmp;
    if (str == NULL || *str == '\0') return (str);
    
    p1 = str;
    p2 = str + strlen(str) - 1;
    while (p2 > p1) {
        tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1 ++;
        p2 --;
    }
    return (str);
}

LINT lread(char *s)
{
	char *string;
	int i,n,j,k,len;
	LINT a;
	a.num[0]=0;
	a.sign=0;
	k=0;
	len=strlen(s);
	if(len==0)
	{
		a.num[1]=0;
		a.len=0;
		a.sign=0;
		return a;
	}
	if(s[0]=='-')
		{
			a.sign=-1;
			k++;
		}
	if(s[0]=='+')
		{
		k++;
		}
	string= strdup(&s[k]);// 符号を除いた部分をコピーする。
	string=my_strrev(string);//コピーしたものを逆順にする　12345→45231
	len=len-k;	    //符号を除いた字数を計算
	
	for(i=0;i<len;i++)
	{
		if(isdigit(string[i])){
		string[i]-='0';
		}
		else{
			break;//  数字でないところで打ちきる。
		}
	
	}
	len=i;
	
	k=1;
	//54321の始めから２数字ずつ二桁の値に直す。
	for(i=0;i<len;i+=2)
	{
		a.num[k]=string[i]+string[i+1]*10;
		k++;
	}
	a.len=k-1;
	return a;
}
