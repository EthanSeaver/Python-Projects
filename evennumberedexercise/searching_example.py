# Sample Python/Pygame Programs
# Simpson College Computer Science
# http://programarcadegames.com/
# http://simpson.edu/computer-science/
# Read in a file from disk and put it in an array.
file = open("super_villains.txt")
name_list = []
for line in file:
line = line.strip()
name_list.append(line)
file.close()
# Linear search
i = 0
while i < len(name_list) and name_list[i] != "Morgiana the Shrew":
i += 1
if i == len(name_list):
print( "The name was not in the list." )
else:
print( "The name is at position",i)
# Binary search
desired_element = "Morgiana the Shrew";
lower_bound = 0
upper_bound = len(name_list)-1
found = False
while lower_bound <= upper_bound and found == False:
middle_pos = (lower_bound + upper_bound) // 2
if name_list[middle_pos] < desired_element:
lower_bound = middle_pos + 1
elif name_list[middle_pos] > desired_element:
upper_bound = middle_pos - 1
else:
found = True
if found:
print( "The name is at position", middle_pos)
else:
print( "The name was not in the list." )