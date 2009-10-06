# Define target name
init_target (RpcUtilities)

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

use_package (BOOST)
use_package (POCO)
use_package (XMLRPC)
use_modules (HttpUtilities)

build_library (${TARGET_NAME} STATIC ${SOURCE_FILES})
use_framework_modules (Core Foundation Interfaces)

link_modules (HttpUtilities)
link_package (BOOST)
link_package (POCO)
link_package (XMLRPC)

# Precompiled headers on MSVC
if (MSVC)
	foreach (src_file ${CPP_FILES})
        	set_source_files_properties (${src_file} PROPERTIES COMPILE_FLAGS "/YuStableHeaders.h")
	endforeach (src_file ${CPP_FILES})

	set_source_files_properties (StableHeaders.cpp PROPERTIES COMPILE_FLAGS "/YcStableHeaders.h")
endif (MSVC)

final_target ()