# k2gehooker
An alternative hooker to MSHookFunction for jailed devices based on breakpoints, yet keeps the capability of calling original functions!

# Usage
Full examples are available in the `Tweak.xm`!

# Notes
- works on both jailbroken and non-jailbroken (sideloadable)
- limited to 6 hooks at the same time
- be sure to consider other options for jailed devices
    - if the target function has a symbol, just use `fishhook`.
    - if you want to hook more than 6, you need to use another approach (statically implant trampolines)

# Credits
- [ellekit](https://github.com/tealbathingsuit/ellekit) for this breakpoint technique
