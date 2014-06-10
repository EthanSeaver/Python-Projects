# Given an int n, 
# return the absolute difference between n and 21, 
# except return double the absolute difference if n is over 21. 
def diff21(n):
    if n > 21:
        n = (n - 21) * 2
      print n
    