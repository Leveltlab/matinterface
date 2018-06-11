# matinterface

Hi All,

This is the source code I developed to make an interface between matlab and Directx11 for scanbox.

The PMT samples, from the Alazar analog to digital converter, are buffered to system memory (DMA) and copied to the graphics card (an NVIDIA titan X). On the graphics card the input is resampled, the green and red channels are separated, copied back to system memory and stored on file, in addition the image is output directly from the graphics card to the screen.

Because much of the work is due to resampling the data, doing this on the GPU saves time on the CPU to run the rest of the matlab code for each each image and update the buffers for new input.

Chris van der Togt, 2018
