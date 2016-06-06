# Mesh2Centroids: SPH-based Lloyd's Relaxation on GPU inside 3D Meshes
![alt tag](http://www.columbia.edu/~yf2320/lloyd.jpg)
This code would do Lloyd's relaxation for point samples inside 3D meshes. 

To run or compile, you'll need a Windows machine and a video card enabled with Direct3D 11. 

It's GPU-based and interactive for millions of points. 

The implementation is done with compute shader. 

The algorithm is similar to SPH fluids, where a more theoretical (and more complete) analysis can be found in Min's paper (http://eprints.bournemouth.ac.uk/22610/).

For the meshes shown above, you may go to the 3D model repository by Keenan (http://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/)

The code is authored by Yun Fei (http://yunfei.work).

##Usage

With the controls on down-right part of screen you may (ordered from top to bottom):

Change the number of samples

Change the tessellation level of meshes

Change the speed of simulation

Change the kernel size

Change the X coordinate of the initial samples (adjust this if you want to sample different sides of the mesh)

Change the Y coordinate of the initial samples (adjust this if you want to sample different sides of the mesh)

Change the Z coordinate of the initial samples (adjust this if you want to sample different sides of the mesh)

Use inverted normal of the mesh

Load a .obj file

Re-sample

Save the samples

Change the threshold that classify the samples as surface-samples

Save the samples classified as surface-samples

Note: on some machines with HiDPI screens the window may not shown after started, press [Left Alt+Enter] to enter full-screen then click 'toggle fullscreen' button the window can be recovered. 

##Bibtex

@MISC{Fei:2012:Misc,

author = {Fei, Yun},

title = {Mesh2Centroids: SPH-based Lloyd's Relaxation on GPU inside 3D Meshes},

month = dec,

year = {2012},

url = {https://github.com/nepluno/sph-lloyd-3d}

}