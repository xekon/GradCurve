Description
===========

GradCurve generates film like grad or other effects (like rain) by adding random noise to a video clip. This noise may optionally be horizontally or vertically correlated to cause streaking.

Ported from AviSynth plugin http://forum.doom9.org/showthread.php?t=111849


Usage
=====

    grad.Curve(clip clip, char var='path')

    src = core.grad.Curve(src,var='/media/sf_enc/032t.acv')

* clip: Clip to process. Any planar format with either integer sample type of 8-16 bit depth or float sample type of 32 bit depth is supported.

* var, uvar: The standard deviation (strength) of the luma and chroma noise, 0 is disabled. uvar does nothing for the color family of GRAY and RGB.

* hcorr, vcorr: Horizontal and vertical correlation, which causes a nifty streaking effect. Range 0.0-1.0

* seed: Specifies a repeatable grad sequence. Set to at least 0 to use.

* constant: Specifies a constant grad pattern on every frame.

The correlation factors are actually just implemented as exponential smoothing which give a weird side affect that I did not attempt to adjust. But this means that as you increase either corr factor you will have to also increase the stddev (grad amount) in order to get the same visible amount of grad, since it is being smooth out a bit.

Increase both corr factors can somewhat give clumps, or larger grad size.

And there is an interesting effect with, say, `grad.Add(var=800, hcorr=0, vcorr=0.9)` or any huge amount of strongly vertical grad. It can make a scene look like it is raining.
