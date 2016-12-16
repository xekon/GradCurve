Description
===========

GradCurve filter can be used to edit the gradation curves similar to the curves function of painting programs. It is a powerful tool that allows you to do some basic correction like brightness, contrast and gamma as well as a wide range of advanced color corrections and manipulations.

Ported from VirtualDub plugin http://members.chello.at/nagiller/vdub/index.html


Usage
=====

    grad.Curve(clip clip, char var='path')

    src = core.grad.Curve(src,var='/media/sf_enc/032t.acv')

* clip: Clip to process. Any planar format with either integer sample type of 8-16 bit depth or float sample type of 32 bit depth is supported.

* var: The path.
