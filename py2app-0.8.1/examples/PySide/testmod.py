from __future__ import absolute_import
import Toplevel

class Object:
   import ClassImport

   def __init__(self):
       import MethodImport
       from MethodImport2 import a, b


def foo():
    import FunctionImport
    from .FunctionImport2 import c

    if 1 == 2:
       import ConditionalFunction
       from ConditionalFunction import *

if 1 == 2:
   import ConditionalToplevel
   from . import ConditionalToplevel2

