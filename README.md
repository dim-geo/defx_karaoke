# defx_karaoke
DeFX Karaoke ported using Juce framework
This is a karaoke tool that can be used to remove voice from music.
It's based on https://defx.sourceforge.net/
You can connect it via ALSA, jack, pulseaudio, pipewire to your browser, remove voice from YT and enjoy singing along your favorite songs!

The mode of operation is this:

Audio channels have two components that are intertwined: voice and music.
Usually, voice has the same phase and amplitude in both stereo channels.

Thus, we can try to remove voice like this:

- New left channel = L-R
- New right Channel = R-L

We can also add the mid channel M = (L+R)/2 after we remove all voice frequencies, in order to restore low and high frequencies.
We remove the middle frequencies by using a modifiable lowpass and highpass filter.

Finally:

- New left channel = L-R + M
- New right Channel = R-L + M

Here are the parameters of the Karaoke filter:

- Dry/Wet mix: control how strong karaoke filter should be ( from 0% to 100%, default 100% )

- Filtered mono level: How strong the mid (M) channel should be. (0% to 200%, default 80%)

- Low-pass filter cutoff frequency.

- High-pass filter cutoff frequency.

- Q value of low-pass filter.

- Q value of high-pass filter.

- Bypass Karaoke button, if you press it, no filter is applied, like 0% in Dry/wet mix.

- Solo filtered mono, if you press this, you should hear only the mid channel ( M ). Ideally you should change filter parameters to hear only music, not voice.


![image](https://github.com/dim-geo/defx_karaoke/assets/5956557/47329ab4-2e53-4eea-80a7-4128751dff96)
