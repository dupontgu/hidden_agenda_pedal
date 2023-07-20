# Hidden Agenda FAQ
**(Written before anyone's asked any questions, but you get the idea)**

### `Will it work with *my* keyboard or mouse?`
If you have a standard USB keyboard or mouse, it _should_ work, but I make **no** promises. USB is complicated, and this is not my full time job. I have tested with as many mice/keebs as I could get my hands on, but I will not be buying any more to test with. It's just not practical. If you find yourself with one of these pedals and are seeing weird behavior, please email me or create a GitHub issue here and provide as much info as you can. Again - I make no promises, but if I can find time to help debug I will.

### `Can I customize the pedal?`
* Out of the "box", there are a few settings you can change using the [`REPL`](TODO). I am open to adding more configurable settings, so feel free to leave suggestions!
* If you want to add/change effects (and are comfy with C/C++), the fx [base classes](../../firmware/common/include/custom_hid.hpp) and simple [passthrough](../../firmware/common/include/mouse_fx/mouse_fx_passthrough.hpp) implementations are a good starting point.
* You can also load [CircuitPython](../../circuitpython/) onto the pedal and use it as a MIDI controller.

### `WHY?`
The idea presented itself to me in the middle of the night. My daughter was ~4 months old at the time - nobody in the house was sleeping, nobody was thinking straight. I was watching a [JHS](https://www.youtube.com/channel/UCjfbkA4jJkJY5g0wbjuoZWA) YouTube video, thinking: "damn, it's gotta feel so good to _stomp_ on that overdrive pedal right before ripping a guitar solo in front of a bunch of screaming fans." The next thought was something like: "Wouldn't it _also_ feel good to stomp on something before typing up an angry email?" I giggled to myself for an extended period of time and starting ordering parts on my phone.

Despite the admittedly silly nature of the device, there are some aspects of it that keep me genuinely excited. I think the idea of having dedicated, modular hardware that processes streams of _data_ (vs. audio) is one worth exploring further. And I hope you'll find that -- in certain configurations -- some of these effects might actually be... useful? Offloading work from hands to feet might be a game changer for some. My friends at Adafruit have recently pointed out how [adding a filter to mouse movement](https://www.youtube.com/watch?v=4SYdXDB0t4M) might help folks who can't move as smoothly as the average computer user. If you can think of any usecases for this pedal that actually help you in some way (or suggestions for features that would make it _more_ helpful to you), PLEASE let me know. I would love to hear from you.