Greetings!

This is the bachelor-thesis of Markus Robert Hall @KIT.
This repository contains the software that was developed alongside as well as the used testdata.


Each subfolder should also have a README which specifies its use
The folder contents are as follows

1 Evaluation and Results - contains test results as well as the settings to recreate the test scenes
2 Models - contains a few models for demonstration
3 Preview Pictures - the picutres in chapter 1
4 Scene Compiler Executable - a full self contained win-x64 build
5 Showcase Pictures - the figures used
6 Vulkan project - the source code and a few test projects, as well as the scene folder

All scenes used by the traversal shader application have to be in the .vksc format! 
They are created by the scene compiler

The moana island scene can be downloaded from the link.
then extract its contents such that the path to the island pbrt file is something like ./island/pbrt/island.pbrt

The moana.bat file should automate this process on windows computers.

The moana island scene follows the given license
the download links is provided in this folder