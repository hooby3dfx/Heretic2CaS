Extra steps required to build Heretic2CaS
(Testing working in VS2017)
1. Make sure to build x86! (Not x64!)
2. Missing files are required from the Heretic 2 Toolkit (1.06 update), name this dir: \code\client
3. For WinSnd (these changes are weird, not sure what the deal is)
- remove Module Definition File: exports.def
- change to static library (.lib)


