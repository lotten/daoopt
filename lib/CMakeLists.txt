# Zlib library
add_subdirectory(zlib)

# SLS4MPE code by Frank Hutter
add_subdirectory(sls4mpe)

# GzStream library (needs gzip)
add_library(Gzstream gzstream.cpp)
target_link_libraries(Gzstream Zlib)

# Alex Ihler's FGLP/JGLP code
add_subdirectory(mex)

# Kalev Kask's stochastic greedy variable ordering code
add_subdirectory(cvo)
