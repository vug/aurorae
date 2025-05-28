# Get arguments
set(BINARY_DIR ${CMAKE_ARGV3})
set(SOURCE_DIR ${CMAKE_ARGV4})
set(INSTALL_DIR ${CMAKE_ARGV5})

# Check for normal PDB
if(EXISTS "${BINARY_DIR}/${SOURCE_DIR}.pdb")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "${BINARY_DIR}/${SOURCE_DIR}.pdb"
        "${INSTALL_DIR}/${SOURCE_DIR}.pdb"
    )
# Check for debug PDB with 'd' suffix
elseif(EXISTS "${BINARY_DIR}/${SOURCE_DIR}d.pdb")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "${BINARY_DIR}/${SOURCE_DIR}d.pdb"
        "${INSTALL_DIR}/${SOURCE_DIR}d.pdb"
    )
else()
    message(WARNING "No PDB file found for ${SOURCE_DIR} (checked both ${SOURCE_DIR}.pdb and ${SOURCE_DIR}d.pdb)")
endif()