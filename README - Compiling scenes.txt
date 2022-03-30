For scene compilation the Scene Compiler is used. It creates scenes for the .vksc format

You can either use the visual studio project or compile an exe with the .bat file in the executable folder
You will need a net5.0 compiler installed as well as the cmd dotnet command line (installed with visual studio 2019, net-developement)
Alternativly you may change the project version to another version. I do not guarantee it working then

Once you have a working exe to compile a gltf scene follow the steps:

1. Start the scene compiler
2. Override the appsetting
3. Choose an *.appsettings.json that has GLTF conversion enabled. (for example the LOD test settings)
4. Choose the directory in which to save the scene. Choose one:
	- Traversal Shader Executable/Scenes - if you use the exe delivered from the repository
	- VulkanProject/Scenes - if you use the exe created by visual studio
	- A custom location if you save the exe to another place
5. Choose the gltf model. Only GLTF-Embedded files are allowed!
6. Wait for the compiler to finish
7. Maybe check if the scene file has appeard in the scenes folder - in some cases the name might be to long, in that case rename it
8. Run the programm and select the scene
9. Enjoy

If you want to test scenes that use the Moana Island asset do the following:
1. Download Moana island
	- manually  - download and extract the .pbrt version 1.1 form the website and save it somewhere on you pc
	- autmatically - run the moana.bat script that downloads the data and extracts if for you. It uses the 'curl' and 'tar' command line commands
		the script creats an island folder in the same directory as the .bat
2. Start the scene compiler
3. Override the appsetting
4. Choose an *.appsettings.json that has the moana conversion enabled. (for example Instancing Test settings, or moana island. Latter takes a long time)
5. Choose the directory in which to save the scene. Choose one:
	- Traversal Shader Executable/Scenes - if you use the exe delivered from the repository
	- VulkanProject/Scenes - if you use the exe created by visual studio
	- A custom location if you save the exe to another place
6. Choose the moana root path. The pbrt folder in the island folder that was extracted.
7. Wait for the compiler to finish. Can take a long time and may consume up to 28 GB of RAM for the biggest moana set
8. Check if the scene appeared in the scenes folder - rename if name to long >100 chars
9. Run program and select the scenee
10. Enjoy