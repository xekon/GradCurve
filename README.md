Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html forum: http://forum.doom9.org/showthread.php?t=133191


Usage
=====

    grad.Curve(clip clip, char fname='path', int ftype=7, int pmode=1)

* clip: Clip to process. Only planar format with float sample type of 32 bit depth is supported.

* fname: The path to the '.acv' curve file, more formats such as .amp will be supported once initial testing is completed.

* ftype: The curve file type: 2=ACV, 3=csv, 4=crv, 5=map, 6=amp Smartvurve hsv, 7=amp

* pmode: process mode: 0=RGB only, 1=RGB + R/G/B, 2=RGB weighted, 3=RGB weighted + R/G/B, 4=no processing

	#example:
	
		import vapoursynth as vs
		core = vs.get_core()
		core.std.LoadPlugin('G:/enc/VapourSynth-GradCurve-master/GradCurve/x64/Debug/GradCurve.dll')
		src = core.d2v.Source(input=r'G:\enc\032t.d2v', rff=False)
		src = core.resize.Bicubic(clip=src, format=vs.RGBS)
		#src2 = core.grad.Curve(src,'G:/enc/032t.amp',7,1)
		src2 = core.grad.Curve(src,'G:/enc/032t.acv',2,1)
		src3 = core.std.StackHorizontal([src,src2])
		#src2.set_output()
		src3.set_output()
	
project files that can be used for testing it: http://www.mediafire.com/file/9nc2etdr9i1tc9j/gradcurve.7z