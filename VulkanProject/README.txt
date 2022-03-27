this folder contains the source code in a VisualStudio solution (.sln) (use visual studio 2019 or newer)

CreateSkybox - parses a file and creates a .cubtex file that is a skybox and puts it into a VkBuffer format
Libraries - contains the used libraries (GLFW,imgui and glm-unsued)
PtexTest - uses the disney ptex github repository to read out ptex textures. Was an expermiat that was discarded
Scene - contains the scene types that are passed to the buffers
SceneCompiler - the project for the scene compiler, it is used to create .vksc files to create the scenes for the project
Scenes - contains the test scenes, the VulkanProject(mainApp) uses ../Scenes to look for scenes, 
		so this folder needs to be next to the folder containing the working directory
VulkanProject - contains the actual Vulkan Application with traversal shaders
VulkanTutorial - contains the vulkan tutorial from https://vulkan-tutorial.com/
