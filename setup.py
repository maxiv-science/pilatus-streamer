#!/usr/bin/env python
from setuptools import setup

setup(name = "tangods-pilatusproxy",
      version = "0.0.1",
      description = ("Tango device server for interfacing with pilatus camservers."),
      author = "Alexander Bjoerling",
      author_email = "alexander.bjorling@maxiv.lu.se",
      license = "GPLv3",
      url = "http://www.maxiv.lu.se",
      packages = ['pilatusproxy'],
      package_dir = {'':'src'},
      scripts = ['scripts/pilatusproxy']
     )
