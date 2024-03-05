#! /usr/bin/env python3

from setuptools import setup, Extension

pypbc_module = Extension("pypbc", sources=["pypbc.c"], libraries=["pbc"])

setup(
    name="pypbc",
    version="1.0",
    description="Python wrapper for the PBC (Pairing-Based Cryptography) library",
    author="Jemtaly (original by Geremy Condra)",
    author_email="Jemtaly@outlook.com",
    url="https://www.github.com/Jemtaly/pypbc",
    ext_modules=[pypbc_module],
)
