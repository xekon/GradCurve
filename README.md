Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html forum: http://forum.doom9.org/showthread.php?t=133191


Usage
=====

    grad.Curve(clip clip, char fname='path', int ftype=1, int pmode=1)

* clip: Clip to process. Only constant format 8-bit integer RGB input supported.

* fname: The curve file path.

* ftype: The curve file type: 1=amp 2=acv, 3=csv, 4=crv, 5=map, 6=hsv Smartvurve

* pmode: process mode: 0=RGB, 1=RGB + R/G/B, 2=RGB weighted, 3=RGB weighted + R/G/B, 4=none, 5=YUV, 6=CMYK, 7=HSV, 8=LAB

project files that can be used for testing it: http://www.mediafire.com/file/9nc2etdr9i1tc9j/gradcurve.7z

linux install:
=====

	cd $HOME/.installs/VapourSynthPlugins
	git clone https://github.com/xekon/GradCurve
	cd GradCurve
	./configure
	make
	sudo make install

linux example:
=====

	import vapoursynth as vs
	core = vs.get_core()
	#core.std.LoadPlugin("/media/sf_moon/enc/libgradcurve.so")
	src = core.d2v.Source(input=r'/media/sf_moon/enc/GradCurve/003b.d2v', rff=False)
	src = core.resize.Bicubic(clip=src, format=vs.RGB24)
	prev = src
	src = core.grad.Curve(src,'/media/sf_moon/enc/GradCurve/003b.amp', ftype=1, pmode=1)
	src = core.std.StackHorizontal([prev,src])
	src.set_output()
	
Windows install, download the [DLL](GradCurve/x64/Release/GradCurve.dll), and place it in your VapourSynth\plugins64 directory.
=====
	
windows example:
=====

	import vapoursynth as vs
	core = vs.get_core()
	#core.std.LoadPlugin('G:/moon/enc/GradCurve-master/GradCurve/x64/Release/GradCurve.dll')
	src = core.d2v.Source(input=r'G:/moon/enc/GradCurve/003b.d2v', rff=False)
	src = core.resize.Bicubic(clip=src, format=vs.RGB24)
	prev = src
	src = core.grad.Curve(src,'G:/moon/enc/GradCurve/003b.amp', ftype=1, pmode=1)
	src = core.std.StackHorizontal([prev,src])
	src.set_output()