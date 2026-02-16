#include <stdio.h>
#include <stdbool.h>


/*   1
Variable = A reusable container for a value.
Behaves as if it were the valu it contains

int = whole numbers (4 bytes in modern sytems)
float = single-precision decimal number (4 bytes)
double = double-precision decimal number (8 bytes)
char = single character single quotes (1 byte)
char[] = array of characters double quotes (size varies)
bool = true or false 1 or 0 (1 byte, requires <stdbool.h>)
*/


/*   2
Format Specifier =
Special tokens that begin with a % symbol, followed
by a character that specifies the data type and optional
modifiers (width, precision, flags)
They control how data is displayed or interprted.
*/

int main(){

  int age = 35;
  int num1 = 1;
  int num2 = 10;
  int num3 = 100;
  float price = 21.99;
  double pi = 3.1415926535;
  char currency = '$';
  char name[] = "Ryan Laleh";

  printf("%3d\n", num1);
  printf("%3d\n", num2);
  printf("%3d\n", num3);

  printf("%d\n", age);
  printf("%f\n", price);
  printf("%lf\n", pi);
  printf("%c\n", currency);
  printf("%s\n", name);

  return 0;
}
