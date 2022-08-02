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

### Continue 


In a sense, traversal shaders already exist in the current architecture. They are responsible for transforming the ray into the lower AS coordinate system and letting the ray traversal continue there.

However, currently we have to define which AS is traversed next during AS-Build and we are limited to single-level instancing.Therefore, the Forest-Tree-Leaf example does not work. This is where start of.

The goal is to build a shader program given our current limitations that emulates the behaviour of a traversal shader with all its functionality. I.e. we can always choose where our traversal continues and we can keep going down the tree as far as we want.



The m
The best example for usage of traversal shaderes is a forest of trees with leafs:


Whenever we shoot a ray into the scene, we traverse thought this tree of AS. If, for example, we intersect an instance of a tree we transform the ray into the tree coordinate system and let traversal continue though its AS before we resume with the rest of the forest. That way we find out closest intersection as fast as possible.



### What can we do?

Suppose we have a working traversal shader, we can then use the new functionality to implement a couple of different techniques to improve upon framerate, CPU-GPU bandwidth and GPU-Memory.
- Dynamic Level of Detail: Allows us to select lower resolution AS for objects that are further away from the observer. This can be implemented by letting the traversal shader calculate the distance from the ray-origin to the bounding box of the intersected object. Then, based on this value we can select the AS to traverse next. In this case we gain a few FPS and we are no longer required to select levels of details in advance, this saves us from constantly updating the AS every time the observer moves and releases some strain on the CPU-GPU bandwidth.
- Multi-Level Instaning: Can save us tons of memory by using instancing of a higher degree. This is espessially useful for cases like the one with the forest. But also houses and walls come to mind. The degree of saved memory can be quite astonishing. With this we are able to go above and beyond any "normal" count of triangles. Say   for example 10 Quintillion (@13FPS). This can be implemented by letting the traversal shader keep adding more structures to traverse until we reach the leafs of the AS-Tree.

What else? Well, you can do a lot of hacky things with them, you could teleport the light ray to another position in the scene. Implementing a light-portal in the process. Or you could start to render fractals (sphereflake) to high depths. You can also use traversal shader to implement a lazy-loading technique for AS that are not visible (TODO-Source). They also open up a couple of possiblities for moving objects, since you can represent movement by transforming the ray into another positon. Just pay attention to the bounding boxes in this case or they will give false negatives.

Now that is all fun and games, but how are we actually going to implement a traversal shader, given our current limitations?

## Method

As previously stated we will emulate the behaviour of a traversal shader in a single shader program. In this program we will make use of Vulkan-RayQueries.
Before we start with the implementation of shader we first require a tree of AS Structures to traverse.
In Vulkan an AS contains of two levels
- Bottom-Level-AS (BLAS): These AS contain the actual geometry, such as meshes made up by triangles. The may also contain Axis-Aligned-Bounding-Boxes (AABBs). Once a primitve (Triangle,AABB) is intersected, control is returned to the shader to confirm the intersection or to generate a custom intersection point in the case of AABBs(for example for tracing spheres).
- Top-Level-AS (TLAS): Is the entry point for a ray query and spans over instances of BLAS, with each instance having its own transform and properties.

This is the point where we introduce programmable instances (proposed by Wong Jong et al). They are basically just instances or AABBs with an attached programm that is run once they are intersected. In this program we can add AS to traverse next and transform the ray to our liking. We want these PIs to be referenced between TLAS and BLAS and between BLAS to BLAS. Sadly, we cant do either. What we can do is fake these PIs by flagging our AABBs and running more ray-queries into lower-level TLAS.

We then implement different behaviour by using a couple of if-statements that implent behaviour such as selecting a level of detail.

Whenever we traverse a tree, we need to keep track of where to continue next. For this we are required to implement a stack that keeps track of traversal while it is running. Most of the optimization and problems I ran into during implementation are in relation to keeping this stack as small and as optimized as possible.

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
