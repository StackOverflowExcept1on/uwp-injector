# Function that tweaks some compiler and linker flags for target
function(set_tweaks_options target)
    # Some flags to optimize for binary file size
    target_link_options(${target} PRIVATE
        /MANIFEST:NO
        /EMITTOOLVERSIONINFO:NO
        /EMITPOGOPHASEINFO
        /DEBUG:NONE)

    # Apply pedantic flags
    target_compile_options(${target} PRIVATE /W4 /permissive- /WX)
endfunction()
