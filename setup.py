#!/usr/bin/env python
from setuptools import setup

setup(name = "pilatus",
      version = "0.0.1",
      description = ("Python library and Tango device server for interfacing with pilatus camservers."),
      author = "Alexander Bjoerling",
      author_email = "alexander.bjorling@maxiv.lu.se",
      license = "GPLv3",
      url = "http://www.maxiv.lu.se",
      packages = ['pilatus'],
      entry_points = {'console_scripts':
                      ['pilatusds = pilatus.PilatusDS:main']}
     )
