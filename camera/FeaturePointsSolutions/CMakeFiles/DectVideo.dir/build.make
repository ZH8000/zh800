# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.0

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/brianhsu/openCV/C

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/brianhsu/openCV/C

# Include any dependencies generated for this target.
include CMakeFiles/DectVideo.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/DectVideo.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/DectVideo.dir/flags.make

CMakeFiles/DectVideo.dir/dectVideo.cpp.o: CMakeFiles/DectVideo.dir/flags.make
CMakeFiles/DectVideo.dir/dectVideo.cpp.o: dectVideo.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/brianhsu/openCV/C/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/DectVideo.dir/dectVideo.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/DectVideo.dir/dectVideo.cpp.o -c /home/brianhsu/openCV/C/dectVideo.cpp

CMakeFiles/DectVideo.dir/dectVideo.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/DectVideo.dir/dectVideo.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/brianhsu/openCV/C/dectVideo.cpp > CMakeFiles/DectVideo.dir/dectVideo.cpp.i

CMakeFiles/DectVideo.dir/dectVideo.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/DectVideo.dir/dectVideo.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/brianhsu/openCV/C/dectVideo.cpp -o CMakeFiles/DectVideo.dir/dectVideo.cpp.s

CMakeFiles/DectVideo.dir/dectVideo.cpp.o.requires:
.PHONY : CMakeFiles/DectVideo.dir/dectVideo.cpp.o.requires

CMakeFiles/DectVideo.dir/dectVideo.cpp.o.provides: CMakeFiles/DectVideo.dir/dectVideo.cpp.o.requires
	$(MAKE) -f CMakeFiles/DectVideo.dir/build.make CMakeFiles/DectVideo.dir/dectVideo.cpp.o.provides.build
.PHONY : CMakeFiles/DectVideo.dir/dectVideo.cpp.o.provides

CMakeFiles/DectVideo.dir/dectVideo.cpp.o.provides.build: CMakeFiles/DectVideo.dir/dectVideo.cpp.o

CMakeFiles/DectVideo.dir/detection.cpp.o: CMakeFiles/DectVideo.dir/flags.make
CMakeFiles/DectVideo.dir/detection.cpp.o: detection.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/brianhsu/openCV/C/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/DectVideo.dir/detection.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/DectVideo.dir/detection.cpp.o -c /home/brianhsu/openCV/C/detection.cpp

CMakeFiles/DectVideo.dir/detection.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/DectVideo.dir/detection.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/brianhsu/openCV/C/detection.cpp > CMakeFiles/DectVideo.dir/detection.cpp.i

CMakeFiles/DectVideo.dir/detection.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/DectVideo.dir/detection.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/brianhsu/openCV/C/detection.cpp -o CMakeFiles/DectVideo.dir/detection.cpp.s

CMakeFiles/DectVideo.dir/detection.cpp.o.requires:
.PHONY : CMakeFiles/DectVideo.dir/detection.cpp.o.requires

CMakeFiles/DectVideo.dir/detection.cpp.o.provides: CMakeFiles/DectVideo.dir/detection.cpp.o.requires
	$(MAKE) -f CMakeFiles/DectVideo.dir/build.make CMakeFiles/DectVideo.dir/detection.cpp.o.provides.build
.PHONY : CMakeFiles/DectVideo.dir/detection.cpp.o.provides

CMakeFiles/DectVideo.dir/detection.cpp.o.provides.build: CMakeFiles/DectVideo.dir/detection.cpp.o

# Object files for target DectVideo
DectVideo_OBJECTS = \
"CMakeFiles/DectVideo.dir/dectVideo.cpp.o" \
"CMakeFiles/DectVideo.dir/detection.cpp.o"

# External object files for target DectVideo
DectVideo_EXTERNAL_OBJECTS =

DectVideo: CMakeFiles/DectVideo.dir/dectVideo.cpp.o
DectVideo: CMakeFiles/DectVideo.dir/detection.cpp.o
DectVideo: CMakeFiles/DectVideo.dir/build.make
DectVideo: /usr/lib64/libopencv_videostab.so.2.4.9
DectVideo: /usr/lib64/libopencv_video.so.2.4.9
DectVideo: /usr/lib64/libopencv_ts.a
DectVideo: /usr/lib64/libopencv_superres.so.2.4.9
DectVideo: /usr/lib64/libopencv_stitching.so.2.4.9
DectVideo: /usr/lib64/libopencv_photo.so.2.4.9
DectVideo: /usr/lib64/libopencv_objdetect.so.2.4.9
DectVideo: /usr/lib64/libopencv_nonfree.so.2.4.9
DectVideo: /usr/lib64/libopencv_ml.so.2.4.9
DectVideo: /usr/lib64/libopencv_legacy.so.2.4.9
DectVideo: /usr/lib64/libopencv_imgproc.so.2.4.9
DectVideo: /usr/lib64/libopencv_highgui.so.2.4.9
DectVideo: /usr/lib64/libopencv_gpu.so.2.4.9
DectVideo: /usr/lib64/libopencv_flann.so.2.4.9
DectVideo: /usr/lib64/libopencv_features2d.so.2.4.9
DectVideo: /usr/lib64/libopencv_core.so.2.4.9
DectVideo: /usr/lib64/libopencv_contrib.so.2.4.9
DectVideo: /usr/lib64/libopencv_calib3d.so.2.4.9
DectVideo: /usr/lib64/libGLU.so
DectVideo: /usr/lib64/libGL.so
DectVideo: /usr/lib64/libSM.so
DectVideo: /usr/lib64/libICE.so
DectVideo: /usr/lib64/libX11.so
DectVideo: /usr/lib64/libXext.so
DectVideo: /usr/lib64/libopencv_nonfree.so.2.4.9
DectVideo: /usr/lib64/libopencv_gpu.so.2.4.9
DectVideo: /usr/lib64/libopencv_photo.so.2.4.9
DectVideo: /usr/lib64/libopencv_objdetect.so.2.4.9
DectVideo: /usr/lib64/libopencv_legacy.so.2.4.9
DectVideo: /usr/lib64/libopencv_video.so.2.4.9
DectVideo: /usr/lib64/libopencv_ml.so.2.4.9
DectVideo: /usr/lib64/libopencv_calib3d.so.2.4.9
DectVideo: /usr/lib64/libopencv_features2d.so.2.4.9
DectVideo: /usr/lib64/libopencv_highgui.so.2.4.9
DectVideo: /usr/lib64/libopencv_imgproc.so.2.4.9
DectVideo: /usr/lib64/libopencv_flann.so.2.4.9
DectVideo: /usr/lib64/libopencv_core.so.2.4.9
DectVideo: CMakeFiles/DectVideo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable DectVideo"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/DectVideo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/DectVideo.dir/build: DectVideo
.PHONY : CMakeFiles/DectVideo.dir/build

CMakeFiles/DectVideo.dir/requires: CMakeFiles/DectVideo.dir/dectVideo.cpp.o.requires
CMakeFiles/DectVideo.dir/requires: CMakeFiles/DectVideo.dir/detection.cpp.o.requires
.PHONY : CMakeFiles/DectVideo.dir/requires

CMakeFiles/DectVideo.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/DectVideo.dir/cmake_clean.cmake
.PHONY : CMakeFiles/DectVideo.dir/clean

CMakeFiles/DectVideo.dir/depend:
	cd /home/brianhsu/openCV/C && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/brianhsu/openCV/C /home/brianhsu/openCV/C /home/brianhsu/openCV/C /home/brianhsu/openCV/C /home/brianhsu/openCV/C/CMakeFiles/DectVideo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/DectVideo.dir/depend
