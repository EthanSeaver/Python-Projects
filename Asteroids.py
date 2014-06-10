import pygame
from pygame.locals import *
import math
import random

# globals for user interface
WIDTH = 800
HEIGHT = 600
score = 0
lives = 3
time = 0.5
wrap = 10 #buffer
rockTurn = (random.random() * (.2 - .01) + .01) * random.choice([1, -1])
rockPos_x = random.randrange(wrap, WIDTH - wrap ) #random x location for rock.
rockPos_y = random.randrange(wrap, HEIGHT - wrap ) #random y location for rock.
rockVel_x = (random.random() * (5 - .1) + .1) * random.choice([1, -1])
rockVel_y = (random.random() * (5 - .1) + .1) * random.choice([1, -1])
started = False
missile_group = set([])
rock_group = set([])
collisions = 0
num_collisions = 0
explosion_group = set([])


class ImageInfo:
    def __init__(self, center, size, radius = 0, lifespan = None, animated = False):
        self.center = center
        self.size = size
        self.radius = radius
        if lifespan:
            self.lifespan = lifespan
        else:
            self.lifespan = float('inf')
        self.animated = animated

    def get_center(self):
        return self.center

    def get_size(self):
        return self.size

    def get_radius(self):
        return self.radius

    def get_lifespan(self):
        return self.lifespan

    def get_animated(self):
        return self.animated


# debris images - debris1_brown.png, debris2_brown.png, debris3_brown.png, debris4_brown.png
# debris1_blue.png, debris2_blue.png, debris3_blue.png, debris4_blue.png, debris_blend.png
debris_info = ImageInfo([320, 240], [640, 480])
debris_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/debris2_blue.png")

# nebula images - nebula_brown.png, nebula_blue.png
nebula_info = ImageInfo([400, 300], [800, 600])
nebula_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/nebula_blue.f2013.png")

# splash image
splash_info = ImageInfo([200, 150], [400, 300])
splash_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/splash.png")

# ship image
ship_info = ImageInfo([45, 45], [90, 90], 35)
ship_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/double_ship.png")

# missile image - shot1.png, shot2.png, shot3.png
missile_info = ImageInfo([5,5], [10, 10], 3, 50)
missile_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/shot2.png")

# asteroid images - asteroid_blue.png, asteroid_brown.png, asteroid_blend.png
asteroid_info = ImageInfo([45, 45], [90, 90], 40)
asteroid_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/asteroid_blue.png")

# animated explosion - explosion_orange.png, explosion_blue.png, explosion_blue2.png, explosion_alpha.png
explosion_info = ImageInfo([64, 64], [128, 128], 17, 24, True)
explosion_image = simplegui.load_image("http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/explosion_alpha.png")

# sound assets purchased from sounddogs.com, please do not redistribute
soundtrack = simplegui.load_sound("http://www.soundjay.com/free-music/sounds/iron-man-01.mp3")
missile_sound = simplegui.load_sound("http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/missile.mp3")
missile_sound.set_volume(.3)
ship_thrust_sound = simplegui.load_sound("http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/thrust.mp3")
explosion_sound = simplegui.load_sound("http://www.pacdv.com/sounds/fart-sounds/fart-2.wav")

# helper functions to handle transformations
def angleToVector(ang):
    return [math.cos(ang), math.sin(ang)]

def dist(p, q):
    return math.sqrt((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2)


# Ship class
class Ship:
    def __init__(self, pos, vel, angle, image, info):
        self.pos = [pos[0],pos[1]]
        self.vel = [vel[0],vel[1]]
        self.thrust = False
        self.angle = angle
        self.angle_vel = 0
        self.image = image
        self.image_center = info.get_center()
        self.image_size = info.get_size()
        self.radius = info.get_radius()
        self.aMissile = Sprite(self.pos, self.vel, self.angle, 0, missile_image, missile_info, missile_sound)


    def draw(self,canvas):
        canvas.draw_image(self.image, self.image_center, self.image_size, self.pos, self.image_size, self.angle)

    def set_pos(self, p):
        self.pos = p

    def set_vel(self, v):
        self.vel = v

    def set_angle_vel(self, ang_v):
        self.angle_vel = ang_v

    def update(self):
        self.angle += self.angle_vel

        #position
        self.pos[0] += self.vel[0]
        self.pos[1] += self.vel[1]


        if self.pos[0] > WIDTH + wrap:
            self.pos[0] = wrap

        if self.pos[0] < wrap:
            self.pos[0] = WIDTH + wrap

        if self.pos[1] > HEIGHT + wrap:
            self.pos[1] = wrap

        if self.pos[1] < wrap:
            self.pos[1] = HEIGHT + wrap


        #friction
        frictionCoefficient = 0.02
        acceleration = 0.15

        self.vel[0] *= (1 - frictionCoefficient)
        self.vel[1] *= (1 - frictionCoefficient)


        #thrust
        forward = angleToVector(self.angle)

        if self.thrust:
            self.vel[0] += forward[0] * acceleration
            self.vel[1] += forward[1] * acceleration

    def shoot(self):
        global missile_group

        shipImage_radius = 45
        missleMinVelocity = 8

        shipTipPos_x = self.pos[0] + ( shipImage_radius * math.cos(self.angle) )
        shipTipPos_y = self.pos[1] + ( shipImage_radius * math.sin(self.angle) )


        missileVelocity_x = self.vel[0] + ( missleMinVelocity * math.cos(self.angle) )
        missileVelocity_y = self.vel[1] + ( missleMinVelocity * math.sin(self.angle) )

        aMissile = Sprite([shipTipPos_x, shipTipPos_y], [missileVelocity_x, missileVelocity_y], self.angle, self.angle_vel, missile_image, missile_info, missile_sound)

        missile_group.add(aMissile)

    def get_position(self):
        return self.pos

    def get_radius(self):
        return self.radius

# Sprite class
class Sprite:
    def __init__(self, pos, vel, ang, ang_vel, image, info, sound = soundtrack):
        self.pos = [pos[0],pos[1]]
        self.vel = [vel[0],vel[1]]
        self.angle = ang
        self.angle_vel = ang_vel
        self.image = image
        self.image_center = info.get_center()
        self.image_size = info.get_size()
        self.radius = info.get_radius()
        self.lifespan = info.get_lifespan()
        self.animated = info.get_animated()
        self.age = 0
        if sound:
            sound.play()

    def draw(self, canvas):
        canvas.draw_image(self.image, self.image_center, self.image_size, self.pos, self.image_size, self.angle)

        # asteroid/missile on the canvas
        if self.pos[0] > WIDTH or self.pos[0] < 0:
            self.pos[0] = self.pos[0] % WIDTH
        if self.pos[1] > HEIGHT or self.pos[1] < 0:
            self.pos[1] = self.pos[1] % HEIGHT

        #explosions
        if self.animated:
            frame = self.age % self.lifespan
            center = [(frame * self.image_size[0]) + self.image_center[0], self.image_center[1]]
            canvas.draw_image(self.image, center, self.image_size, self.pos, self.image_size, self.angle)
        else:
            canvas.draw_image(self.image, self.image_center, self.image_size, self.pos, self.image_size, self.angle)


    def update(self):
        self.angle += self.angle_vel
        self.pos[0]+=self.vel[0]
        self.pos[1]+=self.vel[1]
        self.age += 1

        if self.age < self.lifespan:
            return True
        else:
            return False

    def get_position(self):
        return self.pos

    def get_radius(self):
        return self.radius

    def collide(self, other_object):
        other_obj_pos = other_object.pos
        other_obj_rad = other_object.radius
        distance = dist(self.pos, other_obj_pos)
        if distance < self.radius + other_obj_rad:
            collide = True
            return collide
        if distance > self.radius + other_obj_rad:
            collide = False
            return collide

#explosion helper
def explosion_at_position(pos):
    vel = [0, 0]
    avel = 0
    explosion = Sprite(pos, vel, 0, avel, explosion_image, explosion_info, explosion_sound)
    explosion_group.add(explosion)

#rock & ship/rock & missile COLLISIONS
def group_collide(group, other_object):
    global collisions, lives, score
    collisions = 0
    new_set = set(group)
    for obj in new_set:
        if obj.collide(other_object)== True:
            group.remove(obj)
            collisions += 1
            explosion_at_position(obj.get_position())
    return collisions

#rock & missile COLLISIONS
def group_group_collide(group1, group2):
    global num_collisions, score
    num_collisions = 0
    for i in set(group2):
        if group_collide(group1, i) > 0:
            score += 10
            group2.remove(i)
    return num_collisions

def reset_game():
    global lives, score, WIDTH, HEIGHT, started, missile_group, rock_group, explosion_group
    lives = 3
    score = 0
    ship_thrust_sound.rewind()
    soundtrack.rewind()
    soundtrack.play()
    missile_group = set([])
    rock_group = set([])
    explosion_group = set([])


def keydown(key):
    global missile_group
    acc_speed = .1

    if True:

    #moves ship
        if key == simplegui.KEY_MAP["up"]:
            myShip.image_center[0] += 90
            myShip.thrust = True
            ship_thrust_sound.play()
        elif key==simplegui.KEY_MAP["left"]:
            myShip.angle_vel -= acc_speed
        elif key==simplegui.KEY_MAP["right"]:
            myShip.angle_vel += acc_speed

        if key==simplegui.KEY_MAP["space"]:
            myShip.shoot()

def keyup(key):
    global missile_group, fire, started

    if True:
        if key == simplegui.KEY_MAP["up"]:
            myShip.image_center[0] -= 90
            myShip.thrust = False
            ship_thrust_sound.pause()

        if key==simplegui.KEY_MAP["left"]:
            myShip.angle_vel = 0

        if key==simplegui.KEY_MAP["right"]:
            myShip.angle_vel = 0

# mouseclick handlers
def click(pos):
    global started
    center = [WIDTH / 2, HEIGHT / 2]
    size = splash_info.get_size()
    inWIDTH = (center[0] - size[0] / 2) < pos[0] < (center[0] + size[0] / 2)
    inHEIGHT = (center[1] - size[1] / 2) < pos[1] < (center[1] + size[1] / 2)
    if (not started) and inWIDTH and inHEIGHT:
        started = True
        reset_game()

#draws rock_group & updates
def process_sprite_group(canvas, rock_group, missile_group, explosion_group):
    for rock in rock_group:
        rock.draw(canvas)
        rock.update()
    for aMissile in missile_group:
        aMissile.draw(canvas)
        ans = aMissile.update()
        if ans == False:
            missile_group.remove(aMissile)
    for explosion in explosion_group:
        explosion.draw(canvas)
        ans2 = explosion.update()
        if ans2 == False:
            explosion_group.remove(explosion)

def draw(canvas):
    global time, lives, score, started, collisions, WIDTH, HEIGHT, rock_group

    # animiate background
    time += 1
    center = debris_info.get_center()
    size = debris_info.get_size()
    wtime = (time / 8) % center[0]
    canvas.draw_image(nebula_image, nebula_info.get_center(), nebula_info.get_size(), [WIDTH / 2, HEIGHT / 2], [WIDTH, HEIGHT])
    canvas.draw_image(debris_image, [center[0]-wtime, center[1]], [size[0]-2*wtime, size[1]],
                                [WIDTH /2 + 1.25 * wtime, HEIGHT / 2], [WIDTH - 2.5 * wtime, HEIGHT])

    #score & lives
    canvas.draw_text(str("Lives"), [50, 50], 20, 'Orange')
    canvas.draw_text(str(lives), [65, 85], 30, 'Yellow')
    canvas.draw_text(str("Score"), [680, 50], 20, 'Orange')
    canvas.draw_text(str(score), [695, 85], 30, 'Yellow')

    # draw ship, rocks, and missiles sprites & checks collisions
    myShip.draw(canvas)
    myShip.update()

    if started:
        timer.start()
        process_sprite_group(canvas, rock_group, missile_group, explosion_group)
        if group_collide(rock_group, myShip):
            lives -= 1
            if lives <= 0:
                started = False
                rock_group = set([])
                timer.stop()

        group_group_collide(rock_group, missile_group)


    if not started:
        canvas.draw_image(splash_image, splash_info.get_center(),
                          splash_info.get_size(), [WIDTH / 2, HEIGHT / 2],
                          splash_info.get_size())


# timer handler spawns a rock with random position, velocity, and angular velocity
def rock_spawner():
    global rock_group, WIDTH, HEIGHT

    rockTurn = (random.random() * (0.1 - 0.01) + 0.01) *random.choice([1, -1])
    rockPos_x = (myShip.pos[0] + random.randrange(myShip.radius * 3 , WIDTH - myShip.radius * 2) ) % WIDTH
    rockPos_y = (myShip.pos[1] + random.randrange(myShip.radius * 3 , HEIGHT - myShip.radius * 2) ) % HEIGHT
    rockVel_x = (random.random() * (.5 - .1) + .1) *random.choice([1, -1])
    rockVel_y = (random.random() * (.5 - .1) + .1) *random.choice([1, -1])

    aRock = Sprite([rockPos_x, rockPos_y], [rockVel_x, rockVel_y], 0, rockTurn, asteroid_image, asteroid_info)

    if len(rock_group) <= 11:
        rock_group.add(aRock)

# initialize frame
frame = simplegui.create_frame("ASTEROIDS", WIDTH, HEIGHT)

# initialize ship and two sprites
myShip = Ship([WIDTH / 2, HEIGHT / 2], [0, 0], 1.5 * math.pi, ship_image, ship_info)

# register handlers
frame.set_draw_handler(draw)
frame.set_keydown_handler(keydown)
frame.set_keyup_handler(keyup)
frame.set_mouseclick_handler(click)
timer = simplegui.create_timer(1000.0, rock_spawner)

# get things rolling
timer.start()
frame.start()
