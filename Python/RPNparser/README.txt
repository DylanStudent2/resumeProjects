 RPN parser for tiny code. Tiny contains simple mathematical statements, a simple if nonzero case, and
while loop, along with cases for printing newline, a variable, and tab. Also performs exponentials, modulus, and truncation.
Every statement is ended with ";"

Lets look at tiny1.txt:

n = 1; s = 0;
m = 5*4;
{ n - m ?
   s = s + 1/(n*n);
   < s; < N;
   n = n + 1;
} $

This is initializing variables, while the "{ n -m ?" statement is a while loop statement to see if (n - m) == 0.
If it is, it will  do the following statements, while < s; < N is printing out the variable s and a "N" for newline. 
While loops are enclosed in "{ }" brackets. 

Here's tiny2.txt:

> m;
i = m - 1;
> n;
{ m ? < m; m = m - 1; }

< N;
x = 2*5;
a = x;
{ i ?
   [ n@a ? d = d; : < B; ]
   a = a*x;
   i = i - 1;
}
< n; < N; 
$

"> m" is taking input from the user to define m, whereas the "[ n@a ? d = d; : < B; ]" statement is 
an if statement that does "if floor(n/a) isn't zero, d = d, else print out blank space. 
