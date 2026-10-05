function(elo_resolve_vision_model output_variable cache_variable filename url sha256)
    set(${cache_variable} "" CACHE FILEPATH "Use an existing ${filename} model instead of downloading it")
    if(${cache_variable})
        if(NOT EXISTS "${${cache_variable}}")
            message(FATAL_ERROR "Configured vision model does not exist: ${${cache_variable}}")
        endif()
        set(${output_variable} "${${cache_variable}}" PARENT_SCOPE)
        return()
    endif()

    if(NOT ELO_DOWNLOAD_VISION_MODELS)
        message(FATAL_ERROR
            "${filename} is required. Set ${cache_variable} or enable ELO_DOWNLOAD_VISION_MODELS.")
    endif()

    set(model_directory "${CMAKE_BINARY_DIR}/models")
    set(model_path "${model_directory}/${filename}")
    file(MAKE_DIRECTORY "${model_directory}")

    if(NOT EXISTS "${model_path}")
        message(STATUS "Downloading pinned ELO vision model: ${filename}")
        file(DOWNLOAD
            "${url}"
            "${model_path}"
            EXPECTED_HASH "SHA256=${sha256}"
            TLS_VERIFY ON
            SHOW_PROGRESS
            STATUS download_status
            LOG download_log
        )
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            list(GET download_status 1 status_message)
            file(REMOVE "${model_path}")
            message(FATAL_ERROR "Failed to download ${filename}: ${status_message}\n${download_log}")
        endif()
    endif()

    file(SHA256 "${model_path}" actual_sha256)
    if(NOT actual_sha256 STREQUAL sha256)
        file(REMOVE "${model_path}")
        message(FATAL_ERROR "Invalid SHA-256 for ${filename}")
    endif()

    set(${output_variable} "${model_path}" PARENT_SCOPE)
endfunction()
