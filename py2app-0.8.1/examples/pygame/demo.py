import pygame, random, math

#game settings
width =1024
height = 800
color_black = 0,0,0
color_green = 166,206,57
score = 10

#Initialization
pygame.init()
screen = pygame.display.set_mode ((width, height))
# initialize font; must be called after 'pygame.init()' to avoid 'Font not Initialized' error
#Label 1
myfont = pygame.font.SysFont("monospace", 35)
label = myfont.render("Label1", 1, color_green)
#Label 2
yourFont = pygame.font.SysFont(None, 64)
label2 = yourFont.render("Label2", 1, color_green)#Label 3
hisFont = pygame.font.Font(None, 64)
label3 = hisFont.render("Label3", 1, color_green)

#draw
def draw():
    screen.fill(color_black)
    screen.blit(label, (200, 100))
    screen.blit(label2, (200, 300))
    screen.blit(label3, (200, 500))
    pygame.display.flip()

done=False
while not done:
    events = pygame.event.get()
    for event in events:
        if event.type == pygame.QUIT:
            done = True
        elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
            done = True
    draw()

pygame.quit()
