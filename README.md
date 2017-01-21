Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html forum: http://forum.doom9.org/showthread.php?t=133191


Usage
=====

    grad.Curve(clip clip, char var='path')

	#example:
	import vapoursynth as vs
	core = vs.get_core()
	src = core.d2v.Source(input=r'D:\enc\032t.d2v', rff=False)
	src = core.resize.Bicubic(clip=src, format=vs.RGBS)
	src = core.grad.Curve(src,'D:/enc/032t.acv')
	src.set_output()

* clip: Clip to process. Only planar format with float sample type of 32 bit depth is supported.

* var: The path to the '.acv' curve file, more formats such as .amp will be supported once initial testing is completed.

project files that can be used for testing it: http://www.mediafire.com/file/9nc2etdr9i1tc9j/gradcurve.7z