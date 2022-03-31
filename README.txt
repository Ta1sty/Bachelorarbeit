Greetings!

This is the bachelor-thesis of Markus Robert Hall @KIT.
This repository contains the software that was developed alongside as well as the used testdata.

REQUIREMENTS
Scene compiler requieres the windows-x64 build and at least 32GB ram to compile every scene
VulkanProject(Traversal Shader) requires a graphics device which support the vulkan ray tracing extensions, for example an RTX 3060
The full specs for the testing machine can be seen in the evaluation folder

Each subfolder should also have a README which specifies its use
The folder contents are as follows

1 Evaluation and Results - contains test results as well as the settings to recreate the test scenes
2 Models - contains a few models for demonstration
3 Preview Pictures - the picutres in chapter 1
4 Scene Compiler Executable - a batch file to create a windows-x64 build for the scene compiler
5 Showcase Pictures - the figures used
6 Traversal Shader Executabele - contains a ready .exe file used to run the project
7 Vulkan project - the source code and a few test projects, as well as the scene folder

All scenes used by the traversal shader application have to be in the .vksc format! 
They are created by the scene compiler

The moana island scene can be downloaded from the link.
then extract its contents such that the path to the island pbrt file is something like ./island/pbrt/island.pbrt

The moana.bat file should automate this process on windows computers.
Moana Island is missing a specified texture in a tree mesh. Can be manually fixed or ignored, 
it should not impact performance noticably (<1 FPS)

The moana island scene follows the given license
the download link is provided in this folder
