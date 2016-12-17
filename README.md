Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html


Usage
=====

    grad.Curve(clip clip, char var='path')

	import vapoursynth as vs
	core = vs.get_core()
	ret = core.d2v.Source(input=r'/media/sf_enc/032t.d2v', rff=False)#Frame 6167
	ret = core.resize.Bicubic(clip=ret, format=vs.RGB24, matrix_in_s="709")
	ret = core.grad.Curve(ret,var='/media/sf_enc/032t.acv')
	ret.set_output()

* clip: Clip to process. Any planar format with either integer sample type of 8-16 bit depth or float sample type of 32 bit depth is supported.

* var: The path to the '.acv' curve file, more formats such as .amp will be supported once initial testing is completed.

project files that can be used for testing it: http://www.mediafire.com/file/ofhlq1tmhe582qv/gradcurve.7z