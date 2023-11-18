#!/usr/bin/env python

# __future__ must be at the beginning of the file
from __future__ import print_function

import sys
import os
import string

from optparse import OptionParser

script_dir = os.path.dirname(os.path.abspath(__file__))

TargetDir = script_dir + "/../external/"; # target directory the source code downloaded to.

class GitRepo:
    def __init__(self, httpsUrl, moduleName, extractDir):
        self.httpsUrl   = httpsUrl
        self.moduleName = moduleName
        self.extractDir = extractDir

    def GetBranch(self):
        SrcFile = TargetDir + "../CHANGES";
        if not os.path.exists(SrcFile):
            print("Error: " + SrcFile + " does not exist!!!");
            exit(1);

        revFile = open(SrcFile,'r');
        lines = revFile.readlines();
        found = False;
        for line in lines:
            if (found == True):
                if (line.find("Branch:") == 0):
                    self.branch = line[7:];
                    break;
            if (self.moduleName.lower() in line.lower()):
                found = True;
        revFile.close();

        if (found == False):
            print("Error: Branch is not gotten from " + SrcFile + " correctly, please check it!!!")
            exit(0)
        else:
            print("Get the branch of " + self.extractDir + ": " + self.branch);

    def CheckOut(self):
        fullDstPath = os.path.join(TargetDir, self.extractDir)
        if not os.path.exists(fullDstPath):
            os.system(f"git clone {self.httpsUrl} {fullDstPath} -b {self.branch}")

        os.chdir(fullDstPath);
        os.system("git fetch");
        os.system("git checkout -b" + self.branch);

PACKAGES = [
    GitRepo("https://github.com/microsoft/mimalloc.git",       "mimalloc",       "mimalloc"),
    GitRepo("https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator",       "vma",       "vma"),
    GitRepo("https://github.com/KhronosGroup/glslang.git",       "glslang",       "glslang"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Tools.git",   "spirv-tools",   "spirv-tools"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Headers.git", "spirv-headers", "spirv-headers"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Cross.git",   "spirv-cross",   "spirv-cross"),
    GitRepo("https://github.com/glfw/glfw",   "glfw",   "glfw"),
    GitRepo("https://github.com/martinus/unordered_dense",   "unordered_dense",   "unordered_dense"),
    GitRepo("https://github.com/syoyo/tinygltf",   "tinygltf",   "tinygltf"),
    GitRepo("https://github.com/g-truc/glm",   "glm",   "glm"),
]

def GetOpt():
    global TargetDir;

    parser = OptionParser()

    parser.add_option("-t", "--targetdir", action="store",
                  type="string",
                  dest="targetdir",
                  help="the target directory source code downloaded to")

    (options, args) = parser.parse_args()

    if options.targetdir:
        print("The source code is downloaded to %s" % (options.targetdir));
        TargetDir = options.targetdir;
    else:
        print("The target directory is not specified, using default: " + TargetDir);

def DownloadSourceCode():
    global TargetDir;

    os.chdir(TargetDir);

    # Touch the spvgen CMakeLists.txt to ensure that the new directories get used.
    os.utime('../CMakeLists.txt', None)

    for pkg in PACKAGES:
        pkg.GetBranch()
        pkg.CheckOut()

GetOpt();
DownloadSourceCode();
