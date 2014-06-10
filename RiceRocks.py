#!/usr/bin/env python
# -*- coding: latin-1 -*-

"""
RiceRocks (Asteroids) (May 25, 2014)

My retouched solution of the mini-project #8 of the course
https://www.coursera.org/course/interactivepython (Coursera 2013).

Run on (maybe very slow on some browsers):
  - Chrome 31
  - Firefox 26
  - Safari 5.1.7 (without sounds)
  - Python 2 and 3 with SimpleGUICS2Pygame.

Fix me:
  - Collision problem when divide a big asteroid in two little asteroids.

Piece of SimpleGUICS2Pygame.
https://bitbucket.org/OPiMedia/simpleguics2pygame

GPLv3 --- Copyright (C) 2013, 2014 Olivier Pirson
http://www.opimedia.be/
"""

import math
import random

try:
    from user27_5LlszPPJxQHFMbk import assert_position
    from user33_Bhc7VzXKbXGVQV1 import FPS
    from user33_PJRhxILEqeTIZKf import Loader

    import simplegui

    SIMPLEGUICS2PYGAME = False
except ImportError:
    from SimpleGUICS2Pygame.codeskulptor_lib import assert_position
    from SimpleGUICS2Pygame.simplegui_lib_fps import FPS
    from SimpleGUICS2Pygame.simplegui_lib_loader import Loader

    import SimpleGUICS2Pygame.simpleguics2pygame as simplegui

    simplegui.Frame._hide_status = True
    simplegui.Frame._keep_timers = False

    SIMPLEGUICS2PYGAME = True


#
# Global constants
###################
SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600


#
# Global variables
###################
frame = None

ricerocks = None


#
# Helper functions
###################
def angle_to_vector(angle):
    """
    Return the vector corresponding to the angle expressed in radians.

    :param angle: int or float

    :return: (-1 <= float <= 1, -1 <= float <= 1)
    """
    assert isinstance(angle, int) or isinstance(angle, float), type(angle)

    return (math.cos(angle), math.sin(angle))


def vector_to_angle(vector):
    """
    Return the angle (in radians) corresponding to the direction of vector.

    :param vector: (int or float, int or float)
                   or [int or float, int or float]

    :return: -math.pi <= float <= math.pi
    """
    assert_position(vector)

    return math.atan2(vector[1], vector[0])


#
# Classes
##########
class RiceRocks:
    """
    General class dealing the game.
    """
    def __init__(self):
        """
        Set elements of the game.
        """
        self.loaded = False

        self.keydown_left = False
        self.keydown_right = False
        self.lives = 3
        self.my_ship = None
        self.nb_bombs = None
        self.score = 0
        self.started = False
        self.time = 0

        self.explosions = []
        self.live_explosions = []
        self.missiles = []
        self.rocks = []

        self.animate_background_active = True
        self.music_active = True
        self.sounds_active = True
        self.timer = simplegui.create_timer(1000, self.rock_spawner)

        self.img_infos = None
        self.medias = None

    def bomb_explode(self):
        """
        If it remains bomb
        then detonated a bomb that destroys all asteroids.
        """
        if self.nb_bombs:
            self.nb_bombs -= 1
            if self.sounds_active:
                self.medias.get_sound('bomb_explode').rewind()
                self.medias.get_sound('bomb_explode').play()
            for rock in self.rocks:
                self.explosions.append(Sprite(rock.position, rock.velocity,
                                              0, rock.angle_velocity,
                                              'asteroid_explosion'))
            self.rocks = []

    def draw_and_update(self, canvas):
        """
        Draw and update all stuffs in each FPS cycle.

        :param canvas: simplegui.Canvas
        """
        self.time += 1

        # Draw static background
        if not SIMPLEGUICS2PYGAME:
            canvas.draw_image(self.medias.get_image('nebula'),
                              self.img_infos['nebula'].get_center(),
                              self.img_infos['nebula'].get_size(),
                              (SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0),
                              (SCREEN_WIDTH, SCREEN_HEIGHT))

        # Draw animated background
        if self.animate_background_active:
            center = self.img_infos['debris'].get_center()
            size = self.img_infos['debris'].get_size()

            wtime = (self.time/4.0) % SCREEN_WIDTH

            canvas.draw_image(self.medias.get_image('debris'),
                              center,
                              size,
                              (wtime - SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0),
                              (SCREEN_WIDTH, SCREEN_HEIGHT))
            canvas.draw_image(self.medias.get_image('debris'),
                              center,
                              size,
                              (wtime + SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0),
                              (SCREEN_WIDTH, SCREEN_HEIGHT))

        # Draw missiles, ship, asteroids and explosions
        for missile in self.missiles:
            missile.draw(canvas)

        if self.lives > 0:
            self.my_ship.draw(canvas)

        for rock in self.rocks:
            rock.draw(canvas)

        for i in range(len(self.explosions) - 1, -1, -1):
            explosion = self.explosions[i]

            explosion.draw(canvas)
            explosion.update()
            if explosion.lifespan <= 0:  # explosion finished
                del self.explosions[i]

        # Update ship
        self.my_ship.update()

        # Update missiles
        for i in range(len(self.missiles) - 1, -1, -1):
            missile = self.missiles[i]

            missile.update()
            if missile.lifespan <= 0:  # missile disappear
                del self.missiles[i]
            else:                      # active missile
                for j in range(len(self.rocks) - 1, -1, -1):
                    # Check collide with asteroids
                    rock = self.rocks[j]

                    if missile.collide(rock):  # collide
                        del self.missiles[i]
                        del self.rocks[j]

                        if not rock.little:
                            # Divide in two little asteroids
                            angle = vector_to_angle(missile.velocity)
                            mvel = math.sqrt(
                                (rock.velocity[0]*rock.velocity[0]
                                 + rock.velocity[1]*rock.velocity[1])/2.0)

                            vel = list(angle_to_vector(angle - math.pi/4))
                            vel[0] *= mvel
                            vel[1] *= mvel
                            little1 = Asteroid(rock.position, vel,
                                               rock.angle_velocity*2,
                                               rock.num, True)

                            vel = list(angle_to_vector(angle + math.pi/4))
                            vel[0] *= mvel
                            vel[1] *= mvel
                            little2 = Asteroid(rock.position, vel,
                                               -rock.angle_velocity*2,
                                               rock.num, True)

                            while True:
                                little1.position[0] += little1.velocity[0]
                                little1.position[1] += little1.velocity[1]
                                little2.position[0] += little2.velocity[0]
                                little2.position[1] += little2.velocity[1]
                                if not little1.collide(little2):
                                    break
                            self.rocks.extend((little1, little2))

                        self.score += 1
                        if self.score % 10 == 0:  # add a new bomb
                            self.nb_bombs += 1
                            if self.sounds_active:
                                self.medias.get_sound('bomb_extra').rewind()
                                self.medias.get_sound('bomb_extra').play()
                            if self.score % 50 == 0:  # add a new live
                                self.lives += 1

                        self.explosions.append(Sprite(rock.position,
                                                      rock.velocity,
                                                      0, rock.angle_velocity,
                                                      'asteroid_explosion'))

                        break

        # Update asteroids
        for i in range(len(self.rocks) - 1, -1, -1):
            rock = self.rocks[i]

            rock.update()
            if self.my_ship.collide(rock):  # collide with ship
                del self.rocks[i]

                self.explosions.append(Sprite(rock.position, rock.velocity,
                                              0, rock.angle_velocity,
                                              'asteroid_collide_explosion'))
                self.live_explosions.append(Sprite((self.lives*40, 40),
                                                   (0, 0),
                                                   -math.pi/2, 0,
                                                   'live_explosion'))

                self.lives = max(0, self.lives - 1)
                if self.lives <= 0:  # game over
                    self.stop()

                    self.explosions.append(Sprite(self.my_ship.position,
                                                  self.my_ship.velocity,
                                                  0,
                                                  self.my_ship.angle_velocity,
                                                  'ship_explosion'))

                    if self.sounds_active:
                        self.medias.get_sound('death').rewind()
                        self.medias.get_sound('death').play()

                    break
                else:
                    if self.sounds_active:
                        self.medias.get_sound('collide').rewind()
                        self.medias.get_sound('collide').play()
            else:
                for j in range(0, i):
                    other = self.rocks[j]
                    if rock.collide(other):
                        rock.position[0] = (rock.position[0]
                                            - rock.velocity[0]) % SCREEN_WIDTH
                        rock.position[1] = (rock.position[1]
                                            - rock.velocity[1]) % SCREEN_HEIGHT

                        other.position[0] = ((other.position[0]
                                              - other.velocity[0])
                                             % SCREEN_WIDTH)
                        other.position[1] = ((other.position[1]
                                              - other.velocity[1])
                                             % SCREEN_HEIGHT)

                        # Elastic collision (with radius*3 as mass)
                        # https://en.wikipedia.org/wiki/Elastic_collision
                        tmp_sum = (rock.radius + other.radius)*3
                        tmp_diff = (rock.radius - other.radius)*3

                        double = 2*3*other.radius
                        new_x = (float(tmp_diff*rock.velocity[0]
                                       + double*other.velocity[0])/tmp_sum)
                        new_y = (float(tmp_diff*rock.velocity[1]
                                       + double*other.velocity[1])/tmp_sum)

                        double = 2*3*rock.radius
                        other.velocity[0] = float(
                            double*rock.velocity[0]
                            - tmp_diff*other.velocity[0])/tmp_sum
                        other.velocity[1] = float(
                            double*rock.velocity[1]
                            - tmp_diff*other.velocity[1])/tmp_sum

                        rock.velocity[0] = new_x
                        rock.velocity[1] = new_y

        # Display number of lives
        if self.started:
            info = self.img_infos['ship']
            for i in range(self.lives):
                canvas.draw_image(self.medias.get_image('ship'),
                                  info.get_center(), info.get_size(),
                                  (40 + i*40, 40), (40, 40),
                                  -math.pi/2)

        # Draw and update live explosions
        for i in range(len(self.live_explosions) - 1, -1, -1):
            live_explosion = self.live_explosions[i]

            canvas.draw_image(live_explosion.image,
                              live_explosion.image_center,
                              live_explosion.image_size,
                              live_explosion.position, (40, 40),
                              -math.pi/2)
            live_explosion.update()
            if live_explosion.lifespan <= 0:  # explosion finished
                del self.live_explosions[i]

        # Display number of bombs
        if self.started and self.nb_bombs:
            info = self.img_infos['bomb']
            for i in range(self.nb_bombs):
                canvas.draw_image(self.medias.get_image('bomb'),
                                  info.get_center(), info.get_size(),
                                  (40 + i*40, 80), (20, 40),
                                  -math.pi/2)

        # Display score
        size = 36
        font = 'sans-serif'

        text1 = 'Score'
        width1 = frame.get_canvas_textwidth(text1, size, font)
        text2 = str(self.score)
        width2 = frame.get_canvas_textwidth(text2, size, font)

        canvas.draw_text(text1, (SCREEN_WIDTH - 22 - width1, 22 + size*3.0/4),
                         size, 'Gray', font)
        canvas.draw_text(text2, (SCREEN_WIDTH - 22 - width2, 22 + size*7.0/4),
                         size, 'Gray', font)

        canvas.draw_text(text1, (SCREEN_WIDTH - 20 - width1, 20 + size*3.0/4),
                         size, 'White', font)
        canvas.draw_text(text2, (SCREEN_WIDTH - 20 - width2, 20 + size*7.0/4),
                         size, 'White', font)

        # Draw splash screen if game not started
        if not self.started:
            size = self.img_infos['splash'].get_size()
            canvas.draw_image(self.medias.get_image('splash'),
                              self.img_infos['splash'].get_center(), size,
                              (SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0), size)

        # Update and draw FPS (if started)
        fps.draw_fct(canvas)

    def load_medias(self):
        """
        Load images and sounds and waiting all is loaded,
        the set the general draw handler.
        """
        self.img_infos = {'asteroid-1': ImageInfo((45, 45), (90, 90), 40),
                          'asteroid-2': ImageInfo((45, 45), (90, 90), 40),
                          'asteroid-3': ImageInfo((45, 45), (90, 90), 38),
                          'asteroid_explosion': ImageInfo((64, 64), (128, 128),
                                                          17, 24, True),
                          'asteroid_collide_explosion': ImageInfo((64, 64),
                                                                  (128, 128),
                                                                  17, 24,
                                                                  True),
                          'bomb': ImageInfo((10, 10), (20, 20)),
                          'debris': ImageInfo((320, 240), (640, 480)),
                          'little-asteroid-1': ImageInfo((45, 45), (90, 90),
                                                         27, None, False,
                                                         (60, 60)),
                          'little-asteroid-2': ImageInfo((45, 45), (90, 90),
                                                         27, None, False,
                                                         (60, 60)),
                          'little-asteroid-3': ImageInfo((45, 45), (90, 90),
                                                         26, None, False,
                                                         (60, 60)),
                          'live_explosion': ImageInfo((64, 64), (128, 128),
                                                      17, 24, True),
                          'missile': ImageInfo((5, 5), (10, 10), 3, 50),
                          'nebula': ImageInfo((400, 300), (800, 600)),
                          'ship': ImageInfo((45, 45), (90, 90), 35),
                          'ship_explosion': ImageInfo((64, 64), (128, 128),
                                                      17, 24, True),
                          'splash': ImageInfo((200, 150), (400, 300))}

        def init():
            """
            Init the game after medias loaded.
            """
            if SIMPLEGUICS2PYGAME:
                frame._set_canvas_background_image(
                    self.medias.get_image('nebula'))

            self.medias._images['live_explosion'] = \
                self.medias._images['ship_explosion']

            for i in range(1, 4):
                self.medias._images['little-asteroid-' + str(i)] = \
                    self.medias._images['asteroid-' + str(i)]

            self.medias._sounds['asteroid_collide_explosion'] = \
                self.medias._sounds['asteroid_explosion']

            self.medias.get_sound('missile').set_volume(.5)

            self.my_ship = Ship((SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0), (0, 0),
                                -math.pi/2, 'ship')

            frame.set_draw_handler(self.draw_and_update)

            frame.set_keydown_handler(keydown)
            frame.set_keyup_handler(keyup)

            frame.set_mouseclick_handler(click)

            if self.music_active:
                self.medias.get_sound('intro').play()

            self.loaded = True

        self.medias = Loader(frame, SCREEN_WIDTH, init)

        # Images by Kim Lathrop
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/asteroid_blend.png',
                              'asteroid-1')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/asteroid_blue.png',
                              'asteroid-2')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/asteroid_brown.png',
                              'asteroid-3')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/explosion_alpha.png',
                              'asteroid_explosion')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/explosion_alpha2.png',
                              'asteroid_collide_explosion')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/shot3.png',
                              'bomb')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/debris2_blue.png',
                              'debris')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/shot2.png',
                              'missile')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/nebula_brown.png',
                              'nebula')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/double_ship.png',
                              'ship')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/explosion_orange.png',
                              'ship_explosion')
        self.medias.add_image('http://commondatastorage.googleapis.com/codeskulptor-assets/lathrop/splash.png',
                              'splash')

        # Sounds from http://www.sounddogs.com/ (not free)
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/explosion.ogg',
                              'asteroid_explosion')
        self.medias.add_sound('http://rpg.hamsterrepublic.com/wiki-images/f/f4/StormMagic.ogg',
                              'bomb_explode')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-demos/pyman_assets/extralife.ogg',
                              'bomb_extra')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-demos/pyman_assets/eatedible.ogg',
                              'collide')
        self.medias.add_sound('http://rpg.hamsterrepublic.com/wiki-images/5/58/Death.ogg',
                              'death')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-demos/pyman_assets/intromusic.ogg',
                              'intro')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/soundtrack.ogg',
                              'soundtrack')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/missile.ogg',
                              'missile')
        self.medias.add_sound('http://commondatastorage.googleapis.com/codeskulptor-assets/sounddogs/thrust.ogg',
                              'ship_thrust')

        self.medias.load()

        self.medias.wait_loaded()

    def rock_spawner(self):
        """
        If the maximum is not reached
        then spawns a rock (not too close to the ship).
        """
        if len(self.rocks) < 12:
            too_close = True

            def random_vel():
                """
                :return: int or float
                """
                return min(3,
                           (random.random()*0.3*(
                            self.score/2 + 1)))*random.choice((-1, 1))

            for _ in range(10):
                rock_pos = (random.randrange(0, SCREEN_WIDTH),
                            random.randrange(0, SCREEN_HEIGHT))
                rock_vel = (random_vel(),
                            random_vel())
                rock_ang_vel = random.choice((-1, 1))*(random.random()*0.05
                                                       + 0.01)

                rock = Asteroid(rock_pos, rock_vel,
                                rock_ang_vel,
                                random.randint(1, 3))

                too_close = False
                for rock2 in self.rocks:
                    if rock2.collide(rock):
                        too_close = True

                        break
                too_close = (too_close
                             or (self.my_ship.distance(rock)
                                 < (self.my_ship.radius + rock.radius)*3))
                if not too_close:
                    break

            if not too_close:
                self.rocks.append(rock)

    def start(self):
        """
        Start the game.
        """
        if self.music_active:
            self.medias.get_sound('intro').rewind()
            self.medias.get_sound('soundtrack').play()

        self.keydown_left = False
        self.keydown_right = False
        self.lives = 3
        self.nb_bombs = 0
        self.score = 0

        self.my_ship = Ship((SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0), (0, 0),
                            -math.pi/2, 'ship')

        self.explosions = []
        self.missiles = []
        self.rocks = []

        self.timer.start()
        self.rock_spawner()

        self.started = True

    def stop(self):
        """
        Stop the game.
        """
        self.timer.stop()

        self.nb_bombs = None

        self.missiles = []
        self.rocks = []

        self.started = False

        self.my_ship.stop()

        if self.music_active:
            self.medias.get_sound('soundtrack').rewind()
            self.medias.get_sound('intro').play()


class ImageInfo:
    """
    Informations to use with Sprite.
    """
    def __init__(self, center, size,
                 radius=None, lifespan=None, animated=False,
                 draw_size=None):
        """
        Set informations.

        If radius is None
        then use maximum of size components.

        If lifespan is None
        then use infinity.

        If draw_size is None
        then use size value.

        :param center: (int or float, int or float)
                       or [int or float, int or float]
        :param size: ((int or float) > 0, (int or float) > 0)
                     or [(int or float) > 0, (int or float) > 0]
        :param radius: None or ((int or float) > 0)
        :param lifespan: None or ((int or float) > 0)
        :param animated: bool
        :param draw_size: None
                          or ((int or float) > 0, (int or float) > 0)
                          or [(int or float) > 0, (int or float) > 0]
        """
        assert_position(center)
        assert_position(size, True, True)
        assert ((radius is None)
                or ((isinstance(radius, int) or isinstance(radius, float))
                    and (radius > 0))), radius
        assert ((lifespan is None)
                or ((isinstance(lifespan, int) or isinstance(lifespan, float))
                    and (lifespan > 0))), lifespan
        assert isinstance(animated, bool), type(animated)

        if draw_size is None:
            draw_size = size

        assert_position(draw_size, True, True)

        self._center = list(center)
        self._size = list(size)
        self._radius = (max(size) if radius is None
                        else radius)
        self._lifespan = (lifespan if lifespan
                          else float('inf'))
        self._animated = animated
        self._draw_size = list(draw_size)

    def get_animated(self):
        """
        If is a animated image
        then return True,
        else return False.

        :return: bool
        """
        return self._animated

    def get_center(self):
        """
        Return position of the center of image.

        :return: [int or float, int or float]
        """
        return list(self._center)

    def get_draw_size(self):
        """
        Return draw size of image.

        :return: [(int or float) > 0, (int or float) > 0]
        """
        return list(self._draw_size)

    def get_lifespan(self):
        """
        Return lifespan of image.

        :return: None or ((int or float) > 0)
        """
        return self._lifespan

    def get_radius(self):
        """
        Return radius of image.

        :return: (int or float) > 0
        """
        return self._radius

    def get_size(self):
        """
        Return size of image.

        :return: [(int or float) > 0, (int or float) > 0]
        """
        return list(self._size)


class Sprite:
    """
    Sprite class
    """
    def __init__(self, position, velocity,
                 angle, angle_velocity,
                 media_name):
        """
        Set sprite.

        :param position: (int or float, int or float)
                         or [int or float, int or float]
        :param velocity: (int or float, int or float)
                         or [int or float, int or float]
        :param angle: int or float
        :param angle_velocity: int or float
        :param media_name: str
        """
        assert_position(position)
        assert_position(velocity)
        assert isinstance(angle, int) or isinstance(angle, float), type(angle)
        assert (isinstance(angle_velocity, int)
                or isinstance(angle_velocity, float)), type(angle_velocity)
        assert isinstance(media_name, str), type(media_name)

        if (ricerocks.sounds_active
                and (media_name in ricerocks.medias._sounds)):
            sound = ricerocks.medias.get_sound(media_name)
            sound.rewind()
            sound.play()

        self.position = list(position)
        self.velocity = list(velocity)
        self.angle = angle
        self.angle_velocity = angle_velocity
        self.image = ricerocks.medias.get_image(media_name)

        img_info = ricerocks.img_infos[media_name]
        self.animated = img_info.get_animated()
        self.image_center = img_info.get_center()
        self.image_draw_size = img_info.get_draw_size()
        self.image_size = img_info.get_size()
        self.lifespan = img_info.get_lifespan()
        self.radius = img_info.get_radius()

    def collide(self, other_sprite):
        """
        If this sprite collide with other_sprite
        then return True,
        else return False

        :param other_sprite: Sprite

        :return: bool
        """
        assert isinstance(other_sprite, Sprite), type(other_sprite)

        return ((self.position[0] - other_sprite.position[0])**2
                + (self.position[1] - other_sprite.position[1])**2
                <= (self.radius + other_sprite.radius)**2)

    def distance(self, other_sprite):
        """
        Return the distance between this sprite and other_sprite.

        :param other_sprite: Sprite

        :return: float
        """
        assert isinstance(other_sprite, Sprite), type(other_sprite)

        return math.sqrt((self.position[0] - other_sprite.position[0])**2
                         + (self.position[1] - other_sprite.position[1])**2)

    def draw(self, canvas):
        """
        Draw the sprite
        (if the associated image are not loaded, draw a red disc).

        :param canvas: simplegui.Canvas
        """
        if self.image.get_width() > 0:
            canvas.draw_image(self.image,
                              self.image_center, self.image_size,
                              self.position, self.image_draw_size,
                              self.angle)
        else:
            # Useful to debug
            canvas.draw_circle(self.position, self.radius, 1, 'Red', 'Red')

    def update(self):
        """
        Update position adding velocity,
        angle adding angle_velocity,
        lifespan and current image if animated.
        """
        self.angle += self.angle_velocity

        self.position[0] = (self.position[0]
                            + self.velocity[0]) % SCREEN_WIDTH
        self.position[1] = (self.position[1]
                            + self.velocity[1]) % SCREEN_HEIGHT

        if self.lifespan is not None:
            self.lifespan -= 1
            if self.animated:  # change the current image
                assert self.image_center[0] < self.image.get_width()

                self.image_center[0] += self.image_size[0]


class Asteroid(Sprite):
    """
    Asteroid class
    """
    def __init__(self, position, velocity,
                 angle_velocity,
                 num, little=False):
        """
        Set an asteroid sprite.

        :param position: (int or float, int or float)
                         or [int or float, int or float]
        :param velocity: (int or float, int or float)
                         or [int or float, int or float]
        :param angle_velocity: int or float
        :param num: 1, 2 or 3
        :param litle: bool
        """
        assert_position(position)
        assert_position(velocity)
        assert (isinstance(angle_velocity, int)
                or isinstance(angle_velocity, float)), type(angle_velocity)
        assert num in (1, 2, 3)
        assert isinstance(little, bool), type(little)

        Sprite.__init__(self, position, velocity,
                        0, angle_velocity,
                        ('little-asteroid-' if little
                         else 'asteroid-') + str(num))

        self.num = num
        self.little = little


class Ship(Sprite):
    """
    Ship class
    """
    def __init__(self, position, velocity,
                 angle,
                 media_name):
        """
        Set ship sprite.

        :param position: (int or float, int or float)
                         or [int or float, int or float]
        :param velocity: (int or float, int or float)
                         or [int or float, int or float]
        :param angle: int or float
        :param media_name: str
        """
        assert_position(position)
        assert_position(velocity)
        assert isinstance(angle, int) or isinstance(angle, float), type(angle)
        assert isinstance(media_name, str), type(media_name)

        Sprite.__init__(self, position, velocity, angle,
                        0,
                        media_name)

        self.thrust = False

    def flip(self):
        """
        Flip the ship.
        """
        self.angle += math.pi

    def shot(self):
        """
        Launch a missile.
        """
        vector = angle_to_vector(self.angle)

        ricerocks.missiles.append(
            Sprite((self.position[0] + self.radius*vector[0],
                    self.position[1] + self.radius*vector[1]),
                   (self.velocity[0] + vector[0]*6,
                    self.velocity[1] + vector[1]*6),
                   self.angle, 0,
                   'missile'))

    def stop(self):
        """
        Stop the ship.
        """
        self.turn(None)
        if self.thrust:
            self.thrust_on_off()

    def thrust_on_off(self):
        """
        Switch activation of thrust.
        """
        self.thrust = not self.thrust

        if self.thrust:
            if ricerocks.sounds_active:
                ricerocks.medias.get_sound('ship_thrust').play()
            # Sprite image with actif thrust
            self.image_center[0] += self.image_size[0]
        else:
            if ricerocks.sounds_active:
                ricerocks.medias.get_sound('ship_thrust').rewind()
            # Sprite image with inactif thrust
            self.image_center[0] -= self.image_size[0]

    def turn(self, right):
        """
        Turn the ship
        (in fact change angle_velocity).

        :param right: None or Bool
        """
        assert (right is None) or isinstance(right, bool), type(right)

        self.angle_velocity = {False: -0.05,
                               None: 0,
                               True: 0.05}[right]

    def update(self):
        """
        Update position adding velocity (and deal exit out of the canvas),
        decrease slightly velocity,
        and angle adding angle_velocity.

        Moreover if thrust is active then increase velocity.
        """
        # Update angle
        self.angle += self.angle_velocity

        # Update position
        self.position[0] = (self.position[0]
                            + self.velocity[0]) % SCREEN_WIDTH
        self.position[1] = (self.position[1]
                            + self.velocity[1]) % SCREEN_HEIGHT

        # Update velocity
        if self.thrust:
            acc = angle_to_vector(self.angle)
            self.velocity[0] += acc[0]*.1
            self.velocity[1] += acc[1]*.1

        self.velocity[0] *= .95
        self.velocity[1] *= .95


#
# Event handlers
#################
def click(pos):
    """
    If click on splash screen
    then start the game.

    :param pos: (int >= 0, int >= 0)
    """
    center = (SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0)
    size = ricerocks.img_infos['splash'].get_size()

    if ((not ricerocks.started)
            and ((center[0] - size[0]/2.0)
                 < pos[0]
                 < (center[0] + size[0]/2.0))
            and ((center[1] - size[1]/2.0)
                 < pos[1]
                 < (center[1] + size[1]/2.0))):
        ricerocks.start()


def fps_on_off():
    """
    Active or inactive the calculation and drawing of FPS.
    """
    if fps.is_started():
        fps.stop()
        button_fps.set_text('FPS on')
    else:
        fps.start()
        button_fps.set_text('FPS off')


def keydown(key):
    """
    Event handler to deal key down.

    :param key: int >= 0
    """
    if ricerocks.started:
        if key == simplegui.KEY_MAP['left']:
            ricerocks.keydown_left = True
            ricerocks.my_ship.turn(False)
        elif key == simplegui.KEY_MAP['right']:
            ricerocks.keydown_right = True
            ricerocks.my_ship.turn(True)
        elif key == simplegui.KEY_MAP['up']:
            ricerocks.my_ship.thrust_on_off()
        elif key == simplegui.KEY_MAP['down']:
            ricerocks.my_ship.flip()
        elif key == simplegui.KEY_MAP['space']:
            ricerocks.my_ship.shot()
        elif key == simplegui.KEY_MAP['Z']:
            ricerocks.bomb_explode()


def keyup(key):
    """
    Event handler to deal key up.

    :param key: int >= 0
    """
    if ricerocks.started:
        if key == simplegui.KEY_MAP['left']:
            ricerocks.keydown_left = False
            ricerocks.my_ship.turn(True if ricerocks.keydown_right
                                   else None)
        elif key == simplegui.KEY_MAP['right']:
            ricerocks.keydown_right = False
            ricerocks.my_ship.turn(False if ricerocks.keydown_left
                                   else None)
        elif key == simplegui.KEY_MAP['up']:
            ricerocks.my_ship.thrust_on_off()
        elif key == 27:  # Escape
            quit_prog()


def quit_prog():
    """
    Stop timer and sounds, and quit.
    """
    if ricerocks.loaded:
        ricerocks.stop()
        ricerocks.medias.pause_sounds()
        frame.stop()


def stop():
    """
    Stop the game.
    """
    if ricerocks.loaded:
        ricerocks.stop()


def switch_animate_background():
    """
    Switch animate background on/off.
    """
    ricerocks.animate_background_active = not ricerocks.animate_background_active
    button_animate_background.set_text('Static background'
                                       if ricerocks.animate_background_active
                                       else 'Animate background')


def switch_music():
    """
    Switch music on/off.
    """
    ricerocks.music_active = not ricerocks.music_active

    if ricerocks.music_active:
        button_music.set_text('Music off')
        if ricerocks.started:
            ricerocks.medias.get_sound('soundtrack').play()
        else:
            ricerocks.medias.get_sound('intro').play()
    else:
        button_music.set_text('Music on')
        ricerocks.medias.get_sound('intro').rewind()
        ricerocks.medias.get_sound('soundtrack').rewind()


def switch_sounds():
    """
    Switch sounds on/off.
    """
    ricerocks.sounds_active = not ricerocks.sounds_active
    button_sounds.set_text('Sounds off' if ricerocks.sounds_active
                           else 'Sounds on')


#
# Main
#######
if __name__ == '__main__':
    frame = simplegui.create_frame('RiceRocks (Asteroids)',
                                   SCREEN_WIDTH, SCREEN_HEIGHT, 200)

    fps = FPS(x=0, y=0, font_size=32)

    ricerocks = RiceRocks()
    ricerocks.load_medias()

    frame.add_button('Stop this game', stop)
    frame.add_label('')
    button_music = frame.add_button('Music off', switch_music)
    button_sounds = frame.add_button('Sounds off', switch_sounds)
    frame.add_label('')
    button_animate_background = frame.add_button('Static background',
                                                 switch_animate_background)
    frame.add_label('')
    frame.add_button('Quit', quit_prog)
    frame.add_label('')
    frame.add_label('Turn: Left and Right')
    frame.add_label('Accelerate: Up')
    frame.add_label('Flip: Down')
    frame.add_label('Fire: Space')
    frame.add_label('Bomb: Z (or W)')
    frame.add_label('Esc: Quit')

    frame.add_label('')
    frame.add_label('One bomb for every 10 asteroids destroyed.')
    frame.add_label('')
    frame.add_label('One live for every 50 asteroids destroyed.')

    frame.add_label('')
    frame.add_label('Useful to test:')
    button_fps = frame.add_button('FPS on', fps_on_off)

    frame.start()
