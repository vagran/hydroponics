# This file is a part of 'hydroponics' project.
# Copyright (c) 2015, Artyom Lebedev <artyom.lebedev@gmail.com>
# All rights reserved.
# See LICENSE file for copyright details.

import os, sys

sys.path.append(os.path.join(os.environ['ADK_ROOT'], 'site_scons'))

from adk import *

DefConfAppend(INCLUDE_DIRS = '#/src/include')
