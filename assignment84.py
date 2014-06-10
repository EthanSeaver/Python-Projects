fname = raw_input("Enter file name: ")
if fname == 0:
	fname = 'romeo.txt'
fh = open(fname)
lst = list()
for line in fh:
	print line.rstrip()