#!/bin/python
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

n_data = 3
data = []

for i in range(1, n_data + 1):
    string = 'data' + str(i) + '.csv'
    data.append(np.genfromtxt(string, delimiter=','))

# 40000, 80000, 120 000, 160 000, 200 000, 240 000, 280 000
x_axis = data[0][0]
trimmed_data = []
for e in data:
    e = e[1:]
    trimmed_data.append(e)

mean = sum(trimmed_data) / n_data

#  Normal array konstruktion
plt.plot(x_axis, mean[0], label="Array of structs")
#  Normal array single access iteration
#plt.plot(x_axis, mean[1], label="Array of structs")
#  Normal array multiple access iteration
#plt.plot(x_axis, mean[2], label="Array of structs")
#  Realigned array construction
plt.plot(x_axis, mean[3], label="Struct of arrays")
#  Realigned array single access iteration
#plt.plot(x_axis, mean[4], label="Struct of arrays")
#  Realigned array multiple access iteration
#plt.plot(x_axis, mean[5], label="Struct of arrays")
plt.axvline(x=256000, linestyle='--', label='L1 Cache size', color='g')
plt.axvline(x=1024000, linestyle='--', label='L2 Cache size', color='m')
plt.xlabel('Bytes of data iterated upon')
plt.ylabel('ns (less is better)')
plt.legend(loc="upper-left")
plt.show()
