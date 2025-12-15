if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    # enables generic ESP-NN optimizations by Espressif
    add_definitions(-DEI_CLASSIFIER_TFLITE_ENABLE_ESP_NN=1)
    add_definitions(-DEIDSP_USE_ESP_DSP=1)
    # conditionally enable ESP32S3 specific optimizations
    if(${IDF_TARGET} STREQUAL "esp32s3")
        add_definitions(-DEI_CLASSIFIER_TFLITE_ENABLE_ESP_NN_S3=1)
    endif()
endif()

set(include_dirs
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/tflite-model
    ${CMAKE_CURRENT_LIST_DIR}/model-parameters
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/include
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/porting/espressif/esp-dsp/modules/common/include
)

include(${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/cmake/utils.cmake)

RECURSIVE_FIND_FILE_EXCLUDE_DIR(SOURCE_FILES "edge-impulse-sdk" "CMSIS" "*.cpp")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(MODEL_FILES "tflite-model" "CMSIS" "*.cpp")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(CC_FILES "edge-impulse-sdk" "CMSIS" "*.cc")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(C_FILES "edge-impulse-sdk" "CMSIS" "*.c")

RECURSIVE_FIND_FILE_EXCLUDE_DIR(S_FILES_ESP_NN "edge-impulse-sdk/porting/espressif/ESP-NN" "CMSIS" "*.s")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(S_FILES_ESP_DSP "edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft" "CMSIS" "*.s")

list(APPEND SOURCE_FILES ${C_FILES})
list(APPEND SOURCE_FILES ${CC_FILES})
list(APPEND SOURCE_FILES ${MODEL_FILES})
list(APPEND SOURCE_FILES ${S_FILES_ESP_NN})
list(APPEND SOURCE_FILES ${S_FILES_ESP_DSP})

idf_component_register(
    SRCS 
        "${SOURCE_FILES}" 
    INCLUDE_DIRS 
        "${include_dirs}"
    PRIV_REQUIRES 
        esp_timer
)
