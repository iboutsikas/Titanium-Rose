# Titanium Rose
Titanium Rose is a rendering engine developed to explore decoupling shading from
rendering. Modern real-time applications need to provide more and more graphical
fidelity, but hardware does not become faster at the same rate. However, if we
could shade surfaces once every N frames instead of every frame, we could use
the extra time to perform more complex computations or shade more meshes in the
same amount of time.

This engine is used to investigate how various materials behave under such an
implementation. The technique used by the engine is inspired by the REYES
renderer, but works in real-time. This repository includes materials for matte
plastic, glossy fabric, chrome and a mirror.

# Hazel origins
The project started out as a very old fork of
[Hazel](https://github.com/TheCherno/Hazel) an engine developed primarily by
[Yan Chernikov](https://github.com/TheCherno). While the scope and purpose of
the engine has heavily diverged since then, and the renderer is completely
different you can still find some familiar layouts and code, especially on the
application layer. As such, a huge thank you goes out to the original engine and
the community around it!