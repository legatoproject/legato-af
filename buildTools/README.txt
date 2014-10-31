The build tools are broken into the following main parts:

 - Component Model      object model, including Systems, Apps, Components, Executables, etc.
                        This is the internal intermediate representation between the parser
                        and the output generation (builders).

 - Parser               objects that know how to parse Component.cdef and .adef files and build
                        an in-memory model using the objects from the Component Model.

 - mk                   the "mk tools" ("mksys", "mkapp", etc) which are used to build individual
                        components and executables or full applications and systems of applications.
                        Uses the Parser to construct an object model, then uses Builder objects
                        to construct output files from the object model.
