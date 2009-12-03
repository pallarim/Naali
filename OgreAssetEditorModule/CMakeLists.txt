macro (MocFolder)
    file (GLOB MOCS_TO_SOURCE_GROUP *.cxx */*.cxx)
    source_group ("CMake Moc" FILES ${MOCS_TO_SOURCE_GROUP})
endmacro (MocFolder)

#############################################################################################

# Define target name and output directory
init_target (OgreAssetEditorModule OUTPUT modules/core)

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
file (GLOB XML_FILES *.xml)
file (GLOB MOC_FILES OgreScriptEditor.h OgreMaterialProperties.h)
file (GLOB RESOURCE_FILES resource/*.qrc)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

# Qt4 Moc files to subgroup "CMake Moc"
MocFolder ()

# Qt4 Wrap
QT4_WRAP_CPP(MOC_SRCS ${MOC_FILES})
QT4_WRAP_UI(UI_SRCS ${UI_FILES})
QT4_ADD_RESOURCES(RESOURCE_SRCS ${RESOURCE_FILES})

use_package (BOOST)
use_package (POCO)
use_package (OGRE)
use_package (QT4)
use_package (CURL)
use_package (XMLRPC)
use_package (PROPERTYEDITOR)
use_modules (Core Foundation Interfaces RexCommon RexLogicModule ProtocolUtilities QtModule InventoryModule AssetModule OgreRenderingModule)

build_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${MOC_SRCS} ${RESOURCE_SRCS})

link_modules (Core Foundation Interfaces RexCommon RexLogicModule ProtocolUtilities QtModule InventoryModule AssetModule OgreRenderingModule)
link_package (BOOST)
link_package (POCO)
link_package (OGRE)
link_package (QT4)
link_package (CURL)
link_package (XMLRPC)
link_package (PROPERTYEDITOR)

# Copy XML files from source directory to target directory
foreach (XML_FILE ${XML_FILES})
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${XML_FILE} ${TARGET_DIR})
endforeach (XML_FILE)

final_target ()