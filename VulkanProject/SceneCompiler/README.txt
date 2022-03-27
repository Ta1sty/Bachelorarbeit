This project is used to build scenes for the .vksc file format that can be read by the Vulkan-Application
Scenes created by this project should be saved to the scenes folder.
The settings for creating the scenes are specified in the appsettings.json
	this file can be overridden with another source such as the settings from the Evaluation folder
For the specific options and their purpose see CompilerConfiguration.cs

GLTFConversion contains the classes for GLTF conversion
MoanaConversion contains the classes for the Moana conversion
Scene contains classes to optimize and insert certain scene modficiations
tridecimator is a modification of the tridecimator from https://github.com/cnr-isti-vclab/vcglib to be used for created LOD
Ptex test is an executable to read ptex files - unused