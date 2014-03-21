The build tools are broken into the following main parts:

 - Component Model      object model, including Apps, Components, Executables, etc.

 - Parser               objects that know how to parse Component.cdef and .adef files and build
                        an in-memory model using the objects from the Component Model.

 - mk                   the "mk tools" ("mkexe" and "mkapp") which are used to build individual
                        executables and full applications respectively.
