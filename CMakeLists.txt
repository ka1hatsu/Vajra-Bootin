cmake_minimum_required(VERSION 3.24)
project(VajraBootin VERSION 0.4.5 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Vajra Bootin currently targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.5 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11 dxgi)

add_library(vajra_recommender STATIC src/catalog/Distro.cpp src/catalog/ReleaseArtifact.cpp src/recommender/Recommender.cpp)
target_include_directories(vajra_recommender PUBLIC src)
target_compile_options(vajra_recommender PRIVATE /W4 /permissive- /EHsc)

add_library(vajra_download STATIC src/download/SourcePolicy.cpp src/download/DownloadManager.cpp)
target_include_directories(vajra_download PUBLIC src)
target_compile_options(vajra_download PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(vajra_download PUBLIC winhttp)

add_library(vajra_verification STATIC src/verification/Sha256.cpp)
target_include_directories(vajra_verification PUBLIC src)
target_compile_options(vajra_verification PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(vajra_verification PUBLIC bcrypt)

add_library(vajra_workflow STATIC src/workflow/VerifiedDownload.cpp)
target_include_directories(vajra_workflow PUBLIC src)
target_compile_options(vajra_workflow PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(vajra_workflow PUBLIC vajra_download vajra_verification)

add_executable(Vajra WIN32 src/main.cpp src/app/Application.cpp src/hardware/Scanner.cpp src/ui/AppUi.cpp)
target_include_directories(Vajra PRIVATE src)
target_compile_definitions(Vajra PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(Vajra PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(Vajra PRIVATE imgui vajra_recommender vajra_workflow d3d11 dxgi advapi32)

if(BUILD_TESTING)
    add_executable(VajraRecommenderTests tests/RecommenderTests.cpp)
    target_link_libraries(VajraRecommenderTests PRIVATE vajra_recommender)
    target_compile_options(VajraRecommenderTests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME recommender_rules COMMAND VajraRecommenderTests)

    add_executable(VajraReleaseArtifactTests tests/ReleaseArtifactTests.cpp)
    target_link_libraries(VajraReleaseArtifactTests PRIVATE vajra_recommender)
    target_compile_options(VajraReleaseArtifactTests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME release_artifact_catalog COMMAND VajraReleaseArtifactTests)

    add_executable(VajraSourcePolicyTests tests/SourcePolicyTests.cpp)
    target_link_libraries(VajraSourcePolicyTests PRIVATE vajra_download)
    target_compile_options(VajraSourcePolicyTests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME trusted_source_policy COMMAND VajraSourcePolicyTests)

    add_executable(VajraDownloadManagerTests tests/DownloadManagerTests.cpp)
    target_link_libraries(VajraDownloadManagerTests PRIVATE vajra_download)
    target_compile_options(VajraDownloadManagerTests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME download_manager_guards COMMAND VajraDownloadManagerTests)

    add_executable(VajraSha256Tests tests/Sha256Tests.cpp)
    target_link_libraries(VajraSha256Tests PRIVATE vajra_verification)
    target_compile_options(VajraSha256Tests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME sha256_verification COMMAND VajraSha256Tests)

    add_executable(VajraVerifiedDownloadTests tests/VerifiedDownloadTests.cpp)
    target_link_libraries(VajraVerifiedDownloadTests PRIVATE vajra_workflow)
    target_compile_options(VajraVerifiedDownloadTests PRIVATE /W4 /permissive- /EHsc)
    add_test(NAME verified_download_workflow COMMAND VajraVerifiedDownloadTests)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Vajra)
