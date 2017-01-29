Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html forum: http://forum.doom9.org/showthread.php?t=133191


Usage
=====

    grad.Curve(clip clip, char fname='path', int ftype=7, int pmode=1)

* clip: Clip to process. Only planar format with float sample type of 32 bit depth is supported.

* fname: The curve file path.

* ftype: The curve file type: 2=ACV, 3=csv, 4=crv, 5=map, 6=amp Smartvurve hsv, 7=amp

* pmode: process mode: 0=RGB only, 1=RGB + R/G/B, 2=RGB weighted, 3=RGB weighted + R/G/B, 4=no processing

project files that can be used for testing it: http://www.mediafire.com/file/9nc2etdr9i1tc9j/gradcurve.7z

linux install:
=====

	cd $HOME/.installs/VapourSynthPlugins
	git clone https://github.com/xekon/GradCurve
	cd GradCurve
	chmod +x configure
	./configure
	make
	sudo make install

linux example:
=====

	import vapoursynth as vs
	core = vs.get_core()
	#core.std.LoadPlugin("/media/sf_moon/enc/libgradcurve.so")
	src = core.d2v.Source(input=r'/media/sf_moon/enc/GradCurve/003b.d2v', rff=False)
	src = core.resize.Bicubic(clip=src, format=vs.RGBS)
	prev = src
	src = core.grad.Curve(src,'/media/sf_moon/enc/GradCurve/003b.amp',7,1)
	#src = core.grad.Curve(src,'/media/sf_moon/enc/GradCurve/003b.acv',2,1)
	src = core.std.StackHorizontal([prev,src])
	src.set_output()
	
Windows install, download the [DLL](GradCurve/x64/Release/GradCurve.dll), and place it in your VapourSynth\plugins64 directory.
	
windows example:
=====

	import vapoursynth as vs
	core = vs.get_core()
	#core.std.LoadPlugin('G:/moon/enc/VapourSynth-GradCurve-master/GradCurve/x64/Release/GradCurve.dll')
	src = core.d2v.Source(input=r'G:/moon/enc/GradCurve/003b.d2v', rff=False)
	src = core.resize.Bicubic(clip=src, format=vs.RGBS)
	prev = src
	src = core.grad.Curve(src,'G:/moon/enc/GradCurve/003b.amp',7,1)
	#src = core.grad.Curve(src,'G:/moon/enc/GradCurve/003b.acv',2,1)
	src = core.std.StackHorizontal([prev,src])
	src.set_output()