## Welcome

I'm Markus and welcome you to the GitHub-Page for my thesis "Traversal shader on current GPUs".
This topic really sparked my interest as I've always been interested in computer graphics.
Due to recommendation of my supervisor I have decided to publish my results as well as all relevant sources and code.
I will link the relevant directories and sources in here. So, without further delays, lets get into it.

## Abstract

Ever since Nvidia released their RTX GPU family, real time raytracing applications rapidly rose to popularity. 
To allow this, these GPUs feature hardware acceleration for ray traversal. However, acceleration structures are limited to single-level instancing and to a fixed function traversal behavior. This does not allow for techniques like dynamic level of detail that reduces CPU-GPU communication in between frames nor the use of multi-levelinstancing to reduce scene size on the GPU.
Won-Jong et al. [WJLV19] proposed the insertion of programmable instances into acceleration structures. In this thesis we will use programmable instances to allow for MultiLevel-Instancing and dynamic Level-of-Detail by using the Vulkan Raytracing API. This
will allow us to render the Moana Island production asset by Disney in real time while
saving multiple GBs of VRAM.

## Traversal shaders
Traversal shaders are an extension to acceleration-structure traversal. A traversal shader hereby represents a small programm, which can control how an acceleration structure(AS) is travered. This can be used to implement Multi-Level instancing and dynamic-level of detail, reduce GPU-CPU bandwidth and much more. More information on the general concept can be found in chapter [4.1](https://github.com/Ta1sty/Traversal-Shader-on-Current-GPUs/blob/master/Traversal-Shaders%20on%20current%20GPUs.pdf) or a the paper by [Won-Jong et al.](https://www.intel.com/content/www/us/en/developer/articles/technical/flexible-ray-traversal-with-an-extended-programming-model.html).

### Why use traversal shaders

Traversal shaders offer a couple of improvements to raytracing, especially with regard to memory and performance, they also have a couple of niche uses as well.
For this thesis I focused on the two following applications:
  - Traveral of an accelleration structrure behaves like tree traversal, it is possible to specify an acelleration structure as a leaf node of another. In that case we can keep continuing this way as long as we want. Below is an example that explains it.
  - Level-of-Detail (LoD) is a common practice to reduce performance impact of far away objects. It swaps the meshes of far away objects with lower resolutions and keeps improving resolution as the observer comes closer. In the regular case an application must precalculate the selected levels of detail in advance. However, with traversal shader it is possible to select the level of detail dynamically.

![alt text](Image/LoD-Levels.PNG)![LoD-Levels](https://user-images.githubusercontent.com/57618110/189333002-9dd9a1a0-191d-49a7-ba8d-b7ce1ab4d03a.PNG)


What else is possible? Well, it is possible to do a lot of hacky things with them, you could teleport the light ray to another position in the scene. Implementing a light-portal in the process. Or you could start to render fractals (sphereflake) to high depths. You can also use traversal shader to implement a lazy-loading technique for AS that are not visible (TODO-Source). They also open up a couple of possiblities for moving objects, since you can represent movement by transforming the ray into another positon. Just pay attention to the bounding boxes in this case or they will give false negatives.

### Walking through a forest

The best example to explain traversal shaders is for a forest of trees with leaves. Hereby the forest,trees and their leaves all have their own accelleration structure.
- Forest AS: Spans over the entire scene and contains trees as instances with different transforms. In this example this could be called the Top-Level-AS (TLAS)
- Tree AS: Contains the geometry for the branches and trunk. It also references the leaves as instanced primitves with different transforms
- Leave AS: Contains the geometry for the leaves

The way traversal works with multi-level instancing is that we start at the root (forest) of the scene graph. We then traverse down the forest until we intersect a tree instance. We then transform the ray into the coordinate system of the tree. There, traversal is continued until either the tree or an instanced leaf is hit. In the latter case we continue by transforming the ray into the coordiante system of a leaf. This is where we either insersect a triangle or we return with a negative, in any case we continue through the tree to find closer intersections in the same manner. Once we are done with the tree we finish the forest and see if we can find a closer intersection. The result of the traversal is the closest primitive.

(TODO, picture forest scenegraph)

### Current state of architecture 

In a sense, traversal shaders already exist in the current gpu (notably ampere for rtx, TODO check) architecture. They are responsible for transforming the ray into the lower AS coordinate system and letting the ray traversal continue there.

However, currently we have to define which AS is traversed next when intersecting an instance during the AS-Build. Additionally, we are limited to single-level instancing.Therefore, the Forest-Tree-Leaf example does not work. We choose this architecture as our starting point and develop a proof of concept by modeling a traversal shader in a GLSL shader.

## Method

As previously stated we will emulate the behaviour of a traversal shader in a single GLSL shader. In this program we will make use of Vulkan-RayQueries.
However before we can go about implementing it we are first required to take a look at the scene.

### Setting up a scene
In Vulkan an AS contains of two levels
- Bottom-Level-AS (BLAS): These AS contain the actual geometry, such as meshes made up by triangles. The may also contain Axis-Aligned-Bounding-Boxes (AABBs). Once a primitve (Triangle,AABB) is intersected, control is returned to the shader to confirm the intersection or to generate a custom intersection point in the case of AABBs(for example for tracing spheres).
- Top-Level-AS (TLAS): Is the entry point for a ray query and spans over instances of BLAS, with each instance having its own transform and properties.

With traversal shader we want a tree of acceleration structure, therefore we dont stop at a BLAS, we then define AABBs as leafs of a BLAS that represent lower level TLAS. Every vulkan accleration structure therefore adds two layers of instancing.
- Instanced BLAS as part of a TLAS
- AABBs that represent lower TLAS as geometry in a BLAS

### Programmable instances

This is the point where we introduce programmable instances (proposed by Wong Jong et al). They are basically just instances or AABBs with an attached programm that is run once they are intersected. In this program we can add AS to traverse next and transform the ray to our liking. We want these PIs to be referenced between TLAS and BLAS and between BLAS to BLAS. Sadly, we cant do either. What we can do is fake these PIs by flagging our AABBs and running more ray-queries into lower-level TLAS. In this case we only have access to PIs as children of BLAS. Different behaviour is then implemented by using a couple of if-statements that implent behaviour such as selecting a level of detail.

### Traversal stack and traveral order

Above I described the way traversal continues down a tree. At all traversed levels it is required to keep track of state of traversal inside an accelleration structure. Therefore traversal requires a stack. Furthermore, GLSL does not support recursion as it does not have a stack, therefore we implement our own.
This gives us the problem that we are unable to run more rayqueires recursivly. Instead, we let a rayquery run to finish and keep track of all intersected PIs. Once finished we loop through them and let them select their AS and add a traversal payload to the stack. Traversal finishes when there are no payloads left in the stack.
There are a couple of things that can be optimized:
- Order of added stack payloads: It makes sense that the first intersectd PI should be the first to traverse next, as the RayQuery gives us the closer AABB intersections first.
- Skipping payloads: with each payload we can associate a lower bound for any intersection inside its AABB, therefore we can skip payloads if we already found a closer hit.

Thus a payload need to keep track of:
- Transformation: Used to transform the ray into object space and surface normals back to world space
- tNear: the closest intersection point possible inside the AABB
- selected AS: the TLAS to traverse next

We can now go about implementing the different behaviour such as level of detail

### Implementing Dynamic-LOD

Since the PIs are nothing but AABBs with flagged behaviour, we can exectute any code we want. This allows to select the LOD based on the distance of the AABB to the observer by projecting the AABB onto the screen and selecting the LOD based on the projection size. We then just add the selected LOD to the traversal stack and let traversal resume.

## Evaluation

Performance was test in 3 diffent topics:
1. Level of detail
2. Multi-level instancing
3. Moana island [link]
The latter of the three was a test to showcase usability in real world examples.

### Level of detail

For level of detail we tested on a 2 million triangle statue which was instanced (TODO number) times with 6 levels of detail. Each level reduced triangle count by a factor of 6. We had three cases to test:
-NO-LOD: no levels of detail used, instead all meshes are instanced in one acceleration structure this the ground mark
-Overhead - in this sceneario the same scene was rendered with always the highest level chosen, this was to evaluate the overhead the traversal shader has
-Dynamic LOD - here we used dynamic LOD to see if it was possible to achieve an improvement over no LOD

As for the results, the overhead test showed that there is a significant cost that comes from using multiple ray queries per pixel, however with dynamic level of detail it was possible to achieve a gain in FPS over NO-LOD. Overall there was an improvement in cache efficiency by using traversal shaders.

### Multi-level instancing
In this test I picked a tree form the disney dataset an instanced it so many times until vulkan did not allow for more instances in an acceleration structure.
Then the same scene was rendered with traversal shaders using multi-level instancing. This test yielded an over 90% improvement in vram in this example, however the amount of FPS lost was also very significant with more then 50%. I also tested the  limits of traversal shader an kept instancing the forest over and over a couple of times. This resulted in a scene with an effective triangle count of 10 Quintillion running at 13 FPS.

### Moana island
In this scenario I tested the applicatbility of traversal shaders mixed with traditional rendering. Moana island was a good candidate for that as it featured a lot of single instanced meshes with a couple of multi instanced bushes. I tried to render as much of the scene as possible and ended up at about 14 million instanced with a total triangle count of 22 billion. As for the results:
The scene in single level instancing used almost all VRAM avaiable. Most notably the acceleration structures occupied over 80% of VRAM. With multi level instancing however this decreased singificantly by more than 50GBs (50%+) with only a minimal performance impact.


## Discussion

### Result simmary

In these scenearios traversal shaders showed that they do indeed work and are able to implement the mentioned concepts. Furhtermore, they show a more than significant decrease in memory usage and a slight improvement in cache efficnety. On the contrarly however, they also introduce a heavy overhead as they keep switching control between raytracing core and the streaming multiprocessor.

### Applications
Traversal shader are a huge addition to any production or render software which uses raytracing and must not fulfil real time requirements. They are perfect for objects like trees and houses which have multiple shadered submeshes like leafs, walls, rooms etc. Lod can be used to some extent and does offer some improvements, though one has to be careful on what objects to use traversal. They may also be useful for some niche uses that I mentioned earlier, specifically the reduced amount of rebuilds and lazy loading of AS.

### Drawbacks
Overall traversal should only continue up to 3 levels of instancing otherwise the overhead starts to take over. current GPUs just do not have enough raytracing power to render a scene like moana island in all its beauty with raytracing in real time. RT-Core and SM control switches take most of the shader time and overall the cache effiency does not look to great for AS that are in the dimensions of multiple GBs, though they do improve a little.

### Outlook
Hardware support for traveral shaders, if something along those lines comes in a new vulkan version/ NVidia driver, then I would see some real promise. It isnt even nessecary to support traveral shaders in full generaltity. Just the injection of a program before selecting a single accerlation structure to chose where to continue would be enough to allow for almost all technqiues. More genrally speaking it would definetly be beneficial for reduced latency for any control switches for ray queries in any program. Overall I was astonished at the decrease in memory usage by multi instancing a couple of bushes.

### Thanks
That is all for now. I am currently working privatly on another raytracer with vulkan where I will incorperate my traversal shader to some degree. And depending on the results I might publish that repository aswell. My thanks go out to all people who helped me, all the nice tutorials and public repos out there. You can find these at the end of the Pdf in subscetion (TODO)
I thank you sincerely for reading. 
