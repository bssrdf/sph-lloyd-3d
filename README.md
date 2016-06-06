# Mesh2Centroids: SPH-based Lloyd's Relaxation on GPU inside 3D Meshes
![alt tag](http://www.columbia.edu/~yf2320/lloyd.jpg)
This code would do Lloyd's relaxation for point samples inside 3D meshes. 

To run or compile, you'll need a Windows machine and a video card enabled with Direct3D 11. 

It's GPU-based and interactive for millions of points. 

The implementation is done with compute shader. 

The algorithm is similar to SPH fluids, where a more theoretical (and more complete) analysis can be found in Min's paper (http://eprints.bournemouth.ac.uk/22610/).

For the meshes shown above, you may go to the 3D model repository by Keenan (http://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/)

The code is authored by Yun Fei (http://yunfei.work).


Bibtex:
@MISC{Fei:2012:Misc,
author = {Fei, Yun},
title = {Mesh2Centroids: SPH-based Lloyd's Relaxation on GPU inside 3D Meshes},
month = dec,
year = {2012},
url = {https://github.com/nepluno/sph-lloyd-3d}
}