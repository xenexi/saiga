# Locates the SAIGA library
# This module defines:
# SAIGA_FOUND
# SAIGA_INCLUDE_DIRS
# SAIGA_LIBRARIES
# SAIGA_LIBRARY


find_path(SAIGA_INCLUDE_DIRS 
	NAMES 
		saiga/rendering/deferred_renderer.h
	PATHS
          	/usr/local/include
)

find_library(SAIGA_LIBRARY 
	NAMES 
		saiga
    	PATHS 
		/usr/local/lib
)


#GLM is required 
find_path(SAIGA_GLM_INCLUDE_DIRS 
	NAMES 
		glm.hpp
	PATHS
		/usr/local/include
		/usr/include
		/usr/local/include
	PATH_SUFFIXES
		glm
)


#OpenGL is required
find_package(OpenGL REQUIRED QUIET)

#GLEW is required
find_package(GLEW REQUIRED QUIET)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	saiga  
	DEFAULT_MSG
        SAIGA_INCLUDE_DIRS 
        SAIGA_GLM_INCLUDE_DIRS
        SAIGA_LIBRARY
        OPENGL_LIBRARIES
        GLEW_LIBRARIES
)

SET(SAIGA_INCLUDE_DIRS ${SAIGA_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
SET(SAIGA_LIBRARY ${SAIGA_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

set(SAIGA_LIBRARIES ${SAIGA_LIBRARY})