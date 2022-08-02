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
Traversal shaders are an extension to acceleration-structure traversal. A traversal shader hereby represents a small programm, which can control how an acceleration structure is travered. We can use this to implement Multi-Level instancing, dynamic-level of detail, reduced GPU-CPU bandwidth and much more. More information on the general concept can be found in chapter 4.1(TODO link to thesis) or a the paper by (TODO won jong at al).

### Why use traversal shaders

Traversal shaders offer a couple of improvements to raytracing, especially with regard to memory and performance, they also have a couple of niche uses as well.
In terms of memory, multi-level instancing comes to mind:
  - Traveral of an accelleration structrure behaves like tree traversal, it is possible to specify an acelleration structure as a leaf node of another. In that case we can keep continuing this way as long as we want. Below is an example that explains it.
  - Level-of-detail (LoD) is a common practice to reduce performance impact of far away objects. It swaps the meshes of far away objects with lower resolutions and keeps improving resolution as the observer comes closer. In the regular case an application must precalculate the selected levels of detail in advance, however with traversal shader it is possible to select the level of detail dynamically.

(TODO, Pictures LOD)

### Walking through a forest

The best example to explain traversal shaders is for a forest of trees with leaves. Hereby the forest,trees and their leaves all have their own accelleration structure.
- Forest AS: Spans over the entire scene and contains trees as instances with different transforms. In this example this could be called the Top-Level-AS (TLAS)
- Tree AS: Contains the geometry for the branches and trunk. It also references the leaves as instanced primitves with different transforms
- Leave AS: Contains the geometry for the leaves

The way traversal works with multi-level instancing is that we start at the root (forest) of the scene graph. We then traverse down the forest until we intersect a tree instance. We then transform the ray into the coordinate system of the tree. There, traversal is continued until either the tree or an instanced leaf is hit. In the latter case we continue by transforming the ray into the coordiante system of a leaf. This is where we either insersect a triangle or we return with a negative, in any case we continue through the tree to find closer intersections in the same manner. Once we are done with the tree we finish the forest and see if we can find a closer intersection. The result of the traversal is the closest primitive.

(TODO, picture forest scenegraph)

What else? Well, you can do a lot of hacky things with them, you could teleport the light ray to another position in the scene. Implementing a light-portal in the process. Or you could start to render fractals (sphereflake) to high depths. You can also use traversal shader to implement a lazy-loading technique for AS that are not visible (TODO-Source). They also open up a couple of possiblities for moving objects, since you can represent movement by transforming the ray into another positon. Just pay attention to the bounding boxes in this case or they will give false negatives.

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

A traversal payload need to keep track of:
- Transformation: Used to transform the ray into object space and surface normals back to world space
- tNear: the closest intersection point possible inside the AABB
- selected AS: the TLAS to traverse next

### Implementing Dynamic-LOD

Since the PIs are nothing but AABBs with flagged behaviour, we can exectute any code we want. This allows to select the LOD based on the distance of the AABB to the observer by projecting the AABB onto the screen and selecting the LOD based on the projection size. We then just add the selected LOD to the traversal stack and let traversal resume.

Therefore it is required  tree, we need to keep track of where to continue next. For this we are required to implement a stack that keeps track of traversal while it is running. Most of the optimization and problems I ran into during implementation are in relation to keeping this stack as small and as optimized as possible.

### Evaluation



## Discussion



## 

### Markdown

Markdown is a lightweight and easy-to-use syntax for styling your writing. It includes conventions for

```markdown
Syntax highlighted code block

# Header 1
## Header 2
### Header 3

- Bulleted
- List

1. Numbered
2. List

**Bold** and _Italic_ and `Code` text

[Link](url) and ![Image](src)
```

For more details see [Basic writing and formatting syntax](https://docs.github.com/en/github/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax).

### Jekyll Themes

Your Pages site will use the layout and styles from the Jekyll theme you have selected in your [repository settings](https://github.com/Ta1sty/Traversal-Shader-on-Current-GPUs/settings/pages). The name of this theme is saved in the Jekyll `_config.yml` configuration file.

### Support or Contact

Having trouble with Pages? Check out our [documentation](https://docs.github.com/categories/github-pages-basics/) or [contact support](https://support.github.com/contact) and we’ll help you sort it out.
