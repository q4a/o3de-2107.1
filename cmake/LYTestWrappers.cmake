#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

include_guard()

# LY_GOOGLETEST_EXTRA_PARAMS to append additional
# google test parameters to every google test invocation.
set(LY_GOOGLETEST_EXTRA_PARAMS CACHE STRING "Allows injection of additional options to be passed to all google test commands")

# Note pytest does not need the above because it natively has PYTEST_ADDOPTS environment
# variable support.

find_package(Python REQUIRED MODULE)

ly_set(LY_PYTEST_EXECUTABLE ${LY_PYTHON_CMD} -B -m pytest -v --tb=short --show-capture=log -c ${LY_ROOT_FOLDER}/ctest_pytest.ini --build-directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
ly_set(LY_TEST_GLOBAL_KNOWN_SUITE_NAMES "smoke" "main" "periodic" "benchmark" "sandbox")
ly_set(LY_TEST_GLOBAL_KNOWN_REQUIREMENTS "gpu")

# Set default to 20 minutes
ly_set(LY_TEST_DEFAULT_TIMEOUT 1200)

# Add the CMake Test targets for each suite if testing is supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_suite_build_and_run_targets(${LY_TEST_GLOBAL_KNOWN_SUITE_NAMES})
endif()

# Set and create folders for PyTest and GTest xml output
ly_set(PYTEST_XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Testing/Pytest)
ly_set(GTEST_XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Testing/Gtest)
file(MAKE_DIRECTORY ${PYTEST_XML_OUTPUT_DIR})
file(MAKE_DIRECTORY ${GTEST_XML_OUTPUT_DIR})

#! ly_check_valid_test_suites: internal helper to check for errors in test suites
function(ly_check_valid_test_suite suite_name)
    if(NOT ${suite_name} IN_LIST LY_TEST_GLOBAL_KNOWN_SUITE_NAMES)
        message(SEND_ERROR "Invalid test suite name ${suite_name} in ${CMAKE_CURRENT_LIST_FILE}, it can only be one of the following: ${LY_TEST_GLOBAL_KNOWN_SUITE_NAMES} or unspecified.")
    endif()
endfunction()

#! ly_check_valid_test_requires: internal helper to check for errors in test requirements
function(ly_check_valid_test_requires)
    foreach(name_check ${ARGV})
        if(NOT ${name_check} IN_LIST LY_TEST_GLOBAL_KNOWN_REQUIREMENTS)
           message(SEND_ERROR "Invalid test requirement name ${name_check} in ${CMAKE_CURRENT_LIST_FILE}, it can only be one of the following: ${LY_TEST_GLOBAL_KNOWN_REQUIREMENTS} or unspecified")
        endif()
    endforeach()
endfunction()

#! ly_strip_target_namespace: Strips all "::" namespaces from target
# \arg: TARGET - Target to remove namespace prefixes
# \arg: OUTPUT_VARIABLE  - Variable which will be set to output containing the stripped target
function(ly_strip_target_namespace)
    set(one_value_args TARGET OUTPUT_VARIABLE)
    cmake_parse_arguments(ly_strip_target_namespace "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if(NOT ly_strip_target_namespace_OUTPUT_VARIABLE)
        message(SEND_ERROR "Output variable must be supplied to ${CMAKE_CURRENT_FUNCTION}")
    endif()
    string(REPLACE "::" ";" qualified_name_list ${ly_strip_target_namespace_TARGET})
    list(POP_BACK qualified_name_list stripped_target)
    set(${ly_strip_target_namespace_OUTPUT_VARIABLE} ${stripped_target} PARENT_SCOPE)
endfunction()

#! ly_add_test: Adds a new RUN_TEST using for the specified target using the supplied command
#
# \arg:NAME - Name to for the test run target
# \arg:TEST_REQUIRES(optional) - List of system resources that are required to run this test.
#      Only available option is "gpu"
# \arg:TEST_SUITE(optional) - "smoke" or "periodic" or "sandbox" - prevents the test from running normally
#      and instead places it a special suite of tests that only run when requested.
#      Otherwise, do not specify a TEST_SUITE value and the default ("main") will apply.
#      "benchmark" is currently reserved for Google Benchmarks
#      "sandbox" should be only be used for the workflow of flaky tests
# \arg:TIMEOUT (optional) The timeout in seconds for the module. Defaults to LY_TEST_DEFAULT_TIMEOUT.
# \arg:TEST_COMMAND - Command which runs the tests. It is a required argument
# \arg:NON_IDE_PARAMS - extra params that will be run in ctest, but will not be used in the IDE.
#                       This exists because there is only one IDE "target" (dll or executable)
#                       but many potential tests which invoke it with different filters (suites).
#                       Those params which vary per CTest are stored in this parameter
#                       and do not appy to the "launch options" for the IDE target.
# \arg:RUNTIME_DEPENDENCIES (optional) - List of additional runtime dependencies required by this test.
# \arg:COMPONENT (optional) - Scope of the feature area that the test belongs to (eg. physics, graphics, etc.).
# \arg:LABELS (optional) - Additional labels to apply to the test target which can be used by ctest filters.
# \arg:EXCLUDE_TEST_RUN_TARGET_FROM_IDE(bool) - If set the test run target will be not be shown in the IDE
# \arg:TEST_LIBRARY(internal) - Internal variable that contains the library be used. This is only to be used by the other
#      ly_add_* function below, not by user code
# sets LY_ADDED_TEST_NAME to the fully qualified name of the test, in parent scope
function(ly_add_test)
    set(options EXCLUDE_TEST_RUN_TARGET_FROM_IDE)
    set(one_value_args NAME TEST_LIBRARY TEST_SUITE TIMEOUT)
    set(multi_value_args TEST_REQUIRES TEST_COMMAND NON_IDE_PARAMS RUNTIME_DEPENDENCIES COMPONENT LABELS)
    # note that we dont use TEST_LIBRARY here, but PAL files might so do not remove!

    cmake_parse_arguments(ly_add_test "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (ly_add_test_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "Invalid arguments passed to ly_add_test: ${ly_add_test_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT ly_add_test_NAME)
        message(FATAL_ERROR "You must provide a name for the target")
    endif()

    if(NOT ly_add_test_TEST_SUITE)
        set(ly_add_test_TEST_SUITE "main")
    endif()

    # Set default test module timeout
    if(NOT ly_add_test_TIMEOUT)
        set(ly_add_test_TIMEOUT ${LY_TEST_DEFAULT_TIMEOUT})
    endif()

    if(NOT ly_add_test_TEST_COMMAND)
        message(FATAL_ERROR "TEST_COMMAND arguments must be supplied in order to run test")
    endif()

    # Treat supplied TEST_COMMAND argument as a list. The first element is used as the test command
    list(POP_FRONT ly_add_test_TEST_COMMAND test_command)
    # The remaining elements are treated as arguments to the command
    set(test_arguments ${ly_add_test_TEST_COMMAND})

    # Check that the supplied test suites and test requires are from the choices we provide
    ly_check_valid_test_requires(${ly_add_test_TEST_REQUIRES})
    ly_check_valid_test_suite(${ly_add_test_TEST_SUITE})

    set(qualified_test_run_name_with_suite "${ly_add_test_NAME}.${ly_add_test_TEST_SUITE}")

    # Retrieves platform specific arguments that is used for adding arguments to the test
    set(PAL_TEST_COMMAND_ARGS)

    set(wrapper_file ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/LYTestWrappers_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
    if(NOT EXISTS ${wrapper_file})
        ly_get_absolute_pal_filename(wrapper_file ${wrapper_file})
    endif()
    include(${wrapper_file})

    add_test(
        NAME ${qualified_test_run_name_with_suite}::TEST_RUN
        COMMAND ${test_command} ${test_arguments} ${ly_add_test_NON_IDE_PARAMS} ${PAL_TEST_COMMAND_ARGS}
    )

    set(LY_ADDED_TEST_NAME ${qualified_test_run_name_with_suite}::TEST_RUN)
    set(LY_ADDED_TEST_NAME ${LY_ADDED_TEST_NAME} PARENT_SCOPE)

    # Store the test so we can walk through all of them in LYTestImpactFramework.cmake
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS ${LY_ADDED_TEST_NAME})
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS_${LY_ADDED_TEST_NAME}_TEST_NAME ${ly_add_test_NAME})
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS_${LY_ADDED_TEST_NAME}_TEST_SUITE ${ly_add_test_TEST_SUITE})
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS_${LY_ADDED_TEST_NAME}_TEST_LIBRARY ${ly_add_test_TEST_LIBRARY})

    set(final_labels SUITE_${ly_add_test_TEST_SUITE})

    if (ly_add_test_TEST_REQUIRES)
        list(TRANSFORM ly_add_test_TEST_REQUIRES PREPEND "REQUIRES_" OUTPUT_VARIABLE prepended_list)
        list(APPEND final_labels ${prepended_list})

        # Workaround so we can filter "AND" of labels until https://gitlab.kitware.com/cmake/cmake/-/issues/21087 is fixed
        list(TRANSFORM prepended_list PREPEND "SUITE_${ly_add_test_TEST_SUITE}_" OUTPUT_VARIABLE prepended_list)
        list(APPEND final_labels ${prepended_list})
    endif()

    if (ly_add_test_COMPONENT)
        list(TRANSFORM ly_add_test_COMPONENT PREPEND "COMPONENT_" OUTPUT_VARIABLE prepended_component_list)
        list(APPEND final_labels ${prepended_component_list})
    endif()

    if (ly_add_test_LABELS)
        list(APPEND final_labels ${ly_add_test_LABELS})
    endif()

    # labels expects a single param, of concatenated labels
    # this always has a value because ly_add_test_TEST_SUITE is automatically
    # filled in to be "main" if not specified.
    set_tests_properties(${LY_ADDED_TEST_NAME} 
        PROPERTIES 
            LABELS "${final_labels}"
            TIMEOUT ${ly_add_test_TIMEOUT}
    )
    
    # ly_add_test_NAME could be an alias, we need the actual un-aliased target
    set(unaliased_test_name ${ly_add_test_NAME})
    if(TARGET ${ly_add_test_NAME})
        get_target_property(alias ${ly_add_test_NAME} ALIASED_TARGET)
        if(alias)
            set(unaliased_test_name ${alias})
        endif()
    endif()

    if(NOT ly_add_test_EXCLUDE_TEST_RUN_TARGET_FROM_IDE AND NOT PAL_TRAIT_BUILD_EXCLUDE_ALL_TEST_RUNS_FROM_IDE)

        list(JOIN test_arguments " " test_arguments_line)

        if(TARGET ${unaliased_test_name})

            # In this case we already have a target, we inject the debugging parameters for the target directly
            set_target_properties(${unaliased_test_name} PROPERTIES 
                VS_DEBUGGER_COMMAND ${test_command}
                VS_DEBUGGER_COMMAND_ARGUMENTS "${test_arguments_line}"
            )

        else()

            # Adds a custom target for the test run
            # The true command is used to add a target-level dependency on the test command
            ly_strip_target_namespace(TARGET ${unaliased_test_name} OUTPUT_VARIABLE unaliased_test_name)
            add_custom_target(${unaliased_test_name} COMMAND ${CMAKE_COMMAND} -E true ${args_TEST_COMMAND} ${args_TEST_ARGUMENTS})

            file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR})
            set(ide_path ${project_path})
            # Visual Studio doesn't support a folder layout that starts with ".."
            # So strip away the parent directory of a relative path
            if (${project_path} MATCHES [[^(\.\./)+(.*)]])
                set(ide_path "${CMAKE_MATCH_2}")
            endif()
            set_target_properties(${unaliased_test_name} PROPERTIES 
                FOLDER "${ide_path}"
                VS_DEBUGGER_COMMAND ${test_command}
                VS_DEBUGGER_COMMAND_ARGUMENTS "${test_arguments_line}"
            )

        endif()

        # Include additional dependencies
        if (ly_add_test_RUNTIME_DEPENDENCIES)
            ly_add_dependencies(${unaliased_test_name} ${ly_add_test_RUNTIME_DEPENDENCIES})
        endif()

        # RUN_TESTS wont build dependencies: https://gitlab.kitware.com/cmake/cmake/issues/8774
        # In the mean time we add a custom target and mark the dependency
        ly_add_dependencies(TEST_SUITE_${ly_add_test_TEST_SUITE} ${unaliased_test_name})

    else()
        
        # Include additional dependencies
        if (ly_add_test_RUNTIME_DEPENDENCIES)
            ly_add_dependencies(TEST_SUITE_${ly_add_test_TEST_SUITE} ${ly_add_test_RUNTIME_DEPENDENCIES})
        endif()

    endif()

endfunction()

#! ly_add_pytest: registers target PyTest-based test with CTest
#
# \arg:NAME name of the test-module to register with CTest
# \arg:PATH path to the file (or dir) containing pytest-based tests
# \arg:PYTEST_MARKS (optional) extra pytest marker filtering string (see pytest arg "-m")
# \arg:EXTRA_ARGS (optional) additional arguments to pass to PyTest, should not include pytest marks (value for "-m" should be passed to PYTEST_MARKS)
# \arg:TEST_SERIAL (bool) disable parallel execution alongside other test modules, important when this test depends on shared resources or environment state
# \arg:TEST_REQUIRES (optional) list of system resources needed by the tests in this module.  Used to filter out execution when those system resources are not available.  For example, 'gpu'
# \arg:RUNTIME_DEPENDENCIES (optional) - List of additional runtime dependencies required by this test.
# \arg:COMPONENT (optional) - Scope of the feature area that the test belongs to (eg. physics, graphics, etc.).
# \arg:EXCLUDE_TEST_RUN_TARGET_FROM_IDE(bool) - If set the test run target will be not be shown in the IDE
# \arg:TEST_SUITE(optional) - "smoke" or "periodic" or "sandbox" - prevents the test from running normally
#      and instead places it a special suite of tests that only run when requested.
# \arg:TIMEOUT (optional) The timeout in seconds for the module. If not set defaults to LY_TEST_DEFAULT_TIMEOUT
#
function(ly_add_pytest)

    if(NOT PAL_TRAIT_TEST_PYTEST_SUPPORTED)
        return()
    endif()

    set(options TEST_SERIAL)
    set(oneValueArgs NAME PATH TEST_SUITE PYTEST_MARKS)
    set(multiValueArgs EXTRA_ARGS COMPONENT)

    cmake_parse_arguments(ly_add_pytest "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ly_add_pytest_PATH)
        message(FATAL_ERROR "Must supply a value for PATH to tests")
    endif()

    if(ly_add_pytest_PYTEST_MARKS)
        # Suite marker args added by non_ide_params will duplicate those set by custom_marks_args
        # however resetting flags is safe and overrides with the final value set
        set(custom_marks_args "-m" "${ly_add_pytest_PYTEST_MARKS}")
    endif()

    string(REPLACE "::" "_" pytest_report_directory "${PYTEST_XML_OUTPUT_DIR}/${ly_add_pytest_NAME}.xml")

    ly_add_test(
        NAME ${ly_add_pytest_NAME}
        TEST_SUITE ${ly_add_pytest_TEST_SUITE}
        LABELS FRAMEWORK_pytest
        TEST_COMMAND ${LY_PYTEST_EXECUTABLE} ${ly_add_pytest_PATH} ${ly_add_pytest_EXTRA_ARGS} --junitxml=${pytest_report_directory} ${custom_marks_args}
        TEST_LIBRARY pytest
        COMPONENT ${ly_add_pytest_COMPONENT}
        ${ly_add_pytest_UNPARSED_ARGUMENTS}
    )

    set_tests_properties(${LY_ADDED_TEST_NAME} PROPERTIES RUN_SERIAL "${ly_add_pytest_TEST_SERIAL}")
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS_${LY_ADDED_TEST_NAME}_SCRIPT_PATH ${ly_add_pytest_PATH})
endfunction()

#! ly_add_editor_python_test: registers target Editor Python Bindings test with CTest
#
# \arg:NAME name of the test-module to register with CTest
# \arg:PATH path to the file (or dir) containing Editor Python Bindings-based tests
# \arg:TEST_PROJECT Name of the project to be set before running the test
# \arg:TEST_SUITE name of the test suite to register with CTest
# \arg:TEST_SERIAL (bool) disable parallel execution alongside other test modules, important when this test depends on shared resources or environment state
# \arg:TEST_REQUIRES (optional) list of system resources needed by the tests in this module.  Used to filter out execution when those system resources are not available.  For example, 'gpu'
# \arg:RUNTIME_DEPENDENCIES (optional) - List of additional runtime dependencies required by this test.
#      "Editor" and "EditorPythonBindings" gem are automatically included as dependencies.
# \arg:COMPONENT (optional) - Scope of the feature area that the test belongs to (eg. physics, graphics, etc.).
# \arg:TIMEOUT (optional) The timeout in seconds for the module. If not set, will have its timeout set by ly_add_test to the default timeout.
function(ly_add_editor_python_test)
    if(NOT PAL_TRAIT_TEST_PYTEST_SUPPORTED)
        return()
    endif()

    set(options TEST_SERIAL)
    set(oneValueArgs NAME PATH TEST_SUITE TEST_PROJECT TIMEOUT)
    set(multiValueArgs TEST_REQUIRES RUNTIME_DEPENDENCIES COMPONENT)
    
    cmake_parse_arguments(ly_add_editor_python_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(executable_target $<TARGET_FILE:Legacy::Editor>)

    if(NOT TARGET Legacy::Editor)
        message(FATAL_ERROR "Legacy::Editor was not recognized as a valid target")
    endif()

    if(NOT ly_add_editor_python_test_PATH)
        message(FATAL_ERROR "Must supply a value for PATH to tests")
    endif()
        
    if(NOT ly_add_editor_python_test_TEST_SUITE)
        message(FATAL_ERROR "Must supply a value for TEST_SUITE")
    endif()

    file(REAL_PATH ${ly_add_editor_python_test_TEST_PROJECT} project_real_path BASE_DIRECTORY ${LY_ROOT_FOLDER})

    # Run test via the run_epbtest.cmake script.
    # Parameters used are explained in run_epbtest.cmake.
    ly_add_test(
        NAME ${ly_add_editor_python_test_NAME}
        TEST_REQUIRES ${ly_add_editor_python_test_TEST_REQUIRES}
        TEST_COMMAND ${CMAKE_COMMAND}
            -DCMD_ARG_TEST_PROJECT=${project_real_path} 
            -DCMD_ARG_EDITOR=$<TARGET_FILE:Legacy::Editor> 
            -DCMD_ARG_PYTHON_SCRIPT=${ly_add_editor_python_test_PATH}
            -DPLATFORM=${PAL_PLATFORM_NAME}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/run_epbtest.cmake
        RUNTIME_DEPENDENCIES
            ${ly_add_editor_python_test_RUNTIME_DEPENDENCIES}
            Gem::EditorPythonBindings.Editor
            Legacy::CryRenderNULL
            Legacy::Editor
        TEST_SUITE ${ly_add_editor_python_test_TEST_SUITE}
        LABELS FRAMEWORK_pytest
        TEST_LIBRARY pytest_editor
        TIMEOUT ${ly_add_editor_python_test_TIMEOUT}
        COMPONENT ${ly_add_editor_python_test_COMPONENT}
    )

    set_tests_properties(${LY_ADDED_TEST_NAME} PROPERTIES RUN_SERIAL "${ly_add_editor_python_test_TEST_SERIAL}")
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TESTS_${LY_ADDED_TEST_NAME}_SCRIPT_PATH ${ly_add_editor_python_test_PATH})
endfunction()

#! ly_add_googletest: Adds a new RUN_TEST using for the specified target using the supplied command or fallback to running
#                     googletest tests through AzTestRunner
# \arg:NAME Name to for the test run target
# \arg:TEST_REQUIRES(optional) List of system resources that are required to run this test.
#      Only available option is "gpu"
# \arg:TEST_SUITE(optional) - "smoke" or "periodic" or "sandbox" - prevents the test from running normally
#      and instead places it a special suite of tests that only run when requested.
# \arg:TEST_COMMAND(optional) - Command which runs the tests.
#      If not supplied, a default of "AzTestRunner $<TARGET_FILE:${NAME}> AzRunUnitTests" will be used
# \arg:COMPONENT (optional) - Scope of the feature area that the test belongs to (eg. physics, graphics, etc.).
# \arg:TIMEOUT (optional) The timeout in seconds for the module. If not set, will have its timeout set by ly_add_test to the default timeout.
# \arg:EXCLUDE_TEST_RUN_TARGET_FROM_IDE(bool) - If set the test run target will be not be shown in the IDE
function(ly_add_googletest)
    if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        message(FATAL_ERROR "Platform does not support test targets")
    endif()

    set(one_value_args NAME TEST_SUITE) 
    set(multi_value_args TEST_COMMAND COMPONENT)
    cmake_parse_arguments(ly_add_googletest "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})


    # AzTestRunner modules only supports google test libraries, regardless of whether or not 
    # google test suites are supported
    set_property(GLOBAL APPEND PROPERTY LY_AZTESTRUNNER_TEST_MODULES "${ly_add_googletest_NAME}")

    if(NOT PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
        return()
    endif()


    if (ly_add_googletest_TEST_SUITE AND NOT ly_add_googletest_TEST_SUITE STREQUAL "main")
        # if a suite is specified, we filter to only accept things which match that suite (in c++)
        set(non_ide_params "-gtest_filter=*SUITE_${ly_add_googletest_TEST_SUITE}*")
    else()
        # otherwise, if its the main suite we only runs things that dont have any of the other suites. 
        # Note: it doesn't do AND, only 'or' - so specifying SUITE_main:REQUIRES_gpu
        # will actually run everything in main OR everything tagged as requiring a GPU
        # instead of only tests tagged with BOTH main and gpu...
        # so we have to do it this way (negating all others)
        set(non_ide_params "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
    endif()

    if(NOT ly_add_googletest_TEST_COMMAND)
        # Use the NAME parameter as the build target
        set(build_target ${ly_add_googletest_NAME})
        ly_strip_target_namespace(TARGET ${build_target} OUTPUT_VARIABLE build_target)

        if(NOT TARGET ${build_target})
            message(FATAL_ERROR "A valid build target \"${build_target}\" for test run \"${ly_add_googletest_NAME}\" has not been found.\
                A valid target via the TARGET parameter or a custom TEST_COMMAND must be supplied")
        endif()

        # If command is not supplied attempts, uses the AzTestRunner to run googletest on the supplied NAME
        set(full_test_command $<TARGET_FILE:AZ::AzTestRunner> $<TARGET_FILE:${build_target}> AzRunUnitTests)
        # Add AzTestRunner as a build dependency
        ly_add_dependencies(${build_target} AZ::AzTestRunner)
    else()
        set(full_test_command ${ly_add_googletest_TEST_COMMAND})
    endif()

    string(REPLACE "::" "_" report_directory "${GTEST_XML_OUTPUT_DIR}/${ly_add_googletest_NAME}.xml")

    # Invoke the lower level ly_add_test command to add the actual ctest and setup the test labels to add_dependencies on the target
    ly_add_test(
        NAME ${ly_add_googletest_NAME}
        TEST_SUITE ${ly_add_googletest_TEST_SUITE}
        LABELS FRAMEWORK_googletest
        TEST_COMMAND ${full_test_command} --gtest_output=xml:${report_directory} ${LY_GOOGLETEST_EXTRA_PARAMS}
        TEST_LIBRARY googletest
        ${ly_add_googletest_UNPARSED_ARGUMENTS}
        NON_IDE_PARAMS ${non_ide_params}
        RUNTIME_DEPENDENCIES AZ::AzTestRunner
        COMPONENT ${ly_add_googletest_COMPONENT}
    )
endfunction()

#! ly_add_googlebenchmark: Adds a new RUN_TEST using for the specified target using the supplied command or fallback to running
#                     benchmark tests through AzTestRunner
# \arg:NAME Name to for the test run target
# \arg:TARGET Target to use to retrieve target file to run test on. NAME is adds TARGET as a dependency
# \arg:TEST_REQUIRES(optional) List of system resources that are required to run this test.
#      Only available option is "gpu"
# \arg:TEST_COMMAND(optional) - Command which runs the tests
#      If not supplied, a default of "AzTestRunner $<TARGET_FILE:${TARGET}> AzRunBenchmarks" will be used
# \arg:COMPONENT (optional) - Scope of the feature area that the test belongs to (eg. physics, graphics, etc.).
# \arg:OUTPUT_FILE_FORMAT(optional) - Format of benchmark output file. Valid options are <console|json|csv>
#      If not supplied, json is used as a default and the test run command will output results to
#      "${CMAKE_BINARY_DIR}/BenchmarkResults/" directory
#      NOTE: Not used if a custom TEST_COMMAND is supplied
# \arg:TIMEOUT (optional) The timeout in seconds for the module. If not set, will have its timeout set by ly_add_test to the default timeout.
function(ly_add_googlebenchmark)
    if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        message(FATAL_ERROR "Platform does not support test targets")
    endif()
    if(NOT PAL_TRAIT_TEST_GOOGLE_BENCHMARK_SUPPORTED)
        return()
    endif()

    set(one_value_args NAME TARGET OUTPUT_FILE_FORMAT TIMEOUT)
    set(multi_value_args TEST_REQUIRES TEST_COMMAND COMPONENT)
    cmake_parse_arguments(ly_add_googlebenchmark "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    ly_strip_target_namespace(TARGET ${ly_add_googlebenchmark_NAME} OUTPUT_VARIABLE stripped_name)
    string(TOLOWER "${ly_add_googlebenchmark_OUTPUT_FILE_FORMAT}" output_file_format_lower)

    # Make the BenchmarkResults at configure time so that benchmark results can be stored there
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/BenchmarkResults")
    if("${output_file_format_lower}" STREQUAL "console")
        set(output_format_args
            "--benchmark_out_format=console"
            "--benchmark_out=${CMAKE_BINARY_DIR}/BenchmarkResults/${stripped_name}.log"
        )
    elseif("${output_file_format_lower}" STREQUAL "csv")
        set(output_format_args
            "--benchmark_out_format=csv"
            "--benchmark_out=${CMAKE_BINARY_DIR}/BenchmarkResults/${stripped_name}.csv"
        )
    else()
        set(output_format_args
            "--benchmark_out_format=json"
            "--benchmark_out=${CMAKE_BINARY_DIR}/BenchmarkResults/${stripped_name}.json"
        )
    endif()

    if(NOT ly_add_googlebenchmark_TEST_COMMAND)
        # Use the TARGET parameter as the build target if supplied, otherwise fallback to using the NAME parameter
        set(build_target ${ly_add_googlebenchmark_TARGET})
        if(NOT build_target)
            set(build_target ${ly_add_googlebenchmark_NAME}) # Assume NAME is the TARGET if not specified
        endif()
        ly_strip_target_namespace(TARGET ${build_target} OUTPUT_VARIABLE build_target)

        if(NOT TARGET ${build_target})
            message(FATAL_ERROR "A valid build target \"${build_target}\" for test run \"${ly_add_googlebenchmark_NAME}\" has not been found.\
                A valid target via the TARGET parameter or a custom TEST_COMMAND must be supplied")
        endif()

        # If command is not supplied attempts, uses the AzTestRunner to run googlebenchmarks on the supplied TARGET
        set(full_test_command $<TARGET_FILE:AZ::AzTestRunner> $<TARGET_FILE:${build_target}> AzRunBenchmarks ${output_format_args})
    else()
        set(full_test_command ${ly_add_googlebenchmark_TEST_COMMAND})
    endif()

    ly_add_test(
        NAME ${ly_add_googlebenchmark_NAME}
        TEST_REQUIRES ${ly_add_googlebenchmark_TEST_REQUIRES}
        TEST_COMMAND ${full_test_command} ${LY_GOOGLETEST_EXTRA_PARAMS}
        TEST_SUITE "benchmark"
        LABELS FRAMEWORK_googlebenchmark
        TEST_LIBRARY googlebenchmark
        TIMEOUT ${ly_add_googlebenchmark_TIMEOUT}
        RUNTIME_DEPENDENCIES 
            ${build_target}
            AZ::AzTestRunner
        COMPONENT ${ly_add_googlebenchmark_COMPONENT}
    )

endfunction()

