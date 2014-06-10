"""
Script for building the example.

Usage:
    python setup.py py2app
"""
from setuptools import setup

plist = dict(
    CFBundleDocumentTypes=[
        dict(
            CFBundleTypeExtensions=["txt", "text", "*"],
            CFBundleTypeName="Text File",
            CFBundleTypeRole="Editor",
            NSDocumentClass="TinyTinyDocument",
        ),
    ],
)

setup(
    data_files=['MainMenu.nib', 'TinyTinyDocument.nib'],
    app=[
        dict(script="TinyTinyEdit.py", plist=plist),
    ],
    install_requires=["pyobjc-framework-Cocoa"],
    setup_requires=["py2app"],
    use_2to3 = True,
)
