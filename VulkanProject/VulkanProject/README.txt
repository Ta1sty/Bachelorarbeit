This Project is part of the Markus Halls Bachelor's thesis Traversal Shaderes on Current GPUs
This application is responsible for rendering the scenes to test the Traversal shader performance.

Huge thanks to Christopher Peters with his vulkan toy renderer
https://momentsingraphics.de/ToyRendererOverview.html

and also to Vulkan-tutorial.com
https://vulkan-tutorial.com/

A lot of code for setting up the basic vulkan functionality is from either of the two sources and adjusted accordingly.
The vulkan toy renderer has also been very helpful for understanding and testing the first acceleration structure builds.

The relevant sections are marked in code accordingly

Used libraries are the vulkan lunar sdk, glfw and imgui

The shaders folder is required by the application. The folder raytrace.frag contains the code for the traversal shader

RayQuery Loop triangle opacity check is DISABLED! to renenable it open raytrace.frag and goto line 254 and 
remove the CommitTriangleIntersectionEXT(rayQuery); and break; in the switch statement. Then reload the shader with the button.
The used tests for the thesis have also had it disabled.

