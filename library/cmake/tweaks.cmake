# Function that tweaks some compiler and linker flags for target
function(set_tweaks_options target)
    # Some flags to optimize for binary file size
    target_link_options(${target} PRIVATE
            /MANIFEST:NO
            /EMITPOGOPHASEINFO
            /DEBUG:NONE)

    # Apply pedantic flags (for UWP runtime)
    target_compile_options(${target} PRIVATE /permissive- /WX)
endfunction()
