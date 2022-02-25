from sys import argv
from sympy import isprime

a = int(argv[1])
b = int(argv[2])

with open('py_prime_out.txt', 'w') as f:
	for i in range(a, b):
		f.write(str(int(isprime(i))) + '\n')
