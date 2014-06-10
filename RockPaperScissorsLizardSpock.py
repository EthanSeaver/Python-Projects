# Rock-paper-scissors-lizard-Spock template


# The key idea of this program is to equate the strings
# "rock", "paper", "scissors", "lizard", "Spock" to numbers
# as follows:
#
# 0 - rock
# 1 - Spock
# 2 - paper
# 3 - lizard
# 4 - scissors

print 'Welcome to the game,'

print 'Please select from the following:'

print 'Rock' , 'Paper' , 'Scissors' , 'Lizard' , 'Spock'


import random

# helper functions

def number_to_name(number):
    if number == 0:
        name = 'rock'
    elif number == 1:
        name = 'spock'
    elif number == 2:
        name = 'paper'
    elif number == 3:
        name = 'lizard'
    elif number == 4:
        name = 'scissors'
    else:
        print "Inserted wrong number.  Please re-enter again."
    return name


def name_to_number(name):
    """converts the string name into a number between 0 and 4"""
    if name == 'rock':
        number = 0
    elif name == 'spock':
        number = 1
    elif name == 'paper':
        number = 2
    elif name == 'lizard':
        number = 3
    elif name == 'scissors':
        number = 4
    else:
        number = 0
    return number


def rpsls(player_name):
    # convert name to player_number using name_to_number
    player_number = name_to_number(player_name)
    #print "player_number:", player_number
    # compute random guess for comp_number using random.randrange()
    comp_number = random.randrange(0, 5)
    #print "comp_number:", comp_number
    # compute difference of player_number and comp_number modulo five
    diff = (player_number - comp_number) % 5
    #print "diff:", diff
    # use if/elif/else to determine winner
    # each choice wins agains the preceding two choices and loses against
    # the following two choices
    result = ''
    if diff == 0:
        result = 'Player and computer tie!'
    elif diff == 1:
        result = 'Computer wins!'
    elif diff == 2:
        result = 'Computer wins!'
    elif diff == 3:
        result = 'Player wins!'
    elif diff == 4:
        result = 'Player wins!'
    # convert comp_number to name using number_to_name
    comp_name = number_to_name(comp_number)
    # print results
    print "Player chooses", player_name
    print "Computer chooses", comp_name
    print result
    print '\n'


# test your code
rpsls("rock")
rpsls("spock")
rpsls("paper")
rpsls("lizard")
rpsls("scissors")

# always remember to check your completed program against the grading rubric


