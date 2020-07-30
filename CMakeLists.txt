cmake_minimum_required(VERSION 3.9)
project(clipper2)

find_package(catkin REQUIRED)

add_compile_options(-std=c++11)

catkin_package(
  INCLUDE_DIRS inc
  LIBRARIES ${PROJECT_NAME}
)

include_directories(
  inc
  ${catkin_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
  src/clipper.cpp
  src/clipper_core.cpp
)
target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES})

install(DIRECTORY inc/${PROJECT_NAME}/
   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
   FILES_MATCHING PATTERN "*.h"
)

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
